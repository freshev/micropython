#ifndef LUAT_UART_LEGACY_H
#define LUAT_UART_LEGACY_H

#include "luat_base.h"

#ifdef __LUATOS__
int l_uart_handler(lua_State *L, void* ptr);
#endif

#ifdef LUAT_FORCE_WIN32
int luat_uart_list(uint8_t* list, size_t buff_len);
#endif

int luat_setup_cb(int uartid, int received, int sent);
/*Logic for reporting receiving data interruption:
1. When the serial port is initialized, create a new buffer
2. You can consider applying for a buffer length of several hundred bytes for users to prevent packet loss during user processing.
3. Each time the serial port receives data, it is first stored in the buffer and the length is recorded.
4. When encountering the following situations, call the serial port interrupt again
    a) The buffer is full (when the user applies for more)/there are only a few hundred bytes left in the buffer (when the buffer is requested based on the actual length)
    b) Received fifo receiving timeout interrupt (the serial port data should not continue to be received at this time)
5. When triggering the data received interrupt, the returned data should be the data in the buffer.
6. Release buffer resources when closing the serial port*/

#endif
