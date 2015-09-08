/*

kermit.c: send binary file based on e-kermit
          some code is borrowed from deps/ek/unixio.c

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

/* unistd.h and kermit.h both define X_OK */
#undef X_OK

#include "cdefs.h"  /* Data types for all modules */
#include "kermit.h" /* Kermit symbols and data structures */
#include "ek_file.h"

static struct k_data k;                        /* Kermit data structure */
static struct k_response r;                    /* Kermit response structure */


int xmode = 0;                /* File-transfer mode */
int remote = 1;                /* 1 = Remote, 0 = Local */
#ifdef F_CRC
int check = 3;                /* Block check */
#else
int check = 1;
#endif /* F_CRC */

static UCHAR o_buf[OBUFLEN+8];            /* File output buffer */
static UCHAR i_buf[IBUFLEN+8];            /* File output buffer */

/*
* the output file is unbuffered to ensure that every output byte is commited.
* The input file, however, is buffered for speed. This is just one of many
* possible implmentation choices, invisible to the Kermit protocol module.
*/
static int ttyfd, ofile = -1;        /* File descriptors */
static FILE * ifile = (FILE *)0;    /* and pointers */

static int
readpkt(struct k_data * k, UCHAR *p, int len) {
    int n;
    char x;
    short flag;
    UCHAR c;
#ifdef F_CTRLC
    short ccn;
    ccn = 0;
#endif /* F_CTRLC */

    flag = n = 0;                       /* Init local variables */

    while (1) {
        read(ttyfd, &x, 1);
        c = (k->parity) ? x & 0x7f : x & 0xff; /* Strip parity */

#ifdef F_CTRLC
        /* In remote mode only: three consecutive ^C's to quit */
        if (k->remote && c == (UCHAR) 3) {
            if (++ccn > 2) {
                return(-1);
        }
        } else {
        ccn = 0;
    }
#endif /* F_CTRLC */

        if (!flag && c != k->r_soh)    /* No start of packet yet */
            continue;                     /* so discard these bytes. */
        if (c == k->r_soh) {        /* Start of packet */
            flag = 1;                   /* Remember */
            continue;                   /* But discard. */
        } else if (c == k->r_eom    /* Packet terminator */
           || c == '\012'    /* 1.3: For HyperTerminal */
           ) {
            return(n);
        } else {                        /* Contents of packet */
            if (n++ > k->r_maxlen)    /* Check length */
              return(0);
            else
              *p++ = x & 0xff;
        }
    }
    return(-1);
}

/* Writes n bytes of data to communication device.  */
static int
tx_data(struct k_data * k, UCHAR *p, int n) {
    int x;
    int max;

    max = 10;                           /* Loop breaker */

    while (n > 0) {                     /* Keep trying till done */
        x = write(ttyfd,p,n);
        if (x < 0 || --max < 1)         /* Errors are fatal */
          return(X_ERROR);
        n -= x;
    p += x;
    }
    return(X_OK);                       /* Success */
}

/* Open output file  */
static int
openfile(struct k_data * k, UCHAR * s, int mode) {

    switch (mode) {
        case 1:                /* Read */
            if (!(ifile = fopen(s,"r"))) {
                return(X_ERROR);
            }
            k->s_first   = 1;        /* Set up for getkpt */
            k->zinbuf[0] = '\0';        /* Initialize buffer */
            k->zinptr    = k->zinbuf;    /* Set up buffer pointer */
            k->zincnt    = 0;        /* and count */
            return(X_OK);

        case 2:                /* Write (create) */
            ofile = creat(s,0644);
            if (ofile < 0) {
                return(X_ERROR);
            }
            return(X_OK);

        default: return(X_ERROR);
    }
}

/*  Get info about existing file
 *  only invokde if attribute capability is negotiated
 */
unsigned long
fileinfo(struct k_data * k,
     char * filename, UCHAR * buf, int buflen, short * type, short mode) {

#define SCANBUF 1024
#define SCANSIZ 49152

    struct stat statbuf;
    struct tm * timestamp, * localtime();
    int isbinary = 1;

    FILE * fp;                /* File scan pointer */
    char inbuf[SCANBUF];        /* and buffer */

    if (!buf)
        return(X_ERROR);
    buf[0] = '\0';
    if (buflen < 18)
        return(X_ERROR);
    if (stat(filename,&statbuf) < 0)
        return(X_ERROR);
    timestamp = localtime(&(statbuf.st_mtime));
    sprintf(buf,"%04d%02d%02d %02d:%02d:%02d",
        timestamp->tm_year + 1900,
            timestamp->tm_mon + 1,
            timestamp->tm_mday,
            timestamp->tm_hour,
            timestamp->tm_min,
            timestamp->tm_sec
        );

    /*
      Here we determine if the file is text or binary if the transfer mode is
      not forced.  This is an extremely crude sample, which diagnoses any file
      that contains a control character other than HT, LF, FF, or CR as binary.
      A more thorough content analysis can be done that accounts for various
      character sets as well as various forms of Unicode (UTF-8, UTF-16, etc).
      Or the diagnosis could be based wholly or in part on the filename.
      etc etc.  Or the implementation could skip this entirely by not defining
      F_SCAN and/or by always calling this routine with type set to -1.
    */
    if (!mode) {            /* File type determination requested */
        fp = fopen(filename,"r");    /* Open the file for scanning */
        if (fp) {
            int n = 0, count = 0;
            char c, * p;

            isbinary = 0;
            while (count < SCANSIZ && !isbinary) { /* Scan this much */
            n = fread(inbuf,1,SCANBUF,fp);
            if (n == EOF || n == 0)
              break;
            count += n;
            p = inbuf;
            while (n--) {
                c = *p++;
                if (c < 32 || c == 127) {
                    if (c !=  9 &&    /* Tab */
                        c != 10 &&    /* LF */
                        c != 12 &&    /* FF */
                        c != 13) {    /* CR */
                            isbinary = 1;
                            break;
                        }
                    }
                }
            }
            fclose(fp);
            *type = isbinary;
        }
    }

    return((unsigned long)(statbuf.st_size));
}


