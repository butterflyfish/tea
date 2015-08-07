#ifndef XFER_H_
#define XFER_H_

int
new_emulator(int ser_fd, int ifd, int ofd);

void
xfer_init(void);

void
xfer_loop(void);

#endif
