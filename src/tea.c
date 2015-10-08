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
#include "serial.h"
#include "terminal.h"

static char *ver = "2005.10.8";

static
void usage()
{
    fprintf(stderr,
            "Usage: tea [options]\n\n"
            "--version:             Show version\n"
            "--help|-h:             Help info\n"
            "--device|-d:           Open this serial port. If no, try to open one automatically.\n"
    );

}

int main(int argc, char *argv[])
{
    int number;
    int fd;

    int c;
    int option_index = 0;

    int version = 0;
    char *device = NULL;

    struct terminal *tm;
    struct aev_loop *loop = aev_loop_new(8);

    struct option long_options[] = {

        {"version", no_argument,       &version, 1},
        {"help",    no_argument,       0, 'h'},
        {"device",  required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };


    while (1) {

         c = getopt_long (argc, argv, "hd:",
                          long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 0:
        fprintf (stderr, "Version is %s\n", ver);
        exit(1);

        case 'h':
        usage();
        exit(1);

        case 'd':
        device = optarg;
        break;

        default:
        exit(1);
        }
    }

    number = scan_serial();
    if ( number == 0 )
    {
        fprintf(stderr, "No serial port or Permission denied\n");
        exit(1);
    }

    fd = device ? open_serial(device) : open_one_idle_serial();
    if ( fd < 0 )
    {
        switch (fd) {
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

        exit(1);
    }

    fprintf(stderr, "\033[1;31mEscape key of Tea is Ctrl-]\033[0m\n");

    tm = new_terminal(loop, fd, 0, 1);
    aev_run(loop);
    delete_terminal(tm);

    return 0;
}
