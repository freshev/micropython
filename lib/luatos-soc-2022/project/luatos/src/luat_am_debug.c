#include "bsp_common.h"
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_fota.h"
#include "luat_malloc.h"
#include "luat_debug.h"
#include "cms_def.h"

void luat_shell_push(char* uart_buff, size_t rcount);

#if 1

#define LUAT_LOG_TAG "sc"
#include "luat_log.h"

#define CMD_BUFF_SIZE (4*1024 + 8)

// Take over the input data of the USB virtual log port to realize the boot-free scripting
// Requires LuaTools 2.1.94 or above

/*In csdk, the following commands have been built in
uint8_t fast_ack_cmd[4] = {0x7e, 0x00, 0x00, 0x7e};
uint8_t fast_reboot_cmd[4] = {0x7e, 0x00, 0x01, 0x7e};

According to the protocol document, here we temporarily define 3 flashing related commands.
1. Initialize data transmission {0x7e, 0x00, 0x30, 0x7e}
2. Block data transmission {0x7e, 0x00, 0x31, 0x7e}, followed by 4k fota data
3. End data transmission {0x7e, 0x00, 0x32, 0x7e}

After the transfer is completed, a restart command needs to be sent*/

// 1. Initialization command, no subsequent data
static const uint8_t cmd_fota_init[] = {0x7e, 0x00, 0x30, 0x7e};
// 2. Data transfer command, followed by an unsigned length of 1 byte, then the actual data, usually 128 bytes
static const uint8_t cmd_fota_data[] = {0x7e, 0x00, 0x31, 0x7e}; 
// 3. End command, no subsequent data
static const uint8_t cmd_fota_end[]  = {0x7e, 0x00, 0x32, 0x7e};
// Note: The last package of data and init will have additional processing and will take longer.

// The response to the command is >>>>$X FOTA $Y<<<<
// Among them: $X is OK, which represents success, and other values     represent failure.
// $Y corresponds to INIT WRITE DONE

//Other commands
// Log opening and closing, no subsequent data
static const uint8_t cmd_log_off[] = {0x7e, 0x00, 0x50, 0x7e};
static const uint8_t cmd_log_on[] =  {0x7e, 0x00, 0x51, 0x7e};

// Handle USB packetization, especially cmd_fota_data packetization processing
static uint8_t* tmpbuff;
static size_t buff_size; //Save the amount of data written in tmpbuff

// Write data directly to the soc log virtual serial port, bypassing the DBG processing logic
extern int32_t am_usb_direct_output(uint8_t atCid, uint8_t* atStr, uint16_t atStrLen);

// Convenient output response
static void usb_output_str(const char* data) {
    am_usb_direct_output(CMS_CHAN_USB, (uint8_t*)data, strlen(data));
}
// static void usb_output_raw(const char* data, size_t len) {
//     am_usb_direct_output(CMS_CHAN_USB, data, len);
// }

// Copy the function of the same name in csdk. The original function has a weak attribute and can be overridden.
void am_debug(uint8_t *data, uint32_t len) {
    int ret = 0;
    // LLOGD(">>>>>>>>>>> %d", len);
next:
    if (tmpbuff == NULL) { // A 1k buffer is required, but less than 200 bytes are actually used.
        tmpbuff = luat_heap_malloc(CMD_BUFF_SIZE);
    }
    if (tmpbuff == NULL) { // Memory exploded?
        usb_output_str(">>>>OUT OF MEMORY<<<<");
        return;
    }
    if (buff_size + len > CMD_BUFF_SIZE) { // Unlikely, but still needs defense
        usb_output_str(">>>>too many data, drop old data<<<<");
        buff_size = 0;
    }
    //Copy to local buff to handle subcontracting issues
    memcpy(tmpbuff + buff_size, data, len);
    buff_size += len;
    if (buff_size < 4)
        return; // Wait for more data
    len = 0;
    data = NULL; // Prevent misuse

    //Start judging the command

    // 1.Initialization command
    if (!memcmp(tmpbuff, cmd_fota_init, 4)) {
        luat_fota_init(0, 0, NULL, NULL, 0);
        buff_size -= 4;
        if (buff_size > 0) {
            // Theoretically, there is no follow-up data, so let’s defend ourselves.
            memmove(tmpbuff, tmpbuff + 4, buff_size);
        }
        // FOTA INIT always succeeds. TODO doesn’t seem to be the same, right??
        usb_output_str(">>>>OK FOTA INIT<<<<");
    }
    // 2. Data command
    else if (!memcmp(tmpbuff, cmd_fota_data, 4)) {
        #define DATA_HEAD_SIZE (6)
        if (buff_size < DATA_HEAD_SIZE) {
            // The data is not enough, wait for the next batch
            return;
        }
        size_t rlen = (tmpbuff[4] << 8) + tmpbuff[5]; // Change to 4k
        if (rlen > CMD_BUFF_SIZE - DATA_HEAD_SIZE) {
            usb_output_str(">>>>ERR FOTA WRITE<<<<");
            buff_size = 0;
            return;
        }
        if (rlen > buff_size - DATA_HEAD_SIZE) {
            return; // Wait for the next packet
        }
        //Write data to fota api
        ret = luat_fota_write(tmpbuff + DATA_HEAD_SIZE, rlen);
        if (ret < 0) {
            // Failed -_-
            usb_output_str(">>>>ERR FOTA WRITE<<<<");
            buff_size = 0;
            return;
        }
        else {
            // Very good and powerful, the writing was successful
            usb_output_str(">>>>OK FOTA WRITE<<<<");
        }
        // Relocate remaining data, if any
        buff_size -= rlen + DATA_HEAD_SIZE;
        if (buff_size > 0) {
            memmove(tmpbuff, tmpbuff + DATA_HEAD_SIZE, buff_size);
        }
    }
    // 3. End the command, and a restart command should be sent later.
    else if (!memcmp(tmpbuff, cmd_fota_end, 4)) {
        ret = luat_fota_end(0);
        if (0 == ret) {
            usb_output_str(">>>>OK FOTA DONE<<<<");
        }
        else {
            usb_output_str(">>>>ERR FOTA DONE<<<<");
        }
        // It's over, tmpbuff is released
        if (tmpbuff != NULL) {
            luat_heap_free(tmpbuff);
            tmpbuff = NULL;
            buff_size = 0;
        }
    }
    //Other commands
    else if (!memcmp(tmpbuff, cmd_log_off, 4)) {
        luat_debug_print_onoff(0);
        usb_output_str(">>>>OK LOG OFF<<<<");
        buff_size -= 4;
        if (buff_size > 0) {
            memmove(tmpbuff, tmpbuff + 4, buff_size);
        }
    }
    else if (!memcmp(tmpbuff, cmd_log_on, 4)) {
        luat_debug_print_onoff(1);
        usb_output_str(">>>>OK LOG ON<<<<");
        buff_size -= 4;
        if (buff_size > 0) {
            memmove(tmpbuff, tmpbuff + 4, buff_size);
        }
    }
    else {
        //Unrecognized commands will be treated as errors
        // TODO can be expanded with more commands later
        #ifdef LUAT_USE_REPL
        luat_shell_push(tmpbuff, buff_size);
        luat_heap_free(tmpbuff);
        tmpbuff = NULL;
        buff_size = 0;
        #else
        usb_output_str(">>>>ERR FOTA<<<<");
        #endif
        return;
    }
    // If there is remaining data, loop processing. It is unlikely to have -_-
    if (buff_size >= 4) {
        goto next;
    }
}

#endif
