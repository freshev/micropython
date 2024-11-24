#!/usr/bin/python
#-*- coding: UTF-8 -*-

import sys
import os
import io
import re
import json
import shutil

source_path = r"../luat"
if len(sys.argv) >= 3:
    source_path = sys.argv[1]

print("path:")
print(source_path)

file_list = []
for home, dirs, files in os.walk(source_path):
    for filename in files:
        if filename.endswith(".c"):
            file_list.append(os.path.join(home, filename))

for home, dirs, files in os.walk(source_path+"/../lua"):
    for filename in files:
        if filename.endswith(".c"):
            file_list.append(os.path.join(home, filename))

for home, dirs, files in os.walk(source_path+"/../components"):
    for filename in files:
        if filename.endswith(".c"):
            file_list.append(os.path.join(home, filename))

for home, dirs, files in os.walk(source_path+"/../bsp/rtt"):
    for filename in files:
        if filename.endswith(".c"):
            file_list.append(os.path.join(home, filename))

#Comment format:
# /*
#@sys_pub mod
#The first line states the purpose of the message, such as: WIFI scan completed
#WLAN_SCAN_DONE (the full name of the topic)
#@string The first data passed, @ followed by the data type, a space followed by the data explanation, if not, don’t write these lines
#@number second data
#...According to actual situation, list all passed data
# @usage
#--Examples of use, can be multiple lines
# sys.taskInit(function()
#     xxxxxxxxxx
#     xxxxxxx
#     sys.waitUntil("WLAN_SCAN_DONE")
#     xxxxxxxxxx
# end)
# */
modules = []
#Data structure:
#Moduless = {
#'mod': [
#         {
#             'topic':'WLAN_SCAN_DONE',
#'summary': 'WIFI scan completed',
#'return': [
#                 {
#'type': 'string',
#'summary': 'The first data passed'
#                 },
#                 {
#'type': 'number',
#'summary': 'Second data'
#                 }
#             ],
#'usage': 'sys.taskInit(function()...'
#         },
#         ...
#     ],
#     ...
# }

print("found %d files" % len(file_list))

modules = {}
for file in file_list:
    text = ""
    try:
        f = io.open(file,"r",encoding="utf-8")
        text = f.read()
        f.close()
    except:
        #print("read %s fail, maybe not use utf8" % file)
        continue

    #The following data
    lines = text.splitlines()
    line_now = 0
    while line_now<len(lines)-3:
        #Match api full name line
        name = re.search(r" *@sys_pub *(.+) *",lines[line_now+1],re.I)
        if lines[line_now].startswith("/*") and name:
            mod = name.group(1)#Modules name
            api = {}
            api["topic"] = re.search(r" *(.+) *",lines[line_now+3],re.I).group(1)
            api["summary"] = re.search(r" *(.+) *",lines[line_now+2],re.I).group(1)
            line_now += 4
            api["return"] = []
            api["usage"] = ""
            arg_re = r" *@([^ ]+) +(.+) *"
            usage_re = r" *@usage *"
            #Match return parameters
            while True:
                arg = re.search(arg_re,lines[line_now],re.I)
                arg_return = re.search(usage_re,lines[line_now],re.I)
                if arg and not arg_return:
                    api["return"].append({'type':arg.group(1),'summary':arg.group(2)})
                    line_now+=1
                else:
                    break
            #Match usage example
            while True:
                arg = re.search(usage_re,lines[line_now],re.I)
                if arg:
                    line_now+=1
                    while lines[line_now].find("*/") < 0:
                        api["usage"] += lines[line_now]+"\n"
                        line_now+=1
                else:
                    line_now+=2
                    break
            if not (mod in modules):
                modules[mod] = []
            modules[mod].append(api)
        else:
            line_now += 1

################## Interface data extraction completed ##################
try:
    os.remove("../../luatos-wiki/api/sys_pub.md")
except:
    pass

doc = open("../../luatos-wiki/api/sys_pub.md", "w+",encoding='utf-8')
doc.write("#📮 sys system message\n")
doc.write("\n\n")
doc.write("此处列举了LuatOS框架中自带的系统消息列表\n\n")
doc.write("\n\n")

for _, (name,module) in enumerate(modules.items()):
    doc.write("## "+name+"\n\n")
    doc.write("\n\n")
    doc.write("["+name+"接口文档页](https://wiki.luatos.com/api/"+name+".html)\n\n")
    doc.write("\n\n")
    for pub in module:
        doc.write("### "+pub["topic"]+"\n\n")
        doc.write(pub["summary"]+"\n\n")

        doc.write("**额外返回参数**\n\n")
        if len(pub["return"]) > 0:
            doc.write("|返回参数类型|解释|\n|-|-|\n")
            for arg in pub["return"]:
                doc.write("|"+arg["type"].replace("|","\|")+"|"+arg["summary"].replace("|","\|")+"|\n")
            doc.write("\n")
        else:
            doc.write("无\n\n")

        doc.write("**例子**\n\n")
        if len(pub["usage"]) == 0:
            doc.write("无\n\n")
        else:
            doc.write("```lua\n"+pub["usage"]+"\n```\n\n")

        doc.write("---\n\n")

doc.close()
