/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
