/* serial.h: methods used to configure serial port

Copyright © 2015 Michael Zhu <boot2linux@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
must display the following acknowledgement:
This product includes software developed by the Tea.
4. Neither the name of the Tea nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Michael Zhu <boot2linux@gmail.com> ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Michael Zhu <boot2linux@gmail.com> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <termios.h>
#include <sys/queue.h>

struct serial
{

    struct termios attr;

    int fd;
    char name[50];
    SLIST_ENTRY(serial)
    node;
};

enum ser_parity
{

    SER_PARITY_NONE = 0,
    SER_PARITY_ODD = 1,
    SER_PARITY_EVEN = 2,
};

enum ser_flow_ctrl
{
    SER_FLOW_NONE = 0,
    SER_FLOW_XON,
};

/*
 * scan serial port
 *
 * it must be invoked before other xxx_serial functions
 *
 * @return: the number of serial port
 */
int
scan_serial(void);

/* remove @ser from list built by scan_serial, and free it */
void
delete_serial(struct serial* ser);

/*
 * open serial and load default value
 * return success(0) or error code
 */
int
open_serial(char* name, struct serial** ser);

/*
 * open one idle serial port
 * return success(0) or error code
 */
int
open_one_idle_serial(struct serial** ser);

void
close_serial(struct serial* ser);

void
close_all_serials(void);

/* iterating serial port
 *
 * cb: callback function. stop iterating if it return 1
 */
void
iterate_serial_port(int (*cb)(struct serial* ser, void* data), void* data);

/* translate literal baudrate to speed_t
 * @baudrate: literal baudrate,e.g. 115200
 */
speed_t
baudrate_to_speed(int baudrate);

/* translate speed_t to literal baudrate */
int
speed_to_baudrate(speed_t speed);

/* update speed, but not apply to serial port */
int
serial_setup_speed(struct serial* ser, speed_t speed);

/* change number of data bits */
int
serial_setup_csize(struct serial* ser, int number);

/* change number of stopbits */
int
serial_setup_stopbits(struct serial* ser, int number);

/* configure parity type */
int
serial_setup_parity(struct serial* ser, enum ser_parity p);

/* set flow control type */
int
serial_setup_flowctrl(struct serial* ser, enum ser_flow_ctrl flow);

/* apply termios identified by ser->attr to serial port */
int
serial_apply_termios(struct serial* ser);

#endif
