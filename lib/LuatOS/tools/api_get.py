import re
import io
import os

def get_file_list(paths, ext = ".c"):
    file_list = []
    for path in paths:
        for home, _, files in os.walk(path):
            for filename in files:
                if filename.endswith(ext):
                    file_list.append(os.path.join(home, filename))
    return file_list

#Comment format:
# /*
#@Modules Modules calling name
#@summary A short description of the Modules
#@version version number, optional
#@data date, optional
# */
# /*
#@api/function Modules.function (the complete function name used when calling)
#@string The first parameter, @ followed by the parameter type, followed by a space followed by the parameter explanation
#@number[opt=nil] The second parameter, the default value is nil
#@table[opt={}] The third parameter, the default value is {}
#...according to actual conditions, list all parameters
#@return type The first value returned, here is the explanation
#@return string The second value returned, type is string
#...According to actual, all return values     at the column
#@demo demo path
#@video video link
# @usage
#--Examples of use, can be multiple lines
#lcoal a,b,c = Modules.function("test",nil,{1,2,3})
# */
# static int l_module_function(lua_State *L) {
#//A bunch of code
# }
#
#//@const NONE number no check
########################################################
#Data structure:
#Moduless = [
#     {
#'Modules': 'adc',
#'summary': 'Digital-to-analog conversion',
#'url': 'https://xxxxxx',
#'demo': 'adc',
#'video': 'https://xxxxx',
#'usage': '--xxxxxxx',
#'tag': 'tag in bsp',
#'const': [
# {
#    'var':'uart.NONE',
#    'type':'number',
#'summary':'No verification',
# },
# ],
#         'api':[
#             {
#                 'api':'adc.read(id)',
#'summary': 'Read adc channel',
#'tag': 'tag in bsp',
#'args': [
#                     {
#'type': 'int',
#'summary': 'Channel id, related to the specific device, usually starts from 0'
#                     }
#                 ],
#'return': [
#                     {
#'type': 'int',
#'summary': 'original value'
#                     },
#                     {
#'type': 'int',
#'summary': 'calculated value'
#                     }
#                 ],
#'usage': '-- Open adc channel 2 and read\nif adc.open(2) then...'
#             },
#         ]
#     }
# ]
def get_modules(file_list, start="/*", end="*/"):
    modules = []
    for file in file_list:
        text = ""
        try:
            f = io.open(file,"r",encoding="utf-8")
            text = f.read()
            text = text.replace("\t","    ")
            f.close()
        except:
            #print("read fail, maybe not use utf8")
            continue

        module = {}

        file = file.replace("\\","/")
        if file.rfind("luat/") >= 0:
            file = file[file.rfind("luat/"):]
            module["url"] = "https://gitee.com/openLuat/LuatOS/tree/master/"+file
        else:
            module["url"] = ""


        #Annotation header
        r = re.search(re.escape(start) + r" *\n *@module *(\w+)\n *@summary *(.+)\n",text,re.I|re.M)
        if r:
            module["module"] = r.group(1)
            module["summary"] = r.group(2)
            module["usage"] = ""
            module["demo"] = ""
            module["video"] = ""
            module["tag"] = ""
            module["api"] = []
            module["const"] = []
        else:
            continue

        for mstep in range(len(modules)-1,-1,-1):
            if modules[mstep]["module"] == module["module"]:
                module = modules[mstep]
                del modules[mstep]
                module["url"] = ""

        #The following data
        lines = text.splitlines()
        line_now = 0
        isGotApi = False #Do you already have an interface? Or has the first comment ended?
        while line_now<len(lines)-3:
            if lines[line_now].find(end) >= 0:
                isGotApi = True #The first paragraph of comments is over, no need to look for examples.
            if not isGotApi:#Examples provided with the library
                if re.search(" *@demo *.+",lines[line_now],re.I):
                    module["demo"] = "https://gitee.com/openLuat/LuatOS/tree/master/demo/"
                    module["demo"] += re.search(" *@demo * (.+) *",lines[line_now],re.I).group(1)
                    line_now+=1
                    continue
                if re.search(" *@video *.+",lines[line_now],re.I):
                    module["video"] = re.search(" *@video * (.+) *",lines[line_now],re.I).group(1)
                    line_now+=1
                    continue
                if re.search(" *@tag *.+",lines[line_now],re.I):
                    module["tag"] = re.search(" *@tag * (.+) *",lines[line_now],re.I).group(1)
                    line_now+=1
                    continue
                if re.search(" *@usage *",lines[line_now],re.I):
                    line_now+=1
                    while lines[line_now].find(end) < 0:
                        module["usage"] += lines[line_now]+"\n"
                        line_now+=1
                    isGotApi = True
                    continue
            #Match api full name line
            name = re.search(r" *@api *(.+) *",lines[line_now+2],re.I)
            if not name:
                name = re.search(r" *@function *(.+) *",lines[line_now+2],re.I)
            #Match constant
            const_re = re.search(r"[ \-]*//@const +(.+?) +(.+?) +(.+)",lines[line_now],re.I)
            if const_re:
                const = {}
                const["var"] = module["module"]+"."+const_re.group(1)
                const["type"] = const_re.group(2)
                const["summary"] = const_re.group(3)
                module["const"].append(const)
            if lines[line_now].startswith(start) and name:
                api = {}
                api["api"] = name.group(1)
                api["summary"] = re.search(r" *(.+) *",lines[line_now+1],re.I).group(1)
                api["tag"] = ""
                line_now += 3
                api["args"] = []
                api["return"] = []
                api["usage"] = ""
                if re.search(" *@tag *.+",lines[line_now],re.I):
                    api["tag"] = re.search(" *@tag * (.+) *",lines[line_now],re.I).group(1)
                    line_now+=1
                arg_re = r" *@([^ ]+) +(.+) *"
                return_re = r" *@return *([^ ]+) +(.+) *"
                isGotApi = True
                #Match input parameters
                while True:
                    arg = re.search(arg_re,lines[line_now],re.I)
                    arg_return = re.search(return_re,lines[line_now],re.I)
                    if arg and not arg_return:
                        api["args"].append({'type':arg.group(1),'summary':arg.group(2)})
                        line_now+=1
                    else:
                        break
                #Match return value
                while True:
                    arg = re.search(return_re,lines[line_now],re.I)
                    if arg:
                        api["return"].append({'type':arg.group(1),'summary':arg.group(2)})
                        line_now+=1
                    else:
                        break
                #Match usage example
                while True:
                    arg = re.search(" *@usage *",lines[line_now],re.I)
                    if arg:
                        line_now+=1
                        while lines[line_now].find(end) < 0:
                            api["usage"] += lines[line_now]+"\n"
                            line_now+=1
                    else:
                        line_now+=2
                        break
                module["api"].append(api)
            else:
                line_now += 1
        #Packages without api are not imported.
        if len(module["api"]) > 0:
            modules.append(module)
            #print(module["module"])
        else:
            print("没有API", module["module"])

    #Sort by name
    def sorfFnc(k):
        return k["module"]
    modules.sort(key=sorfFnc)
    return modules
