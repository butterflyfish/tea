#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/queue.h>
#include <stddef.h>
#include <fcntl.h>
#include <ev.h>
#include "xfer.h"
#include "setup.h"

struct emulator {

    int ser_fd;    /* represent serial port */

    /* represent controlling tty */
    int ifd;     /* read from this fd */
    int ofd;     /* write to this fd */

    ev_io ser_w;   /* serial port watcher */
    ev_io tty_w;   /* controling tty watcher */

    SLIST_ENTRY(emulator) node;
};


static SLIST_HEAD(,emulator) em_head;
static struct ev_loop *loop;

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/* read  from serial port and then write to controlling tty */
static void
ser_read (EV_P_ struct ev_io *w, int revents)
{
    int len;
    char buf[1024];
    struct emulator *em = container_of(w, struct emulator, ser_w);

    len = read(em->ser_fd, buf, sizeof buf);
    if( len <= 0) {
        perror(ttyname(em->ser_fd));
        ev_io_stop(EV_A_ &em->ser_w);
        ev_io_stop(EV_A_ &em->tty_w);
    }
    write(em->ofd, buf, len);
}

/* read from controlling tty and then write to serial port */
static void
tty_read (EV_P_ struct ev_io *w, int revents)
{
    int len;
    unsigned char buf;
    struct emulator *em = container_of(w, struct emulator, tty_w);

    len = read(em->ifd, &buf, 1);

     /* esc key: Ctrl-] */
    if ( buf == 29 ) {

        ev_suspend(loop);

        fcntl(em->ifd, F_SETFL, fcntl(em->ifd, F_GETFL, 0) & ~O_NONBLOCK);

        setup_loop(em->ifd, em->ofd, em->ser_fd);

        fcntl(em->ifd, F_SETFL, fcntl(em->ifd, F_GETFL, 0) | O_NONBLOCK);

        ev_resume(loop);

        return;
    }

    write(em->ser_fd, &buf, 1);
}

int
new_emulator(int ser_fd, int ifd, int ofd)
{
    struct emulator *em;

    /* update serial port fd */
    SLIST_FOREACH(em, &em_head, node) {

        if (em->ifd == ifd && em->ofd == ofd) {

            ev_io_stop(loop, &em->ser_w);
            em->ser_fd =  ser_fd;

            ev_io_init(&em->ser_w, ser_read, ser_fd, EV_READ);
            ev_io_start(loop, &em->ser_w);
            return 0;
        }
    }

    em = malloc( sizeof *em);
    em->ser_fd = ser_fd;
    em->ifd = ifd;
    em->ofd = ofd;

    ev_io_init(&em->ser_w, ser_read, ser_fd, EV_READ);
    ev_io_init(&em->tty_w, tty_read, ifd, EV_READ);

    ev_io_start(loop, &em->ser_w);
    ev_io_start(loop, &em->tty_w);

    SLIST_INSERT_HEAD(&em_head, em,node );

    enable_raw_mode(em->ifd);
    fcntl(em->ifd, F_SETFL, fcntl(em->ifd, F_GETFL, 0)|O_NONBLOCK);

    return 0;
}

int
close_emulator(int ifd)
{
    struct emulator *em;
    int ret = -1;

    SLIST_FOREACH(em, &em_head, node) {

        if ( em->ifd == ifd ) {

            ev_io_stop(EV_A_ &em->ser_w);
            ev_io_stop(EV_A_ &em->tty_w);
        }
    }

    if (em) {
        SLIST_REMOVE(&em_head, em, emulator, node);
        ret = 0;
    }

    return ret;
}

void
xfer_init(void)
{
    loop = EV_DEFAULT;
}

void
xfer_loop()
{
    ev_run (loop, 0);

    /* break was called, so exit */
    return;
}
