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

struct serial {

    struct termios attr;

    int fd;
    char path[100];
    SLIST_ENTRY(serial) node;
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


/*
 * open serial and load default value
 * return success(0) or error code
 */
int
open_serial(char *name, struct serial **ser);

/*
 * open one idle serial port
 * return success(0) or error code
 */
int
open_one_idle_serial( struct serial **ser );

int
close_serial(int fd);

void
close_all_serials(void);

/*
 * show setup information of serial port
 * its output like utility stty
 *
 * @fd: where to write setup info
 * @ser: point to serial port
 */
void
show_serial_setup(struct serial *ser, int fd);

/* translate literal baudrate to speed_t
 * @baudrate: literal baudrate,e.g. 115200
 */
speed_t
baudrate_to_speed(int baudrate);

/* update speed, but not apply to serial port */
int
serial_setup_speed(struct serial *ser, speed_t speed);

/* apply termios identified by ser->attr to serial port */
int
serial_apply_termios(struct serial *ser);

#endif