/*  Read data from a file  */
int
readfile(struct k_data * k) {
    if (!k->zinptr) {
        return(X_ERROR);
    }
    if (k->zincnt < 1) {        /* Nothing in buffer - must refill */
        if (k->binary) {        /* Binary - just read raw buffers */
            k->dummy = 0;
            k->zincnt = fread(k->zinbuf, 1, k->zinlen, ifile);
        } else {            /* Text mode needs LF/CRLF handling */
            int c;            /* Current character */
            for (k->zincnt = 0; (k->zincnt < (k->zinlen - 2)); (k->zincnt)++) {
                if ((c = getc(ifile)) == EOF)
                    break;
                if (c == '\n')        /* Have newline? */
                    k->zinbuf[(k->zincnt)++] = '\r'; /* Insert CR */
                k->zinbuf[k->zincnt] = c;
            }
        }
        k->zinbuf[k->zincnt] = '\0';    /* Terminate. */
        if (k->zincnt == 0)        /* Check for EOF */
            return(-1);
        k->zinptr = k->zinbuf;        /* Not EOF - reset pointer */
    }
    (k->zincnt)--;            /* Return first byte. */

    return(*(k->zinptr)++ & 0xff);
}



int
writefile(struct k_data * k, UCHAR * s, int n) {
    fprintf(stderr, "kermit: don't support receive file\n");
    return n;
}

int
closefile(struct k_data * k, UCHAR c, int mode) {
    int rc = X_OK;            /* Return code */

    switch (mode) {
    case 1:                /* Closing input file */
        if (!ifile)            /* If not not open */
            break;            /* do nothing but succeed */
        if (fclose(ifile) < 0)
            rc = X_ERROR;
        break;
    case 2:                /* Closing output file */
    case 3:
        if (ofile < 0)            /* If not open */
            break;            /* do nothing but succeed */
        if (close(ofile) < 0) {        /* Try to close */
            rc = X_ERROR;
        } else if ((k->ikeep == 0) &&    /* Don't keep incomplete files */
               (c == 'D')) {    /* This file was incomplete */
                if (k->filename) {
                    unlink(k->filename);    /* Delete it. */
                }
        }
        break;
    default: rc = X_ERROR;
    }
    return(rc);
}

int
kermit_send_file(int ofd, char ** filelist) {

    int status, rx_len;
    UCHAR *inbuf;
    short r_slot;

    int len = 0;

    ttyfd = ofd;
    status = X_OK;                      /* Initial kermit status */


    /*  Fill in parameters for this run */
    k.xfermode = xmode;            /* Text/binary automatic/manual  */
    k.binary = 1;            /* 0 = text, 1 = binary */
    k.remote = remote;            /* Remote vs local */
    k.parity = P_PARITY;                  /* Set this to desired parity */
    k.bct = (check == 5) ? 3 : check;    /* Block check type */
    k.ikeep = 0;            /* don't Keep incompletely received files */
    k.filelist = filelist;        /* List of files to send (if any) */
    k.cancel = 0;            /* Not canceled yet */

    /*  Fill in the i/o pointers  */
    k.zinbuf = i_buf;            /* File input buffer */
    k.zinlen = IBUFLEN;            /* File input buffer length */
    k.zincnt = 0;            /* File input buffer position */
    k.obuf = o_buf;            /* File output buffer */
    k.obuflen = OBUFLEN;        /* File output buffer length */
    k.obufpos = 0;            /* File output buffer position */

    /* Fill in function pointers */
    k.rxd    = readpkt;            /* for reading packets */
    k.txd    = tx_data;            /* for sending packets */
    k.openf  = openfile;                /* for opening files */
    k.finfo  = fileinfo;                /* for getting file info */
    k.readf  = readfile;        /* for reading files */
    k.writef = writefile;               /* for writing to output file */
    k.closef = closefile;               /* for closing files */
    k.dbf    = 0;

    /* Force Type 3 Block Check (16-bit CRC) on all packets, or not */
    k.bctf   = (check == 5) ? 1 : 0;

    /* Initialize Kermit protocol */
    status = kermit(K_INIT, &k, 0, 0, "", &r);
    if (status == X_ERROR)
        return -1;
    status = kermit(K_SEND, &k, 0, 0, "", &r);
    while (status != X_DONE) {

        /* TODO:
         * add timeout on ttyfd
         */

        inbuf = getrslot(&k,&r_slot);    /* Allocate a window slot */
        rx_len = k.rxd(&k,inbuf,P_PKTLEN); /* Try to read a packet */

        if (rx_len < 1) {               /* No data was read */
            freerslot(&k,r_slot);    /* So free the window slot */
            if (rx_len < 0)             /* If there was a fatal error */
                return -1;
        }
        switch (status = kermit(K_RUN, &k, r_slot, rx_len, "", &r)) {
            case X_OK:
                /*
                * This shows how, after each packet, you get the protocol state, file name,
                * date, size, and bytes transferred so far.  These can be used in a
                * file-transfer progress display, log, etc.
                */
                if(0 == r.sofar)
                    printf("Sending file %s .... %%",r.filename);
                for(int i=0;i<len;i++)
                    putchar('\b');
                len=printf("%d",(int)(r.sofar*100/r.filesize));
                fflush(stdout);

                continue;            /* Keep looping */

            case X_DONE:
                putchar('\n');
                break;    /* Finished */
            case X_ERROR:
                return -1;
        }
    }
    return 0;
}
