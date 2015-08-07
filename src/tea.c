#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "serial.h"
#include "xfer.h"


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

    xfer_init();
    new_emulator(fd, 0, 1);

    xfer_loop();
    return 0;
}
