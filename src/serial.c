/* serial.c: methods used to configure serial port

Copyright © 2015 Michael Zhu <boot2linux@gmail.com>
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

static struct {
    speed_t speed;
    int baudrate;
} ser_speed[ ] = {
    {B0,       0},
    {B300,     300},
    {B1200,    1200},
    {B2400,    2400},
    {B4800,    4800},
    {B9600,    9600},
    {B19200,   19200},
    {B38400,   38400},
    {B57600,   57600},
    {B115200,  115200},
    {B230400,  230400},
};


static char * match_name[] = {
    "tty.usbserial",
    "ttyS",
    "ttyUSB",
    "ttyACM",
};

static int
match_serial(const struct dirent *entry)
{
    int i;

    for (i=0; i < sizeof(match_name)/sizeof(match_name[0]); i++) {
        if ( !strncmp(entry->d_name, match_name[i], strlen(match_name[i])) )
            return 1;
    }

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
    if (serial == NULL) {
        perror("add_serial");
        return -1;
    }
    memset(serial, 0, sizeof(struct serial));

    strncpy(serial->name, name, sizeof(serial->name));
    SLIST_INSERT_HEAD(&serial_head, serial, node);

    return 0;
}

static int
_open(char *name) {

    char path[255];

    sprintf(path, "%s%s", strncmp(name, "/dev/", sizeof "/dev/" - 1) ?
            "/dev/" : "", name);

    return open(path, O_RDWR|O_NONBLOCK, 0);
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

    int fd;
    struct termios attr;

    /* init serail head */
    SLIST_INIT(&serial_head);

    /* Scan through dir - it contains all tty-devices in the system */
    n = scandir("/dev/", &list, match_serial, NULL);
    if ( n<0 ) {
        perror("scandir");
        exit(-1);
    } else {

        while (n--) {

            /* ensure it's connected */
            fd = _open(list[n]->d_name);
            if (fd > 0) {

                if ( 0 == tcgetattr(fd, &attr) ) {

                    /* printf("add serial port %s\n", list[n]->d_name); */
                    add_serial(list[n]->d_name);
                    ret ++;
                }
                close(fd);
            }

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
open_serial(char *name, struct serial **ser)
{
    int fd;
    struct serial *serial = NULL;
    struct termios *attr;
    struct flock lock, savelock;

    fd = _open(name);
    if ( fd < 0 )
        return fd;

    lock.l_type= F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len =0;
    savelock = lock;
    fcntl(fd, F_GETLK, &lock);

    if (lock.l_type == F_WRLCK || lock.l_type == F_RDLCK)
    {
        /* printf("Process %d lock %s already!\n", lock.l_pid, name); */
        return -EBUSY;
    } else {
        fcntl(fd, F_SETLK, &savelock);
    }

    SLIST_FOREACH(serial, &serial_head, node){
        if (!strncmp(name, "/dev/", 5))
            name += 5;
        if (!strcmp(name, serial->name)){

                if (serial->fd)
                    return -EBUSY;

                serial->fd = fd;
                attr = &serial->attr;
                break;
        }
    }

    if (!serial)
        return -ENOENT;

    if (tcgetattr(fd, attr))
    {
        perror("tcgetattr");
        return -errno;
    }

    /*
     * a break condition detected on input is ignored,
     * a byte with a framing or parity error is ignored
     */
    attr->c_iflag = IGNBRK | IGNPAR;

    /*
     * CREAD: enable receiver
     * CLOCAL: local connection, no modem contol
    */
    attr->c_cflag = CREAD | CLOCAL;

    attr->c_oflag = 0; /* raw output */
    attr->c_lflag = 0;

    *ser = serial;
    return 0;
}

void
delete_serial(struct serial *ser) {

    SLIST_REMOVE(&serial_head, ser, serial, node);
    free(ser);
}

int
open_one_idle_serial( struct serial **ser )
{
    struct serial *serial;
    int ret = -ENOENT;

    SLIST_FOREACH(serial, &serial_head, node) {
        ret = open_serial(serial->name, ser);
        if ( 0 == ret )
            return 0;
    }
    return ret;
}

void
close_serial(struct serial *ser) {

    close(ser->fd);
    ser->fd = 0;
}

void
close_all_serials(void)
{
    struct serial *serial;

    SLIST_FOREACH(serial, &serial_head, node){
        close(serial->fd);
    }
}

void
iterate_serial_port(int (*cb)(struct serial *ser, void *data), void *data) {

    struct serial *serial;
    int ret;

    if (!cb) return;

    SLIST_FOREACH(serial, &serial_head, node){
        ret = cb(serial, data);
        if (ret) break;
    }
}

int
speed_to_baudrate(speed_t speed)
{
    int i = 0;

    for( i=0; i<sizeof(ser_speed)/sizeof(ser_speed[0]); i++ )
        if (speed == ser_speed[i].speed)
            return ser_speed[i].baudrate;

    return -1;
}

speed_t
baudrate_to_speed(int baudrate)
{
    int i = 0;

    for( i=0; i<sizeof(ser_speed)/sizeof(ser_speed[0]); i++ )
        if (baudrate == ser_speed[i].baudrate)
            return ser_speed[i].speed;

    return 0;
}

int
serial_setup_speed(struct serial *ser, speed_t speed) {

    return cfsetspeed(&ser->attr, speed);
}

int
serial_setup_csize(struct serial *ser, int number){
    int cs;

    if ( number < 5 || number > 8 )
        return -1;

    switch(number) {
        case 5: cs=CS5;break;
        case 6: cs=CS6;break;
        case 7: cs=CS7;break;
        case 8: cs=CS8;break;
    }

    ser->attr.c_cflag &=  ~CSIZE;
    ser->attr.c_cflag |=  cs;

    return 0;
}

int
serial_setup_stopbits(struct serial *ser, int number){

    if ( number !=1 && number != 2 )
        return -1;

    if ( number == 1)
        ser->attr.c_cflag &= ~CSTOPB;
    else
        ser->attr.c_cflag |= CSTOPB;

    return 0;
}

int
serial_setup_parity(struct serial *ser, enum ser_parity p) {

    struct termios *tms = &ser->attr;

    switch (p) {

        case SER_PARITY_NONE:
            tms->c_cflag &= ~PARENB;
            break;

        case SER_PARITY_ODD:
            tms->c_cflag |= PARENB|PARODD;
            break;

        case SER_PARITY_EVEN:
            tms->c_cflag &= ~PARODD;
            tms->c_cflag |= PARENB;
            break;

        default: return -1;
    }
    return 0;
}

int
serial_setup_flowctrl(struct serial *ser, enum ser_flow_ctrl flow) {

    struct termios *tms = &ser->attr;

    switch (flow) {
        case SER_FLOW_XON:
            tms->c_iflag |= IXON|IXOFF;
            break;
        case SER_FLOW_NONE:
            tms->c_iflag &= ~IXON;
            tms->c_iflag &= ~IXOFF;
            break;
        default:
            return -1;
    }
    return 0;
}

int
serial_apply_termios(struct serial *ser) {

    return tcsetattr(ser->fd, TCSAFLUSH, &ser->attr);
}
