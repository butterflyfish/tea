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


static struct termios termios_origin;

static int
cmd_quit(struct terminal *tm, int argc, char **argv);

static int
cmd_help(struct terminal *tm, int argc, char **argv);

static int
cmd_kermit_send(struct terminal *tm, int argc, char **argv);

static int
cmd_xmodem_send(struct terminal *tm, int argc, char **argv);

static int
cmd_ymodem_send(struct terminal *tm, int argc, char **argv);

static int
cmd_show(struct terminal *tm, int argc, char **argv);

static int
cmd_speed(struct terminal *tm, int argc, char **argv);

static int
cmd_list(struct terminal *tm, int argc, char **argv);

static int
cmd_csize(struct terminal *tm, int argc, char **argv);

struct command cmdtbl[] = {

    {"quit",    cmd_quit, "",   "Exit Tea!"},
    {"help",    cmd_help,   "",  "Display what you are seeing"},
    {"show",    cmd_show,   "",  "Show current configuration"},
    {"speed",   cmd_speed,   "<baudrate>",  "Change baudrate,.e.g 115200"},
    {"csize",   cmd_csize,   "<csize>",  "Change number of data bits,.e.g 7"},
    {"list",    cmd_list,   "",  "List serial port"},
    {"ks",      cmd_kermit_send, "<file>", "Send file using Kermit"},
    {"xs",      cmd_ymodem_send, "<file>", "Send file using Xmodem. Data size is 128B"},
    {"ys",      cmd_ymodem_send, "<file>", "Send file using Ymodem. Data size is 1024B"},
    {NULL, NULL, NULL}
};


static int
cmd_quit(struct terminal *tm, int argc, char **argv){

    exit(0);
}

static int
cmd_help(struct terminal *tm, int argc, char **argv){

    struct command *cmd;
    int i=0;

    for(cmd = &cmdtbl[0]; cmd->name; cmd = &cmdtbl[++i]) {

        printf("\r\n  \x1b[1m%s\x1b[0m \x1b[90m%s\x1b[0m\r\n", cmd->name, cmd->params);
        printf("  \x1b[33msummary:\x1b[0m %s\r\n", cmd->summary);
    }

    return 0;
}

static int
cmd_kermit_send(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if ( kermit_send_file(tm->ser->fd, &argv[1]) )
        fprintf(stderr, "send file failed!\n");

    return 0;
}

static int
cmd_ymodem_send(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if ( xymodem_send_file(1024, tm->ser->fd, argv[1]) )
        fprintf(stderr, "send file failed!\n");

    return 0;
}

static int
cmd_xmodem_send(struct terminal *tm, int argc, char **argv){

    if ( argc != 2 )
        return -1;

    if ( xymodem_send_file(128, tm->ser->fd, argv[1]) )
        fprintf(stderr, "send file failed!\n");

    return 0;
}

static int
cmd_show(struct terminal *tm, int argc, char **argv){

    show_serial_setup(tm->ser, tm->ofd);
    return 0;
}

static int
cmd_list(struct terminal *tm, int argc, char **argv){

    list_serial_port(tm->ser, tm->ofd);
    return 0;
}

static int
cmd_speed(struct terminal *tm, int argc, char **argv){

    speed_t speed;

    if ( argc != 2 )
        return -1;

    speed = baudrate_to_speed(atoi(argv[1]));
    if ( speed == 0 ) {
        fprintf(stderr, "illegal baudrate \x1b[33m%s\x1b[0m\n", argv[1]);
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
        fprintf(stderr, "illegal csize \x1b[33m%s\x1b[0m\n", argv[1]);
        return 0;
    }

    serial_apply_termios(tm->ser);

    return 0;
}

static void
cli_exec(struct terminal *tm, char *buf) {

    #define MAX_ARGC 5
    char *argv[MAX_ARGC] = {0};
    char *sep = " \t\n";
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

    for(cmd = &cmdtbl[0]; cmd->name; cmd = &cmdtbl[i++]) {

        if ( 0 == strcmp(cmd->name, argv[0]) ) {
            ret = cmd->func(tm, argc, argv);
            if ( ret < 0 && cmd->params)
                fprintf(stderr, "\x1b[33mSYNOPSIS:\x1b[0m %s %s\n", cmd->name, cmd->params);

            return;
        }
    }
    fprintf(stderr, "Unknow command \x1b[33m%s\x1b[0m\n", argv[0]);
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
    tcsetattr(tm->ifd, TCSAFLUSH, &termios_origin);
}

/*
 * interactive shell
 */
void
cli_loop(struct terminal *tm)
{
    char buf[1024];
    int len;

    fcntl(tm->ifd, F_SETFL, fcntl(tm->ifd, F_GETFL, 0) & ~O_NONBLOCK);

    disable_raw_mode(tm);

    write(tm->ofd, "\n", sizeof "\n");

    /* green color */
    fprintf(stderr, "\n\033[1;32mPress Enter to resume the connection,type help get command list.\033[0m\n");

    while(1)
    {
        write(tm->ofd, "Tea> ", sizeof "Tea> ");

        len = read(tm->ifd, buf, sizeof buf);
        buf[len] = 0;

        /* to jump out setup */
        if ( buf[0] == '\n' ) {
            write(tm->ser->fd, buf, 1);
            break;
        }

        cli_exec(tm, buf);
    }

    enable_raw_mode(tm);
    fcntl(tm->ifd, F_SETFL, fcntl(tm->ifd, F_GETFL, 0) | O_NONBLOCK);
}
