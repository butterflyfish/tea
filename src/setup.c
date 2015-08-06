#include <termios.h>
#include "setup.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static struct termios savetio;

int
setup_stdin(void)
{
    struct termios tio;
    if ( !isatty(0) )
        return -1;

    tcgetattr(0, &savetio);
    tio = savetio;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_iflag = savetio.c_iflag & ~(INLCR|IGNCR|ICRNL);

    tio.c_cc[VEOF] = _POSIX_VDISABLE;
    tio.c_cc[VEOL] = _POSIX_VDISABLE;
    tio.c_cc[VEOL2] = _POSIX_VDISABLE;
    tio.c_cc[VERASE] = _POSIX_VDISABLE;
    tio.c_cc[VWERASE] = _POSIX_VDISABLE;
    tio.c_cc[VKILL] = _POSIX_VDISABLE;
    tio.c_cc[VREPRINT] = _POSIX_VDISABLE;
    tio.c_cc[VINTR] = _POSIX_VDISABLE;
    tio.c_cc[VQUIT] = _POSIX_VDISABLE;
    tio.c_cc[VSUSP] = _POSIX_VDISABLE;
    tio.c_cc[VLNEXT] = _POSIX_VDISABLE;
    tio.c_cc[VDISCARD] = _POSIX_VDISABLE;

    tcsetattr(0, TCSADRAIN, &tio);

    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0)|O_NONBLOCK);
    return 0;
}

static void
restore_stdin(void)
{
    if (isatty(0)) {
        tcsetattr(0, TCSADRAIN, &savetio);
    }
    fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) & ~O_NONBLOCK);
}

/*
 * interactive shell
 */
void
setup(void)
{
    char buf[1024];
    restore_stdin();

    putchar('\n');
    while(1)
    {
        int i;
        printf("tea> ");

        fgets(buf, sizeof buf, stdin);

        for ( i=0; ; i++)
            if( !isspace(buf[i]) ) break;

        /* to jump out setup */
        if ( !buf[i] )
            break;

        if ( !strncmp(buf, "quit", 4) )
            exit(0);
    }

    setup_stdin();
}
