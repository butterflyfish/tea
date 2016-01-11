/* xmodem/ymodem sender
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

#ifndef _XYMODEM_H_
#define _XYMODEM_H_

#include <sys/types.h>
#include <stdint.h>

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CTRLZ 0x1A

// #define _XYMODEM_DEBUG_
#ifdef _XYMODEM_DEBUG_
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

enum xymodem_state
{

    XYMODEM_WAIT_START = 1, /* wait start char 'C' or NAK */
    XYMODEM_SEND_PKT,       /* send packet */
    XYMODEM_SEND_EOT,       /* tell receiver the end of file */
    XYMODEM_DONE,
};

typedef void (*log_t)(void* data, char* fmt, ...);

struct xymodem
{

    off_t size;   /* size of buffer to be sent */
    uint8_t* buf; /* point to data buffer to be sent */
    off_t offset; /* offset of daa buffer */

    enum xymodem_state state;
    int crc; /* 1: CRC; else check sum */
    int mtu; /* size of data unit */

    int ttyfd; /* fd of serial device */
    int fd;    /* fd of file *filename* */
    char* filename;

    int (*openf)(struct xymodem* xy, char* file);
    int (*closef)(struct xymodem* xy);
    uint8_t (*inbyte)(struct xymodem* xy, int timeout);
    uint8_t (*outbyte)(struct xymodem* xy, uint8_t byte);
    int (*writepkt)(struct xymodem* xy, uint8_t head[3], uint8_t sum[2]);
    void (*processbar)(struct xymodem* xy);

    log_t log;
    void* data;
};

void
xymodem_io_init(struct xymodem* xy, log_t log, void* data);

int
xymodem_send_file(struct xymodem* xy, int mtu, int ttyfd, char* file);

#endif
