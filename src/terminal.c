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
#include <string.h>
#include <errno.h>
#include "terminal.h"
#include "cli.h"


/* read  from serial port and then write to controlling tty */
static void
ser_read (struct aev_loop *loop, aev_io *w, int evmask)
{
    int len;
    char buf[1024];
    struct terminal *tm = w->data;

    len = read(tm->ser->fd, buf, sizeof buf);
    if( len <= 0) {
        aev_io_stop(loop, &tm->ser_w);
        aev_io_stop(loop, &tm->tty_w);
        disable_raw_mode(tm);
    }
    write(tm->ofd, buf, len);
}

/* read from controlling tty and then write to serial port */
static void
tty_read (struct aev_loop *loop, aev_io *w, int evmask)
{
    int len;
    unsigned char buf;
    struct terminal *tm = w->data;

    len = read(tm->ifd, &buf, 1);

    /* map DEL to Backspace */
    if (buf == 127)
        buf = 8;

     /* esc key: Ctrl-] */
    if ( buf == 29 ) {

        cli_loop(tm);
        return;
    }

    if (tm->ser)
        write(tm->ser->fd, &buf, 1);
}

struct terminal *
new_terminal(struct aev_loop *loop, struct serial *ser, int ifd, int ofd)
{
    struct terminal *tm;

    tm = malloc( sizeof *tm);

    tm->loop = loop;
    tm->ifd = ifd;
    tm->ofd = ofd;

    aev_io_init(&tm->tty_w, ifd, tty_read,  AEV_READ, tm);
    aev_io_start(loop, &tm->tty_w);

    if ( ser ) {

        tm->ser = ser;
        aev_io_init(&tm->ser_w, ser->fd, ser_read,  AEV_READ, tm);
        aev_io_start(loop, &tm->ser_w);

    } else {
        tm->ser = NULL;
    }

    enable_raw_mode(tm);

    return tm;
}

void terminal_connect_serial(struct terminal *tm, char *name){
    struct serial *ser = NULL;
    int ret;

    ret = open_serial(name, &ser);
    if ( ret < 0 )
    {
        switch (ret) {
            case -ENOENT:
                fprintf(stderr, "No serial port!\n");
                break;
            case -EBUSY:
                fprintf(stderr, "Serial ports are busy!\n");
                break;
            default:
                fprintf(stderr, "%s\n", strerror(errno));
                break;
        }
        return;
    }

    if (tm->ser) {
        aev_io_stop(tm->loop, &tm->ser_w);
    }

    tm->ser = ser;
    aev_io_init(&tm->ser_w, ser->fd, ser_read, AEV_READ, tm);
    aev_io_start(tm->loop, &tm->ser_w);
}

void
delete_terminal(struct terminal *tm)
{
    if(tm) free(tm);
}
