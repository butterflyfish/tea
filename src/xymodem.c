/*
xmodem/ymodem sender
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

/* packet format:
 *      +---------+-------+-------+------+-----+
 *      | SOH/STX | Seq1. | Seq2. | data | SUM |
 *      +---------+-------+-------+------+-----+
 *      SOH/STX  = which is used, it's decided by sender
 *      STX  = data unit is 1024 bytes
 *      Seq1 = The sequence number. It starts from 1.
 *      Seq2 = The complement of the sequence number.
 *      Data = SOH: data unit is 128 bytes ; STX: data unit is 1024 bytes
 *      SUM  = Add the contents of the data bytes and use the low-order
 *             8 bits of the result; or 16 byte CRC. It's decided by receiver.
 *
 * If lenght of data is less than 128/1024 bytes, ^Z padding must be added.
 *
 * Example sequence
 *      Receiver                       Sender
 *        |                              |
 *        | -------- C or NAK -------->  |
 *        |                              |
 *        | <--------- PKT ------------  |
 *        | --------- ACK ------------>  |
 *        |                              |
 *        |         ... ...              |
 *        |                              |
 *        | <-------- EOT -------------  |
 *        | --------- ACK ------------>  |
 *        |                              |
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xymodem.h"


/*
 * calculate crc16-ccitt very fast
 * plese refer to http://www.ccsinfo.com/forum/viewtopic.php?t=24977
 */
static uint16_t
crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t x;
    uint16_t crc = 0;

    while (len--) {
        x = (crc >> 8) ^ *buf++;
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }
    return crc;
}

static void
cal_sum(struct xymodem *xy, uint8_t *sum) {

    uint16_t crc;
    uint8_t *buf;
    int i;

    buf = xy->buf + xy->offset;
    if(xy->crc == 1) {

        crc = crc16(buf, xy->mtu);
        sum[0] = (crc>>8) & 0xFF;
        sum[1] = crc & 0xFF;
    }
    else {
        sum[0]=0;
        for (i = 0; i < xy->mtu; ++i) {
            sum[0] += buf[i];
        }
    }
}

static int
xymodem_send(struct xymodem *xy)
{
    uint8_t key;
    uint8_t head[3];
    uint8_t sum[2];
    static uint8_t pktnum = 1;

    switch(xy->state) {

        case XYMODEM_WAIT_START:

            printf("Wait for receiver to start ...\n");
            key = xy->inbyte(xy,0);
            if (key == 'C' || key == NAK) {

                if (key == 'C') xy->crc = 1;
                pktnum = 1;
                xy->state = XYMODEM_SEND_PKT;
                debug("jump to SEND-PKT state\n");

            } else if (key == CAN) {

                debug("send ACK for received CAN\n");
                xy->outbyte(xy,ACK);
                xy->state = XYMODEM_DONE;

            } else {

                printf("Received unexpected char:%x\n",key);
                return -1;
            }
            break;

        case XYMODEM_SEND_PKT:

            head[0] = (xy->mtu == 128) ? SOH:STX;
            head[1] = pktnum;
            head[2] = ~pktnum;

            cal_sum(xy,sum);

            debug("send %d pkt\n",pktnum);
            xy->writepkt(xy, head, sum);

            key = xy->inbyte(xy,0);

            if (key == ACK) {
                debug("recv ACK for pkt %d\n", pktnum);

                pktnum ++;
                xy->offset += xy->mtu;

                if (xy->processbar)
                    xy->processbar(xy);

                if (xy->offset >= xy->size) {
                    xy->state = XYMODEM_SEND_EOT;
                }
            }
            else if (key == NAK) {
                debug("recv NAK\n");
            }
            else if (key == CAN) {
                debug("send ACK for received CAN\n");
                xy->outbyte(xy,ACK);
                xy->state = XYMODEM_DONE;
            }else {
                printf("Received unexpected char:%x\n",key);
                return -1;
            }
            break;

        case XYMODEM_SEND_EOT:

            debug("send EOT\n");
            xy->outbyte(xy,EOT);

            key=xy->inbyte(xy,0);
            if (key == ACK) {
                debug("recv ACK for EOT\n");
                xy->state = XYMODEM_DONE;
                break;
            }
            if (key != ACK) {
                printf("Timeout EOT\n");
                return -1;
            }
            break;

        default:
            printf("Unknow state\n");
            return -1;
    }
    return 0;
}


int
xymodem_send_file(struct xymodem *xy, int mtu, int ttyfd, char *filename){

    int ret = 0;
    int flags;

    if ( xy->openf == NULL || xy->closef == NULL ||
        xy->inbyte == NULL || xy->outbyte == NULL ||
        xy->writepkt == NULL) {
        fprintf(stderr, "Either of openf,closef,inbyte,outbyte,writepkt can not be NULL\n");
        return -1;
    }


    if (mtu != 128 && mtu != 1024){
        fprintf(stderr, "MTU must be 128 or 1024") ;
        return -1;
    }

    if (access(filename, R_OK)) {
        fprintf(stderr, "Can not access file %s\n", filename);
        return 0;
    }

    xy->state = XYMODEM_WAIT_START;
    xy->mtu = mtu;
    xy->ttyfd = ttyfd;

    xy->filename = filename;
    xy->openf(xy,filename);

    while (1) {

        if (xymodem_send(xy)) {
            ret = -1;
            break;
        }
        if (xy->state == XYMODEM_DONE)
            break;
    }

    xy->closef(xy);

    return ret;
}
