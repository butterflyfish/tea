#include <termios.h>
#include "setup.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
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

    /* TODO: it should not place here */
    fcntl(ifd, F_SETFL, fcntl(ifd, F_GETFL, 0)|O_NONBLOCK);
    return 0;
}

void
disable_raw_mode(int ifd)
{
    tcsetattr(ifd, TCSAFLUSH, &origin);
    fcntl(ifd, F_SETFL, fcntl(ifd, F_GETFL, 0) & ~O_NONBLOCK);
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
