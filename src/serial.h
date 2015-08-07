#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <termios.h>
#include <sys/queue.h>

struct serial {

    struct termios attr;

    int fd;
    char path[100];
    SLIST_ENTRY(serial) node;
};


/*
 * scan serial port
 *
 * it must be invoked before other xxx_serial functions
 *
 * @return: the number of serial port
 */
int
scan_serial(void);

/*
 * iterating serial port and executing callback foreach
 * it stop iterating if callback func return non-zero
 */
int
foreach_serial( int (*foreach)(struct serial *) );

/*
 * open serial and load default value
 */
int
open_serial(char *name);

/*
 * open one idle serial port
 * return fd or error code
 */
int
open_one_idle_serail( void );

int
close_serial(int fd);

void
close_all_serials(void);

#endif

