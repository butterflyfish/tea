#ifndef KERMIT_FILE_H
#define KERMIT_FILE_H

/* send file via kermit protocol
 *
 * ofd: tty fd
 * filelist: list of files,end of NULL
 */
int
kermit_send_file(int ofd, char ** filelist);
#endif
