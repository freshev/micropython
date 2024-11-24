# Play with the fun audio playback of Air780E

The Air780E development board has a built-in Shunxin 7149Audio Codec, which has relatively weak driving capabilities. In order for developers to drive a variety of speakers and have fun, we have prepared an audio PA expansion board. At the same time, we have prepared an online cloud speaker control platform - Hezhou Gadget http://tools.openluat.com/tools/yunlaba

![](C:\Users\WL\Desktop\Screenshot 2022-11-15 182029.png)

The format of data delivered by the cloud platform adopts GBK encoding.

0 indicates that the following is ordinary text, and the four bytes following 0 indicate the length of the text. 1 indicates audio, and the four bytes after 1 indicate the length of the url.

Preparation:

1. Hezhou Air780E development board

2. Hezhou Air780E development board speaker expansion board

3. One speaker

Overall hardware matching diagram:

<img src="C:\Users\WL\Desktop\WeChat Picture_20221115110941.jpg" alt="WeChat Picture_20221115110941" />

<img src="C:\Users\WL\Desktop\WeChat Picture_20221115110950.jpg" alt="WeChat Picture_20221115110950" />

Demonstration plan one: ordinary audio playback

1. Send text broadcast content through the Hezhou Gadget Cloud Platform (8,000 yuan received from Alipay):

2. Distribute audio + text broadcast through the Hezhou Gadget Cloud Platform (payment successful is 8,000 yuan)

Demonstration plan two: Set the TTS broadcast parameters, set the pronunciation style, speaker, etc., and experience a different and interesting TTS voice broadcast

TTS personalized setting format reference: web_audio project under the Gitee warehouse project file

                 

 					

 				 

Friends who like it, hurry up and try it.
