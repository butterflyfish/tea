/* setup.c: control terminal
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

#include <termios.h>
#include "setup.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static struct termios origin;

/*
 * raw mode;
 * input is not assembled into lines and special characters are not processed
 *
 * canonical:
 * canonical (or cooked mode under BSD) is the default.
 * input is assembled into lines and special characters are processed.
 */
int
enable_raw_mode(int ifd)
{
    struct termios raw;
    if ( !isatty(ifd) )
        return -1;

    tcgetattr(ifd, &origin);
    raw = origin;

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

    tcsetattr(ifd, TCSAFLUSH, &raw);

    return 0;
}

void
disable_raw_mode(int ifd)
{
    tcsetattr(ifd, TCSAFLUSH, &origin);
}

/*
 * interactive shell
 */
void
setup_loop(int ifd, int ofd, int ser_fd)
{
    char buf[1024];

    disable_raw_mode(ifd);

    write(ofd, "\n", sizeof "\n");
    while(1)
    {
        write(ofd, "tea> ", sizeof "tea> ");

        read(ifd, buf, sizeof buf);

        /* to jump out setup */
        if ( buf[0] == '\n' )
            break;

        if ( !strncmp(buf, "quit", 4) )
            exit(0);
    }

    enable_raw_mode(ifd);
}
