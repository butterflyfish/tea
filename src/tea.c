/* tea.c: main file
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include "terminal.h"
#include "tea.h"
#include "telnet.h"

static char* ver = "2016.Jan.11";

static tea_t tea = {

    .port = 23,
    .backlog = 10,

    .speed = B115200,
    .cs = 8,
    .stopbits = 1,
    .p = SER_PARITY_NONE,
    .flow = SER_FLOW_NONE,
};

static void
usage()
{
    fprintf(stderr,
            "Usage: tea [options]\n\n"
            "--version:                Show version\n"
            "--help|-h:                Help info\n"
            "--telnet:                 Share serial port over telnet\n"
            "--port:                   Listen port of telnet\n"
            "--forground:              Telnet run at forground\n"
            "--device|-d:              Serial port name or path. If no, try to open one automatically, but ignored if telnet is enabled.\n"
            "--speed|-s:               Serial port speed. Default is 115200\n"
            "--bits|-b <5|6|7|8>:      The number of data bits. Default is 8\n"
            "--stopbits|-t <1|2>:      The number of stop bit. Default is 1\n"
            "--parity|-p <odd|even>:   Parity setting. Default is none\n"
            "--flow|-f <xon>:          Flow control type. Default is none\n");
}

static void
create_pid_file(void)
{

    int fd;
    FILE* pidfile;
    struct flock lock, savelock;

    fd = open(TEA_PID_FILE, O_RDWR | O_CREAT, 0640);
    if (fd < 0) {
        fprintf(stderr, "failed to create pid file:%s\n", strerror(errno));
        exit(1);
    }

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    savelock = lock;
    fcntl(fd, F_GETLK, &lock);

    if (lock.l_type == F_WRLCK) {
        fprintf(stderr, "Another Tea instance(pid=%d) is running\n", lock.l_pid);
        exit(1);
    } else {
        fcntl(fd, F_SETLK, &savelock);
    }

    pidfile = fdopen(fd, "w");
    fprintf(pidfile, "%d\n", getpid());

    /* close will unlock, so use fflush to update pid */
    fflush(pidfile);
}

int
main(int argc, char* argv[])
{
    int number;
    int pid;
    int forground = 0;

    int c;
    int option_index = 0;

    int version = 0;
    int telnet = 0;
    char* device = NULL;

    struct option long_options[] = {

        { "version", no_argument, &version, 1 },
        { "telnet", no_argument, &telnet, 1 },
        { "port", required_argument, 0, 'n' },
        { "help", no_argument, 0, 'h' },
        { "device", required_argument, 0, 'd' },
        { "speed", required_argument, 0, 's' },
        { "bits", required_argument, 0, 'b' },
        { "stopbits", required_argument, 0, 't' },
        { "parity", required_argument, 0, 'p' },
        { "flow", required_argument, 0, 'f' },
        { "forground", no_argument, &forground, 1 },
        { 0, 0, 0, 0 }
    };

    while (1) {

        c = getopt_long(argc, argv, "hd:s:b:t:p:f:",
                        long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
            case 0:
                if (version) {
                    fprintf(stderr, "Version is %s\n", ver);
                    exit(1);
                }
                break;

            case 'h':
                usage();
                exit(1);

            case 'n':
                tea.port = atoi(optarg);
                break;

            case 'd':
                device = optarg;
                break;

            case 's':
                tea.speed = baudrate_to_speed(atoi(optarg));
                if (tea.speed == 0) {
                    fprintf(stderr, "Illegal baudrate\n");
                    exit(1);
                }
                break;

            case 'b':
                tea.cs = atoi(optarg);
                if (tea.cs < 5 || tea.cs > 8) {
                    fprintf(stderr, "number of data bits is illegal\n");
                    exit(1);
                }
                break;

            case 't':
                tea.stopbits = atoi(optarg);
                if (tea.stopbits != 1 && tea.stopbits != 2) {
                    fprintf(stderr, "number of stop bits is illegal\n");
                    exit(1);
                }
                break;

            case 'p':
                if (0 == strcmp(optarg, "even"))
                    tea.p = SER_PARITY_EVEN;
                else if (0 == strcmp(optarg, "odd"))
                    tea.p = SER_PARITY_ODD;
                else {

                    fprintf(stderr, "parity type is illegal\n");
                    exit(1);
                }
                break;

            case 'f':
                if (0 == strcmp(optarg, "xon"))
                    tea.flow = SER_FLOW_XON;
                else if (0 == strcmp(optarg, "none"))
                    tea.flow = SER_FLOW_NONE;
                else {
                    fprintf(stderr, "flow control type is illegal\n");
                    return 0;
                }
                break;

            default:
                exit(1);
        }
    }

    number = scan_serial();
    if (number == 0) {
        fprintf(stderr, "No serial port or Permission denied\n");
        exit(1);
    }

    if (telnet) {
        char service[100];
        int fd;

        if (0 == forground) {
            pid = fork();
            if (pid < 0) {
                fprintf(stderr, "Fork failed: %s", strerror(errno));
                exit(1);
            } else if (pid > 0) {
                exit(0);
            }

            /* obtain a new process group */
            setsid();

            if (chdir("/") < 0) {
                fprintf(stderr, "unable to chdir to '/': %s", strerror(errno));
                exit(1);
            }

            create_pid_file();

            fd = open("/dev/null", O_RDWR);
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
        }

        aev_loop_init(&tea.loop);

        sprintf(service, "%d", tea.port);
        start_telnet_server(&tea, NULL, service);
    } else {
        aev_loop_init(&tea.loop);
        new_terminal(&tea, device, 0, 1, tty_read);
    }

    aev_run(&tea.loop);

    return 0;
}
