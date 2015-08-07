#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <termios.h>
#include <sys/queue.h>


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
 * open serial and load default value
 */
int
open_serial(char *name);

/*
 * open one idle serial port
 * return fd or error code
 */
int
open_one_idle_serial( void );

int
close_serial(int fd);

void
close_all_serials(void);

#endif

