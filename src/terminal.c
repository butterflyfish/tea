/* terminal.c: delivery char between user input and serial port
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include "terminal.h"

static struct ev_loop *loop;

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/* read  from serial port and then write to controlling tty */
static void
ser_read (EV_P_ struct ev_io *w, int revents)
{
    int len;
    char buf[1024];
    struct terminal *tm = container_of(w, struct terminal, ser_w);

    len = read(tm->ser_fd, buf, sizeof buf);
    if( len <= 0) {
        ev_io_stop(EV_A_ &tm->ser_w);
        ev_io_stop(EV_A_ &tm->tty_w);
        disable_raw_mode(&tm->cli);
    }
    write(tm->ofd, buf, len);
}

/* read from controlling tty and then write to serial port */
static void
tty_read (EV_P_ struct ev_io *w, int revents)
{
    int len;
    unsigned char buf;
    struct terminal *tm = container_of(w, struct terminal, tty_w);

    len = read(tm->ifd, &buf, 1);

    /* map DEL to Backspace */
    if (buf == 127)
        buf = 8;

     /* esc key: Ctrl-] */
    if ( buf == 29 ) {

        ev_suspend(loop);
        cli_loop(&tm->cli);
        ev_resume(loop);
        return;
    }

    write(tm->ser_fd, &buf, 1);
}

struct terminal *
new_terminal(int ser_fd, int ifd, int ofd)
{
    struct terminal *tm;

    tm = malloc( sizeof *tm);
    tm->ser_fd = ser_fd;
    tm->ifd = ifd;
    tm->ofd = ofd;

    ev_io_init(&tm->ser_w, ser_read, ser_fd, EV_READ);
    ev_io_init(&tm->tty_w, tty_read, ifd, EV_READ);

    ev_io_start(loop, &tm->ser_w);
    ev_io_start(loop, &tm->tty_w);

    tm->cli.ifd = ifd;
    tm->cli.ofd = ofd;
    tm->cli.ser_fd = ser_fd;
    enable_raw_mode(&tm->cli);

    fcntl(tm->ifd, F_SETFL, fcntl(tm->ifd, F_GETFL, 0)|O_NONBLOCK);

    return tm;
}

void
delete_terminal(struct terminal *tm)
{
    if(tm) free(tm);
}

void
xfer_init(void)
{
    loop = EV_DEFAULT;
}

void
xfer_loop()
{
    ev_run (loop, 0);

    /* break was called, so exit */
    return;
}
