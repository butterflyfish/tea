#include <sys/queue.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "serial.h"


static SLIST_HEAD(,serial) serial_head;

/* point to serial used by this process
 * one process talk to one serial
 */
struct serial *current;

#define MATCH_SERIAL_STR0 "tty.usbserial"
#define MATCH_SERIAL_STR1 "ttyS"
#define MATCH_SERIAL_STR2 "ttyUSB"

static int
match_serial(const struct dirent *entry)
{
    if ( !strncmp(entry->d_name, MATCH_SERIAL_STR0, sizeof MATCH_SERIAL_STR0 - 1) )
        return 1;

    if ( !strncmp(entry->d_name, MATCH_SERIAL_STR1, sizeof MATCH_SERIAL_STR1 - 1) )
        return 1;

    if ( !strncmp(entry->d_name, MATCH_SERIAL_STR2, sizeof MATCH_SERIAL_STR2 - 1) )
        return 1;
    return 0;
}

/*
* add serial port to list
*/
static int
add_serial(const char *name)
{
    struct serial *serial;

    serial = malloc(sizeof *serial);
    sprintf(serial->path,"/dev/%s",name);
    SLIST_INSERT_HEAD(&serial_head, serial, node);

    return 0;
}

/*
 * scan serial port and build linked list
 *
 * it must be invoked before other xxx_serial functions
 *
 * @return: the number of serial port
 */
int
scan_serial(void)
{
    int n;
    int ret = 0;
    struct dirent **list;

    /* init serail head */
    SLIST_INIT(&serial_head);

    /* Scan through dir - it contains all tty-devices in the system */
    n = scandir("/dev/", &list, match_serial, NULL);
    if ( n<0 ) {
        perror("scandir");
        exit(-1);
    } else {
        ret = n;
        while (n--) {
            add_serial(list[n]->d_name);
            free(list[n]);
        }
        free(list);
    }
    return ret;
}

/*
 * open serial and load default value
 */
int
open_serial(char *name)
{
    int fd;
    char path[255];
    struct serial *serial = NULL;
    struct termios *attr;

    sprintf(path, "%s%s", strncmp(name, "/dev/", sizeof "/dev/" - 1) ?
            "/dev/" : "", name);

    fd = open(path, O_RDWR|O_NONBLOCK|O_EXLOCK, 0);
    if ( fd < 0 )
        return -1;

    if ( flock(fd, LOCK_EX) )
        return -1;

    SLIST_FOREACH(serial, &serial_head, node){
        if (!strcmp(serial->path, path)){
                serial->fd = fd;
                current = serial;
                attr = &serial->attr;
                break;
        }
    }
    if (!serial)
        return -1;

    if (tcgetattr(fd, attr))
    {
        perror("tcgetattr");
        return -1;
    }

    /*  set default baudrate to 115200 */
    cfsetspeed(attr, B115200);

    /*
     * a break condition detected on input is ignored,
     * a byte with a framing or parity error is ignored
     */
    attr->c_iflag = IGNBRK | IGNPAR;

    /*
     * enable receiver
     * a connection does not depend on the state of the modem status lines
     * 8 bits are used for both transmission and reception
    */
    attr->c_cflag = CREAD | CLOCAL | CS8 ;

    attr->c_oflag = 0;
    attr->c_lflag = 0;

    if (tcsetattr(fd,TCSAFLUSH,attr))
    {
        perror("tcsetattr");
        return -1;
    }

    return fd;
}

int
open_one_idle_serial( void )
{
    struct serial *serial;
    int fd;

    SLIST_FOREACH(serial, &serial_head, node) {

        fd = open_serial(serial->path);
        if ( fd > 0 ) {
            printf("open serial %s.\n", serial->path);
            return fd;
        }
    }
    return fd;
}


int
close_serial(int fd)
{
    flock(fd, LOCK_UN);
    return close(fd);
}

void
close_all_serials(void)
{
    struct serial *serial;

    SLIST_FOREACH(serial, &serial_head, node){
        close_serial(serial->fd);
    }
}

int
foreach_serial(int (*foreach)(struct serial *))
{
    struct serial *serial;
    int ret = -1;

    if (NULL == foreach)
        return -1;

    SLIST_FOREACH(serial, &serial_head, node) {
        ret = foreach(serial);
        if ( ret )
            return ret;
    }
    return ret;
}

static int
serial_translate_baud(int inrate)
{
    switch(inrate)
    {
        case 0:
            return B0;
        case 300:
            return B300;
        case 1200:
            return B1200;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        default:
            return -1; /* do custom divisor */
    }
}

/*
 * show setup information of current serial port
 * its output like utility stty
 *
 * @fd: where to write setup info
 */
void
show_serial_setup(int fd)
{
    write(fd, "Not implement\n", sizeof "Not implement\n");
}
