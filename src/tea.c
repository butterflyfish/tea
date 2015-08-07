#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "serial.h"
#include "setup.h"


int main(int argc, char *argv[])
{
    int number;
    int fd;
    char buf[1024];

    number = scan_serial();
    if ( number == 0 )
    {
        fprintf(stderr, "No serial port is found!\n");
        exit(1);
    }

    fd = open_one_idle_serial();
    if ( !fd )
    {
        fprintf(stderr, "No idel serial port!\n");
        exit(1);
    }

    enable_raw_mode(0);

    while(1){

        int len;
        char tbuf;

        len = read(fd, buf, sizeof buf);
        if ( len <= 0 )
        {
            /* perror(ttyname(fd));
            exit(1); */
        }
        else write(1, buf, len);

        len = read(0, &tbuf, 1);
        if( len > 0 )
        {
            if ( tbuf == 29 ) /* esc key: Ctrl-] */
                setup_loop(0, 1, fd);
            else
                write(fd, &tbuf, 1);
        }
    }
    return 0;
}
