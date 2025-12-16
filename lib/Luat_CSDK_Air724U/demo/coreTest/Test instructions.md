# CSDK development coreTest instructions

The initial version has the following test tasks built-in, and fortest calls the test function through the function pointer. The first parameter is the called function, the second parameter is the number of times the called function needs to be called, and the third function is how long the two calls are separated. The subsequent indefinite length parameters are parameters that need to be passed in by the called function.

```c
int appimg_enter(void *param)
{
	networkTest();
extern bool networkstatus;
while (networkstatus == FALSE)
		iot_os_sleep(500);
// Turn off the watchdog, the crash will not restart. Open by default
	iot_debug_set_fault_mode(OPENAT_FAULT_HANG);
//Open debug information, close by default
iot_vat_send_cmd("AT^TRACECTRL=0,1,3\r\n", sizeof("AT^TRACECTRL=0,1,3\r\n"));
	iot_debug_print("[hello]appimg_enter");
Fastest (Zbartest, 1, 1000);
Festest (Adctest, 1, 500, Openat_adc_2);
Festest (Adctest, 1, 500, Openat_adc_3);
Fastest (Flasht test, 1, 500, 0x320000, 0x330000);
Fastest (Data Time Test, 1, 500);
fortest(fsTest, 1, 500, "/fs.test"); //There is a problem, the file deletion is not successful

fortest(ftpTest, 1, 1000); //Repeat test will crash, as if the file has not been deleted. Restart is not possible, the file system must be erased before retesting
Fastest (GSMLoctest, 1, 1000);
fortest(httpTest, 1, 1000);//There is an error message, I can't understand
Fastest (Pwdtest, 1, 1000);
Fastest (Rilest, 1, 1000);

Fastest (Sockettest, 1, 1000, "DNS");
Festest (socket test, 1, 1000, "UDP");
fortest(socketTest, 1, 1000, "TCP");
Fastest (vest, 1, 1000);
return 0;
}
```

After compiling and downloading, just search for coreTest in the trace of coolWatcher to see all the print information. Enter coreTest-False to see all the Moduless that have errors, find and debug.

As long as there is one error, it is abnormal. There is no error message, which means the test passes.







