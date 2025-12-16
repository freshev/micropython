Luat_CSDK_Air724U Introduction
============

## 1. Introduction

>Luat_CSDK_Air724U is a set of C language software development environment prepared for the use of Hezhou Air724U (Zhanrui 8910 chip) Modules. 
It allows customers to use Hezhou's wireless communication Modules like developing a microcontroller



## 2. Software Architecture

![Software Architecture](https://images.gitee.com/uploads/images/2020/0707/090924_c101da41_1221708.png "luat_iot_sdk_arch.png")

## 3. Version differences

&emsp;&emsp;

| core version      | volte         | bt            | tts           | app available space | file system available space |
| ----------------- | ------        | ------        | ------        | -----------         | --------------              |
| csdk              | Not supported | Not supported | Not supported | 1536K               | 2432K                       |
| csdk_bt_tts       | Not supported | Support       | Support       | 1216K               | 1472K                       |
| csdk_bt_tts_volte | Support       | Support       | Support       | 900K                | 1152K                       |

Note: If the actual size of the user APP after compilation is close to the allocated APP free space, this will not be able to use the remote upgrade full package upgrade. You need to upgrade the core and the underlying layer separately, or put it together to use differential upgrades. This version of the firmware is in development stage and may be unstable. It is recommended to conduct sufficient testing before remote upgrade. To avoid irreparable losses.



## 4. Minimum system

```
//Minimum system
static VOID demo_task_main(PVOID pParameter)
{
	while(1)
	{
		iot_os_sleep(1000);
		iot_debug_print("demo_task_main");
	}
}

int appimg_enter(void *param)
{   
	iot_debug_print("appimg_enter");
iot_os_create_task(demo_task_main, NULL,
2048, 1, OPENAT_OS_CREATE_DEFAULT, "task");
return 0;
}

void appimg_exit(void)
{
    iot_debug_print("appimg_exit");
}
```

## 5. Firmware download tool

> [LuatTools](http://www.openluat.com/Product/file/luatoolsV2-redirect.html)

## 6. How to compile

>In command line mode, enter the code path. For example, if you need to compile the demo_socket project, go to project and run demo_socket.bat. After the compilation is successful, two pac files will be generated in the directory `hex\Air720U_CSDK_demo_socket\`: `Air720U_CSDK_demo_socket.pac` and `Air720U_CSDK_demo_socket_APP.pac`. Among them, `Air720U_CSDK_demo_socket.pac` contains the underlying and csdk layer firmware, and `Air720U_CSDK_demo_socket_APP.pac` contains only csdk layer firmware.

## 7. Support routines

All the compilations that support demos are placed in the project directory. You can compile the corresponding demo applications by running the corresponding bat.
| Routines | Description |
| :--------| :--: |
|demo_audio|Demonstration of functions such as recording, channel settings|
|demo_capture|Photography function demonstration|
|demo_datetime|System time settings|
|demo_fota|Remote Upgrade Example|
|demo_fs|File system operation example|
|demo_ftp|FTP function example|
|demo_http|HTTP function example|
|demo_mqtt|mqtt function example|
|demo_socket|Socket Interface Example|
|demo_ssl|ssl encrypted socket example|
|demo_tts|Voice broadcast example|
|demo_wifiloc|wifi location example|
|demo_zbar|QR code scanning example|
|demo_lvgl|littleVGL UI framework example|

## 8. Official application
> [lua parser](app/elua/LUA parser open source documentation.md)
