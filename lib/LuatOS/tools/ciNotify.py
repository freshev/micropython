import sys
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
from git import Repo
import urllib
import requests
import http.cookiejar
import json

#$MQTTADDR $MQTTPORT $MQTTTOPIC "https://xxxx" "air101" DD_APPKEY DD_APPSECRET DD_NOTIFY_LIST DD_API_TOKEN DD_API_SEND
#1      2           3           4             5         6       7               8               9           10

repo = Repo("../")

#Temporarily disabled
############# MQTT ###############
#client = mqtt.Client()
# try:
##Please modify the server by yourself, you need to pass in parameters
#client.connect(sys.argv[1], int(sys.argv[2]), 60)
##topic Please modify it according to your needs. You need to pass in parameters.
#info = sys.argv[4]+"\r\n"+str(repo.head.commit.author)+"-"+str(repo.head.commit.message)
#if len(sys.argv) >= 6:
#repo = Repo("../../"+sys.argv[5])
#info = info+"\r\nSub-repository"+sys.argv[5]+"Last commit:\r\n"+str(repo.head.commit.author)+"-"+str(repo.head. commit.message)
#pub = client.publish(sys.argv[3],info)
#     pub.wait_for_publish()
#     client.disconnect()
#     print("sent")
#except Exception as e:
#     print(e)


###############DingTalk Reminder######################

dd_appkey = sys.argv[6]
dd_appsecret = sys.argv[7]
dd_list = sys.argv[8].split(",")
dd_api_token = sys.argv[9]
dd_api_send = sys.argv[10]
try:
    headers = {'user-agent': '114514'}
    token = requests.post(dd_api_token,json={"appKey":dd_appkey,"appSecret":dd_appsecret},headers=headers).json()["accessToken"]
    #send message
    headers = {'user-agent': '114514', 'x-acs-dingtalk-access-token': token}
    r = requests.post(dd_api_send,json={
            "robotCode":dd_appkey,
            "userIds":dd_list,
            "msgKey" : "sampleLink",
            "msgParam" : json.dumps({
                "title": sys.argv[5]+"的编译炸了",
                "text": "最后提交："+str(repo.head.commit.author)+"\r\n"+str(repo.head.commit.message),
                "messageUrl": sys.argv[4],
                "picUrl": "https://www.luatos.com/img/footer-logo.png",
            })
        },headers=headers)

    print(r.json())

except Exception as e:
    print(e)
