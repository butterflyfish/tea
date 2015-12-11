/* terminal.h: delivery char between user input and serial port
 *
opyright © 2015 Michael Zhu <boot2linux@gmail.com>
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

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "aev.h"
#include "serial.h"
#include "tea.h"

#define TERMINAL_BUF_SIZE  512

/* receive from user input and write to serial port */
typedef void (*aio_recv_t)(struct aev_loop *loop, aev_io *w, int evmask);

struct terminal {

    unsigned char buf[TERMINAL_BUF_SIZE + 1];
    int len;  /* length of data in buffer */

    struct serial *ser;
    aev_io ser_w;   /* serial port watcher */

    int ifd, ofd;     /* reader/writer fd representing terminal */
    aev_io term_w;   /* user reader watcher */

    int telnet; /* telnet state */
    int cli; /* cli state */

    aio_recv_t aio_recv; /* receive handler */

    struct aev_loop *loop;
};

/* read from controlling tty and then write to serial port */
void
tty_read (struct aev_loop *loop, aev_io *w, int evmask);

/* write data in buffer tm->buf into serial port */
int
terminal_write_serial(struct terminal *tm);

/* create a new terminal */
struct terminal *
new_terminal(tea_t *tea, char *name, int ifd, int ofd, aio_recv_t aio_recv);

/* connect new serial */
int
terminal_connect_serial(struct terminal *tm, char *name);

/* stop terminal and free it */
void
delete_terminal(struct terminal *tm);

void
terminal_print(struct terminal *tm, const char *fmt, ...);

/*
 * show setup information of serial port attached on terminal @tm
 * its output like utility stty
 */
void
terminal_show_serial_setting(struct terminal *tm);

#endif
