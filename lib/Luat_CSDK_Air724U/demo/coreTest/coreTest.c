/***************
	demo_hello
****************/

#include "iot_debug.h"
#include "iot_os.h"

#define fortest(fun, num, ms, ...) \
	for (char i = 0; i < num; i++) \
	{                              \
		fun(__VA_ARGS__);          \
		iot_os_sleep(ms);          \
	}

extern bool adcTest(E_AMOPENAT_ADC_CHANNEL channel);
extern void flashTest(UINT32 begain_addr, UINT32 end_addr);
extern void datetimeTest(void);
extern bool fsTest(char *file);
extern void networkTest(void);
extern bool ftpTest(void);
extern bool gsmlocTest(void);
extern void httpTest(void);
extern void pwdTest(void);
extern void RilTest(void);
extern void socketTest(char *mode);
extern void vatTest(void);
extern void zbarTest(void);
extern bool ttsTest(char *text, u32 len);

int appimg_enter(void *param)
{
	pwdTest();
	networkTest();
	extern bool networkstatus;
	while (networkstatus == FALSE)
	{
		iot_debug_print("[hello]coreTest wait network");
		iot_os_sleep(500);
	}
	// ¹
	iot_debug_set_fault_mode(OPENAT_FAULT_HANG);
	//´ò¿ªµ÷ÊÔÐÅÏ¢£¬Ä¬ÈÏ¹Ø±Õ
	iot_vat_send_cmd("AT^TRACECTRL=0,1,1\r\n", sizeof("AT^TRACECTRL=0,1,1\r\n"));
	iot_debug_print("[hello]coreTest appimg_enter");
	while (1)
	{
		fortest(zbarTest, 1, 200);
		fortest(adcTest, 1, 200, OPENAT_ADC_2);
		fortest(adcTest, 1, 200, OPENAT_ADC_3);
		fortest(flashTest, 1, 200, 0x360000, 0x370000);
		fortest(datetimeTest, 1, 200);
		fortest(fsTest, 1, 200, "/fs.test");

		// Fastest (ftptest, 0, 1000); // Öøø´²âteô ± Øëà »ú

		//fortest(gsmlocTest, 1, 200);
		// Fastest (httptest, 1, 200); // Óð´nîóðåï ¢
		//fortest(pwdTest, 1, 200);
		fortest(RilTest, 1, 200);

		fortest(socketTest, 1, 200, "DNS");
		fortest(socketTest, 1, 200, "UDP");
		fortest(socketTest, 1, 200, "TCP");
		fortest(vatTest, 1, 200);
		fortest(ttsTest, 1, 200, "1", sizeof("1"));
	}
	return 0;
}

void appimg_exit(void)
{
	iot_debug_print("[hello]appimg_exit");
}
