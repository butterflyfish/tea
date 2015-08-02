#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "serial.h"

int foreach(struct serial* serial)
{
    int fd;

    fd = open_serial(serial->path);
    if ( fd > 0 ) {
        printf("open serial %s.\n", serial->path);
        return fd;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int number;
    int fd;
    char buf[1024];

    number = scan_serial();
    if ( number == 0 )
    {
        fprintf(stderr, "No serial device is found!\n");
        exit(1);
    }

    fd = foreach_serial(foreach);
    if ( !fd )
    {
        fprintf(stderr, "No idel serial device!\n");
        exit(1);
    }

    while(1){
        int len;
        len = read(fd, buf, sizeof buf);
        if ( len < 0 )
        {
            /* perror(ttyname(fd));
            exit(1); */
        }
        else write(1, buf, len);
        sleep(1);
    }
    return 0;
}
