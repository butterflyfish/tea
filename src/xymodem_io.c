/*
IO method of xmodem/ymodem sender
one line to give the program's name and a brief description
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


#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <fcntl.h>

#include "xymodem.h"

static int
openf(struct xymodem *xy, char *file){

    struct stat sbuf;
    off_t i;
    uint8_t *buf;

    xy->fd = open(file, O_RDWR, 0);

    fstat(xy->fd, &sbuf);

    if(sbuf.st_size % xy->mtu)
        xy->size = (1 + sbuf.st_size/xy->mtu) * xy->mtu;
    else
        xy->size = sbuf.st_size;

    /* map file to memory to speed up access */
    xy->buf = mmap (0, xy->size, PROT_WRITE, MAP_SHARED, xy->fd, 0);
    if (xy->buf == MAP_FAILED) {
        fprintf(stderr, "mmap failed\n");
        return -1;
    }

    /* assign pading CTRLZ */
    buf = xy->buf + sbuf.st_size;
    for (i = 0; i < xy->size - sbuf.st_size; ++i) {
       buf[i] = CTRLZ;
    }
    return 0;
}

static int
closef(struct xymodem *xy){

    if (munmap (xy->buf, xy->size) < 0) {
        fprintf(stderr, "munmap failed\n");
        return -1;
    }
    return 0;
}

static uint8_t
inbyte(struct xymodem *xy, int timeout){
    uint8_t key;
    read(xy->ttyfd, &key,1);
    return key;
}

static uint8_t
outbyte(struct xymodem *xy, uint8_t byte){
    write(xy->ttyfd, &byte,1);
    return 0;
}

static int
writepkt(struct xymodem *xy, uint8_t head[3], uint8_t sum[2]){
    struct iovec vec[3];

    vec[0].iov_len = 3;
    vec[0].iov_base = head;

    vec[1].iov_len = xy->mtu;
    vec[1].iov_base = xy->offset + xy->buf;

    vec[2].iov_len = xy->crc ? 2:1;
    vec[2].iov_base = &sum[0];

    return  writev(xy->ttyfd, vec, 3);
}

void
processbar(struct xymodem *xy) {
#ifndef _XYMODEM_DEBUG_
    static int len = 0;
    int i;

    if(xy->mtu == xy->offset) {
        len = 0;
        printf("Sending file %s .... ",xy->filename);
    }

    for(i=0; i<len; i++)
        putchar('\b');

    len=printf("%d%%", (int)(xy->offset*100/xy->size));

    if(xy->size == xy->offset)
        putchar('\n');
    fflush(stdout);
#endif
}

int
xymodem_send_file(int mtu, int ttyfd, char *file){

    struct xymodem xy;
    int flags;

    flags = fcntl(ttyfd, F_GETFL, 0);
    fcntl(ttyfd, F_SETFL, flags & ~O_NONBLOCK);

    xymodem_init(&xy);

    /* assign callback funcs */
    xy.openf = openf;
    xy.closef = closef;
    xy.inbyte = inbyte;
    xy.outbyte = outbyte;
    xy.writepkt = writepkt;
    xy.processbar = processbar;

    return xymodem(&xy, mtu, ttyfd, file);
}
