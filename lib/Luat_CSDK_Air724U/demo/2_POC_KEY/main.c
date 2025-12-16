/***************
	demo_hello
****************/
#include <string.h>
#include "iot_debug.h"
#include "iot_os.h"

#include "am_openat_drv.h"

#include "iot_vat.h"

#include "iot_keypad.h"

#define KEYCODE_PTT 2 * 10 + 1
#define KEYCODE_POC 1 * 10 + 0
#define KEYCODE_VOLP 2 * 10 + 0
#define KEYCODE_VOLM 1 * 10 + 1
#define KEYCODE_OK 3 * 10 + 2
#define KEYCODE_UP 3 * 10 + 3
#define KEYCODE_DOWN 4 * 10 + 2
#define KEYCODE_EXIT 255 * 10 + 255

VOID keypad_cb(T_AMOPENAT_KEYPAD_MESSAGE *pKeypadMessage)
{
    iot_debug_print("[poc-key] pKeypadMessage->bPressed :%d", pKeypadMessage->bPressed);
    iot_debug_print("[poc-key] pKeypadMessage->data.matrix.r :%d", pKeypadMessage->data.matrix.r);
    iot_debug_print("[poc-key] pKeypadMessage->data.matrix.c :%d", pKeypadMessage->data.matrix.c);
}

int appimg_enter(void *param)
{
    iot_debug_print("[hello]appimg_enter");
    // Turn off the watchdog, the crash will not restart. Open by default
    iot_debug_set_fault_mode(OPENAT_FAULT_HANG);
    iot_os_sleep(100);
    //Set keypad, line 3 and column 2 are physically forced download keys (OK keys). Pressing this button when powering on will enter download mode
    //The poc project does not Introduction the boot key. You must call the keypad once to enter the download mode, otherwise you can only dismantle the machine after turning into bricks.
    iot_vat_send_cmd("AT*DOWNLOAD=2,3,2\r\n", sizeof("AT*DOWNLOAD=2,3,2\r\n"));
    iot_os_sleep(100);
    //Open debug information, close by default
    iot_vat_send_cmd("AT^TRACECTRL=0,1,2\r\n", sizeof("AT^TRACECTRL=0,1,2\r\n"));
    iot_os_sleep(100);

    T_AMOPENAT_KEYPAD_CONFIG keypadConfig = {0};
    //pConfig.config=
    keypadConfig.type = OPENAT_KEYPAD_TYPE_MATRIX;
    keypadConfig.pKeypadMessageCallback = keypad_cb;
    keypadConfig.config.matrix.keyInMask = 0x1f;
    keypadConfig.config.matrix.keyOutMask = 0x1f;
    BOOL err = iot_keypad_init(&keypadConfig);
    if (!err)
        iot_debug_print("[poc-key] iot_keypad_init false!");

    return 0;
}

void appimg_exit(void)
{
    iot_debug_print("[poc-key] appimg_exit");
}
