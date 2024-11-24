# Hezhou Air780E TTS parameter setting format (TTS that can change the voice)

1. Original format (select default parameters)

```c
char str[] = "Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

2.Set the speaker

Set the code reference. Only one speaker can be set at the same time.

```c
Format: [m*] (*=51~55)
51 – a long time
52 – Many
53 – Xiaoping
54 – Donald Duck
55 – Xu Baobao
Select many (52) as the speaker
char str[] = "[m52]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

3. Set pronunciation style

```c
Format: [f*] (*=0/1/2)
Parameters: 0 - word by word
1 – Tell the story straight
2 – Colorful
Note: The default is straightforward narrative style.
char str[] = "[f1]Hello everyone, I am Xiao Zhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
 
```

4. Select language

```c
Format: [g*] (*=0/1/2)
Parameters: 0 – automatic judgment
1 – Mandarin Chinese
2 – English language
3 – French
Note: The default language is automatic judgment.
char str[] = "[g0]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface

```

5. Set up a number crunching strategy

```c
Format: [n*] (*=0/1/2)
Parameters: 0 – automatic judgment
1 – Number processing
2 – Numerical processing of numbers
Note: The default is automatic judgment.
char str[] = "[n1]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

6. Reading setting of English number 0

```c
Format: [o*] (*=0/1)
Parameters: 0 – English digits 0 is pronounced as “O”
1 – The English numeral 0 is pronounced “zero”
Note: The default is the English number 0, which is pronounced "zero".
Note: The mark will only take effect when 0 is read aloud as a number. When 0 is processed as a numerical value, it will always be read as zero.
char str[] = "[o1]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

7. Silence for a period of time

```c
Format: [p*] (*=unsigned integer)
Parameters: * – The length of silence, unit: milliseconds (ms)
char str[] = "[p2000]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

8.Set speaking speed

```c
Format: [s*] (*=0~10)
Parameters: * – 0-10
Note: The default speaking speed value is 5, and the speaking speed adjustment range is half to twice the default speaking speed, that is, a value of 0 is half slower than the default speaking speed, and a value of 10 is twice as fast as the default speaking speed.
char str[] = "[s5]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

9. Set the tone

```c
Format: [t*] (*=0~10)
Parameters: * - The intonation value corresponds to the parameter setting value of 6553* (value -5), that is, 0 corresponds to -32765, 5 corresponds to 0, and 10 corresponds to +32765
Note: The default intonation value is 5, and the intonation adjustment range is from 64Hz below the default intonation fundamental frequency to 128Hz above.
char str[] = "[t5]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

10.Set the volume

```c
Format: [v*] (*=0~10)
Parameters: * - The volume value corresponds to the parameter setting value of 6553* (value -5), that is, 0 corresponds to -32765, 5 corresponds to 0, and 10 corresponds to +32765.
Note: The volume adjustment range is from mute to the maximum value supported by the audio device. The default value 5 is the middle volume.
char str[] = "[v1]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

11.Set the pronunciation of "1" in Chinese numbers

```c
Format: [y*] (*=0/1)
Parameters: 0 - "1" is read as "yāo" when combining numbers
1 – “1” is pronounced as “yī” when combining numbers
Note: By default, "1" is read as "yāo" when synthesizing numbers.
char str[] = "[y1]Hello everyone, I am Xiaozhou";
luat_audio_play_tts_text(0, str, sizeof(str));//audio broadcast TTS interface
```

