#ifndef KERMIT_IO_H
#define KERMIT_IO_H

typedef void (*klog_t)(void* data, char* fmt, ...);

/* send file via kermit protocol
 *
 * ofd: tty fd
 * filelist: list of files,end of NULL
 */
int
kermit_send_file(int ofd, char** filelist, klog_t log, void* data);
#endif
