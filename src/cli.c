/* cli.c: command line interface of Tea

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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include "kermit_io.h"
#include "xymodem.h"
#include "cli.h"
#include "serial.h"
#include "tea.h"

enum cli_state {
    CLI_ENTER = 0,
    CLI_IN,
    CLI_EXIT,
};

/* cli command */
struct command {

     const char *name;
     int (*func)(struct terminal *tm, int argc, char **argv);
     const char *params;
     const char *summary;

};

static struct termios termios_origin;

struct command cmdtbl[];


static int
cmd_quit(struct terminal *tm, int argc, char **argv){

    return 0;
}

static int
cmd_help(struct terminal *tm, int argc, char **argv){

    struct command *cmd;
    int i=0;

    for(cmd = &cmdtbl[0]; cmd->name; cmd = &cmdtbl[++i]) {

        terminal_print(tm, "\r\n  \x1b[1m%s\x1b[0m \x1b[90m%s\x1b[0m\r\n", cmd->name, cmd->params);
        terminal_print(tm, "  \x1b[33msummary:\x1b[0m %s\r\n", cmd->summary);
    }

    return 0;
}

static int
cmd_kermit_send(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if ( kermit_send_file(tm->ser->fd, &argv[1]) )
        terminal_print(tm, "send file failed!\n");

    return 0;
}

static int
cmd_ymodem_send(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if ( xymodem_send_file(1024, tm->ser->fd, argv[1]) )
        terminal_print(tm, "send file failed!\n");

    return 0;
}

static int
cmd_xmodem_send(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if ( xymodem_send_file(128, tm->ser->fd, argv[1]) )
        terminal_print(tm, "send file failed!\n");

    return 0;
}

static int
cmd_show(struct terminal *tm, int argc, char **argv){

    terminal_show_serial_setting(tm);
    return 0;
}

static int
print_serial_ports(struct serial *ser, void *data){
    struct terminal *tm = (struct terminal *)data;

    if (!strcmp(tm->ser->name, ser->name)) /* current serial port */
            terminal_print(tm, "\033[1;32m%s\033[0m\n", ser->name); /* green color */
    else if (ser->fd) /* connected by other terminal */
            terminal_print(tm, "\033[1;31m%s\033[0m\n", ser->name); /* red color */
    else
            terminal_print(tm, "%s\n", ser->name);

    return 0;
}

static int
cmd_list(struct terminal *tm, int argc, char **argv){

    iterate_serial_port(print_serial_ports, tm);
    return 0;
}

static int
cmd_connect(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if (!strcmp(argv[1], tm->ser->name))
        return 0;

    terminal_connect_serial(tm, argv[1]);

    return 0;
}

static int
cmd_speed(struct terminal *tm, int argc, char **argv){

    speed_t speed;

    if ( argc != 2 )
        return -1;

    speed = baudrate_to_speed(atoi(argv[1]));
    if ( speed == 0 ) {
        terminal_print(tm, "illegal baudrate \x1b[33m%s\x1b[0m\n", argv[1]);
        return 0;
    }

    serial_setup_speed(tm->ser, speed);
    serial_apply_termios(tm->ser);

    return 0;
}

static int
cmd_csize(struct terminal *tm, int argc, char **argv){

    int cs;

    if ( argc != 2 )
        return -1;

    cs = atoi(argv[1]);
    if ( serial_setup_csize(tm->ser, cs) < 0 ) {
        terminal_print(tm, "illegal csize \x1b[33m%s\x1b[0m\n", argv[1]);
        return 0;
    }

    serial_apply_termios(tm->ser);

    return 0;
}

static int
cmd_stopbits(struct terminal *tm, int argc, char **argv){

    int stopbits;

    if ( argc != 2 )
        return -1;

    stopbits = atoi(argv[1]);
    if ( serial_setup_stopbits(tm->ser, stopbits) < 0 ) {
        terminal_print(tm, "illegal stopbits \x1b[33m%s\x1b[0m\n", argv[1]);
        return 0;
    }

    serial_apply_termios(tm->ser);

    return 0;
}

static int
cmd_parity(struct terminal *tm, int argc, char **argv){

    enum ser_parity p;

    if ( argc != 2 )
        return -1;

    if (0 == strcmp(argv[1], "even"))
        p = SER_PARITY_EVEN;
    else if (0 == strcmp(argv[1], "odd"))
        p = SER_PARITY_ODD;
    else if (0 == strcmp(argv[1], "none"))
        p = SER_PARITY_NONE;
    else {
        terminal_print(tm, "illegal parity \x1b[33m%s\x1b[0m\n", argv[1]);
        return 0;
    }

    serial_setup_parity(tm->ser, p);
    serial_apply_termios(tm->ser);

    return 0;
}

static int
cmd_flow(struct terminal *tm, int argc, char **argv){

    enum ser_flow_ctrl flow;

    if ( argc != 2 )
        return -1;

    if (0 == strcmp(argv[1], "xon"))
        flow = SER_FLOW_XON;
    else if (0 == strcmp(argv[1], "none"))
        flow = SER_FLOW_NONE;
    else {
        terminal_print(tm, "illegal flow control type \x1b[33m%s\x1b[0m\n", argv[1]);
        return 0;
    }

    serial_setup_flowctrl(tm->ser, flow);
    serial_apply_termios(tm->ser);

    return 0;
}


