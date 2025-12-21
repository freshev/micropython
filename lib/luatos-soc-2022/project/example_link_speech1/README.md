### Alibaba Cloud Qianli Sound Transmission Demonstration
## Note! ! !
1 This demo is only for demonstrating the Alibaba Cloud Qianli Voice Transmission function. It does not have good exception handling. Please do not use it for other purposes.
2 needs to be replaced with its own device triplet
3 The amount broadcast demonstration audio format of this demo is amr
4 This demo does not enable the tts function and only audio files are broadcast.

## Demo project
1 Corpus push and amount broadcast
2 Dynamic audio broadcast

## Demo preparation
# Corpus push and amount broadcast
First, issue the corpus required for the amount broadcast. Alibaba Cloud comes with common corpus. The file must be in amr format.
1 digital corpus
2 Quantity word package
3 Currency unit corpus

Secondly, the format delivered through Alibaba Cloud Qianli Transsion SpeechByCombination API is "{$number}", excluding double quotes.
where number is a string not greater than 99999999.99
For example, "{$10000.11}", the device will broadcast ten thousand points and one dollar.
https://help.aliyun.com/document_detail/223764.html

# Dynamic audio broadcast
Audio files are dynamically delivered through the Liyun Qianli Transsion SpeechBySynthesis API and played by the device.
https://help.aliyun.com/document_detail/369398.html



