#ifndef __LUAT_YMODEM_H__
#define __LUAT_YMODEM_H__

#include "luat_base.h"

#include "sfud.h"

//save_path is the save folder path
//force_save_path forces the save file path, taking precedence over save_path
void *luat_ymodem_create_handler(const char *save_path, const char *force_save_path);
//sfud_offsetv is the flash offset address
void *luat_ymodem_create_handler_sfud(const sfud_flash* flash,size_t sfud_offset);
//Collect files
//Handshake phase, data is NULL, ack='c'
//In the data phase, if a whole packet is received, ack will return a success or failure symbol according to the parsing result. If it is incomplete, ack=0
//After the call is completed, if ack is not 0, ack needs to be sent out
//After the call is completed, if the flag is not 0, the flag needs to be sent out
//If return is not 0, NAK occurs
//After file reception is completed file_ok=1
//all_done=1 after receiving a stop frame or cancel frame
int luat_ymodem_receive(void *handler, uint8_t *data, uint32_t len, uint8_t *ack, uint8_t *flag, uint8_t *file_ok, uint8_t *all_done);

void luat_ymodem_reset(void *handler);
//After ymodem transfer is completed, call to save the file and release resources
void luat_ymodem_release(void *handler);
#endif
