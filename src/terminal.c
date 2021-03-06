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
#include <stdarg.h>
#include "terminal.h"
#include "cli.h"
#include "tea.h"

/* read  from serial port and then write to controlling tty */
static void
ser_read(struct aev_loop* loop, aev_io* w, int evmask)
{
    int len;
    char buf[1024];
    struct terminal* tm = w->data;
    struct serial* ser;

    len = read(tm->ser->fd, buf, sizeof buf);
    if (len <= 0) {

        disable_raw_mode(tm);

        /* backup ser before delete terminal */
        ser = tm->ser;
        delete_terminal(tm);

        delete_serial(ser);
        return;
    }
    write(tm->ofd, buf, len);
}

int
terminal_write_serial(struct terminal* tm)
{
    int ret;

    if (!tm->ser)
        return 0;

    /* TODO: handle write error */
    ret = write(tm->ser->fd, tm->buf, tm->len);
    tm->len = 0;
    return ret;
}

/* read from controlling tty and then write to serial port */
void
tty_read(struct aev_loop* loop, aev_io* w, int evmask)
{
    int len;
    int ret;
    unsigned char byte;
    struct terminal* tm = w->data;

    len = read(tm->ifd, &byte, 1);
    if (len <= 0) {
        delete_terminal(tm);
        return;
    }

    /* map DEL to Backspace */
    if (byte == 127)
        byte = 8;

    tm->buf[tm->len++] = byte;
    ret = cli_process(tm);
    if (ret) {
        if (ret < 0)
            delete_terminal(tm);
        return;
    }

    terminal_write_serial(tm);
}

struct terminal*
new_terminal(tea_t* tea, char* name, int ifd, int ofd, aio_recv_t aio_recv)
{
    struct terminal* tm;

    tm = malloc(sizeof *tm);
    if (NULL == tm) {
        fprintf(stderr, "Failed to allocate memory for terminal\n");
        return NULL;
    }

    memset(tm, 0, sizeof(struct terminal));
    tm->loop = &tea->loop;
    tm->ifd = ifd;
    tm->ofd = ofd;
    tm->tea = tea;

    if (terminal_connect_serial(tm, name)) {
        close(ifd);
        free(tm);
        return NULL;
    }

    aev_io_init(&tm->term_w, ifd, aio_recv, AEV_READ, tm);
    aev_io_start(tm->loop, &tm->term_w);

    enable_raw_mode(tm);

    terminal_print(tm, "\033[1;31mEscape key of Tea is %s\033[0m\n", TEA_ESC_KEY_STR);

    return tm;
}

static void
terminal_setup_serial(struct terminal* tm)
{

    tea_t* tea = tm->tea;
    struct serial* ser = tm->ser;

    serial_setup_csize(ser, tea->cs);
    serial_setup_speed(ser, tea->speed);
    serial_setup_parity(ser, tea->p);
    serial_setup_flowctrl(ser, tea->flow);
    serial_setup_stopbits(ser, tea->stopbits);
    serial_apply_termios(ser);
}

int
terminal_connect_serial(struct terminal* tm, char* name)
{
    struct serial* ser = NULL;
    int ret;

    ret = name ? open_serial(name, &ser) : open_one_idle_serial(&ser);
    if (ret < 0) {
        switch (ret) {
            case -ENOENT:
                terminal_print(tm, "\033[1;31mNo serial port!\033[0m\n");
                break;
            case -EBUSY:
                terminal_print(tm, "\033[1;31mSerial ports are busy!\033[0m\n");
                break;
            default:
                terminal_print(tm, "\033[1;31mFailed to open serial -- %s\033[0m\n", strerror(errno));
                break;
        }
        return -1;
    }

    terminal_print(tm, "Serial port %s is connected\n", ser->name);

    if (tm->ser) {
        aev_io_stop(tm->loop, &tm->ser_w);
        close_serial(tm->ser);
    }

    tm->ser = ser;
    terminal_setup_serial(tm);

    aev_io_init(&tm->ser_w, ser->fd, ser_read, AEV_READ, tm);
    aev_io_start(tm->loop, &tm->ser_w);
    return 0;
}

void
delete_terminal(struct terminal* tm)
{
    struct aev_loop* loop = tm->loop;

    aev_io_stop(loop, &tm->term_w);
    close(tm->ifd);
    close(tm->ofd);

    if (tm->ser) {
        aev_io_stop(loop, &tm->ser_w);
        close_serial(tm->ser);
    }
    free(tm);
}

void
terminal_print(struct terminal* tm, const char* fmt, ...)
{
    va_list ap;
    char buf[1024];
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    /* conver \n to \r\n */
    if (buf[len - 1] == '\n' && buf[len - 2] != '\r') {

        buf[len - 1] = '\r';
        buf[len] = '\n';
        len += 1;
    }
    write(tm->ofd, buf, len);
}

/*
 * show setup information of serial port attached on terminal @tm
 * its output like utility stty
 */
void
terminal_show_serial_setting(struct terminal* tm)
{
    struct termios* tms = &tm->ser->attr;
    int ret;
    int baudrate;
    int csize;

    baudrate = speed_to_baudrate(cfgetispeed(tms));
    terminal_print(tm, "Baudrate: %d\n", baudrate);

    /* the number of data bits */
    switch (CSIZE & tms->c_cflag) {
        case CS8:
            csize = 8;
            break;
        case CS7:
            csize = 7;
            break;
        case CS6:
            csize = 6;
            break;
        case CS5:
            csize = 5;
            break;
    }
    terminal_print(tm, "Number of data bits: %d\n", csize);

    /* stop bits: 1 or 2 */
    terminal_print(tm, "Stop bits: %d\n", CSTOPB & tms->c_cflag ? 2 : 1);

    /* partiy check */
    terminal_print(tm, "Parity: ");
    if (!(PARENB & tms->c_cflag))
        terminal_print(tm, "none\n");
    else if (PARODD & tms->c_cflag)
        terminal_print(tm, "odd\n");
    else
        terminal_print(tm, "even\n");

    /* flow control */
    terminal_print(tm, "Flow control: ");
    if ((IXON & tms->c_iflag) && (IXOFF & tms->c_iflag))
        terminal_print(tm, "Xon\n");
    else
        terminal_print(tm, "none\n");
}