static int
cli_exec(struct terminal *tm, char *buf) {

    #define MAX_ARGC 5
    char *argv[MAX_ARGC] = {0};
    char *sep = " \t\r\n";
    char *token;
    int argc=0;
    struct command *cmd;
    int i=0;
    int ret;

    token = strtok(buf, sep);
    while( (token != NULL) && (argc <= MAX_ARGC) )  {

        argv[argc] = token;
        /* printf("argv[%d] is %s\n", argc, argv[argc]); */
        argc++;

        token= strtok(NULL,sep);
    }

    if ( !strcmp(argv[0], "quit") )
        return -1;

    for(cmd = &cmdtbl[0]; cmd->name; cmd = &cmdtbl[i++]) {

        if ( 0 == strcmp(cmd->name, argv[0]) ) {
            ret = cmd->func(tm, argc, argv);
            if ( ret < 0 && cmd->params)
                terminal_print(tm, "\x1b[33mSYNOPSIS:\x1b[0m %s %s\n", cmd->name, cmd->params);

            return 0;
        }
    }
    terminal_print(tm, "Unknow command \x1b[33m%s\x1b[0m\n", argv[0]);
    return 0;
}

/*
 * raw mode;
 * input is not assembled into lines and special characters are not processed
 *
 * canonical:
 * canonical (or cooked mode under BSD) is the default.
 * input is assembled into lines and special characters are processed.
 */
int
enable_raw_mode(struct terminal *tm)
{
    struct termios raw;
    if ( !isatty(tm->ifd) )
        return -1;

    tcgetattr(tm->ifd, &termios_origin);
    raw = termios_origin;

    raw.c_oflag = 0;
    raw.c_lflag = 0;

    /* input mode
     * no NL to CR, no CR to NL
     * no XON/XOFF software flow control
     */
    raw.c_iflag &= ~(INLCR | ICRNL | IGNCR | IXON);

    raw.c_cc[VEOF] = _POSIX_VDISABLE;
    raw.c_cc[VEOL] = _POSIX_VDISABLE;
    raw.c_cc[VEOL2] = _POSIX_VDISABLE;
    raw.c_cc[VERASE] = _POSIX_VDISABLE;
    raw.c_cc[VWERASE] = _POSIX_VDISABLE;
    raw.c_cc[VKILL] = _POSIX_VDISABLE;
    raw.c_cc[VREPRINT] = _POSIX_VDISABLE;
    raw.c_cc[VINTR] = _POSIX_VDISABLE;
    raw.c_cc[VQUIT] = _POSIX_VDISABLE;
    raw.c_cc[VSUSP] = _POSIX_VDISABLE;
    raw.c_cc[VLNEXT] = _POSIX_VDISABLE;
    raw.c_cc[VDISCARD] = _POSIX_VDISABLE;

    tcsetattr(tm->ifd, TCSAFLUSH, &raw);

    return 0;
}

void
disable_raw_mode(struct terminal *tm)
{
    if ( isatty(tm->ifd) )
        tcsetattr(tm->ifd, TCSAFLUSH, &termios_origin);
}

/*
 * interactive shell
 * return:
 *      true: processing; false: finished
 */
int
cli_process(struct terminal *tm)
{
    int ret = 1;

    /* tm->buf[tm->len] = 0; */
    /* fprintf(stderr, "buf[len=%d] is %s\n", tm->len, tm->buf); */

    switch(tm->cli) {

        case CLI_ENTER:

            if ( tm->buf[tm->len - 1] != TEA_ESC_KEY ) {
                ret = 0;
                break;
            }

            disable_raw_mode(tm);
            aev_io_stop(tm->loop, &tm->ser_w);

            terminal_print(tm, "\n\033[1;32mPress Enter to resume the connection,"
                               "type help get command list.\033[0m\n"); /* green color */
            terminal_print(tm, "\rTea> ");
            tm->cli = CLI_IN;
            tm->len = 0;
            break;

        case CLI_IN:
            if (tm->buf[tm->len-1] == 8) { /* backsapce */
                tm->len -= 2;
                break;
            }

            if (tm->buf[tm->len - 1] != '\r' && tm->buf[tm->len - 1] != '\n')
                break;

            if (tm->len == 1) { /* will jump out CLI */
                tm->cli = CLI_EXIT;
                tm->len = 0;
                ret = 0;
                break;
            }

            tm->buf[tm->len] = 0;
            if ( cli_exec(tm, (char*)tm->buf) ){
                return -1;
            }
            tm->len = 0;
            terminal_print(tm, "\rTea> ");
            break;

        case CLI_EXIT:

            enable_raw_mode(tm);
            aev_io_start(tm->loop, &tm->ser_w);

            tm->cli = CLI_ENTER;
            ret = 0;
            break;

        default:break;
    }

    return ret;
}

struct command cmdtbl[] = {

    {"quit",    cmd_quit, "",   "Exit Tea!"},
    {"help",    cmd_help,   "",  "Display what you are seeing"},
    {"show",    cmd_show,   "",  "Show current configuration"},
    {"list",    cmd_list,   "",  "List serial port"},
    {"connect", cmd_connect,   "<serial port name>",  "Connect serial port"},
    {"speed",   cmd_speed,   "<baudrate>",  "Change baudrate,.e.g 115200"},
    {"csize",   cmd_csize,   "<csize>",  "Change number of data bit,.e.g 7"},
    {"stopbits",cmd_stopbits,   "<stopbits>",  "Change number of stop bit to 1 or 2"},
    {"parity",  cmd_parity,   "<parity type>",  "Change parity type to none|even|odd"},
    {"flow",    cmd_flow,   "<flow type>",  "Change flow type to none|xon"},
    {"ks",      cmd_kermit_send, "<file>", "Send file using Kermit"},
    {"xs",      cmd_ymodem_send, "<file>", "Send file using Xmodem. Data size is 128B"},
    {"ys",      cmd_ymodem_send, "<file>", "Send file using Ymodem. Data size is 1024B"},
    {NULL, NULL, NULL}
};


