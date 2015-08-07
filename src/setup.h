#ifndef _SETUP_H
#define _SETUP_H

/*
 * enable raw mode for controlling tty @ifd
 */
int
enable_raw_mode(int ifd);

void
disable_raw_mode(int ifd);

/*
 * interactive shell
 *
 * jump to serial again if only pressing Enter key
 *
 * @ser_fd: represent which serial port
 * @ifd: input of controlling tty
 * @ofd: output of controlling tty
 *
 */
void
setup_loop(int ifd, int ofd, int ser_fd);


#endif
