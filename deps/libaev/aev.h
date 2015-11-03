/*
simple event library, but hight performance

Copyright © 2015 Michael Zhu <boot2linux@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the libaev nor the
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

#ifndef _AEV_H_
#define _AEV_H_

#include <sys/time.h>

#define AEV_NONE       0
#define AEV_READ       1
#define AEV_WRITE      2


#define AEV_MAX_EVENT_SIZE 1024

/* event loop structure */
struct aev_loop;

/* reference count */
struct aev_ref {
    int count;
    void (*free)(struct aev_loop *loop);
};

/* event loop structure */
struct aev_loop {

    struct aev_ref ref; /* reference counter */

    /* platform dependent data */
    int aevfd;
    void *paev;
};

/* aev_loop_init: initialize event loop */
int aev_loop_init(struct aev_loop *loop);


/* aev_run: loop routine of event loop
 *
 * it exits automatically, and delete event loop @loop
 *
 * @loop: point to event loop
 */
int aev_run(struct aev_loop *loop);


/* aev_ref_get: increase reference count on the loop */
int aev_ref_get(struct aev_loop *loop);


/* aev_ref_put: decrease reference count on the loop */
int aev_ref_put(struct aev_loop *loop);





/* type of IO watcher */
typedef struct aev_io aev_io;

/* callback function type of IO watcher */
typedef void (*aev_io_cb)(struct aev_loop *loop, aev_io *w, int evmask);

/* IO event watcher */
struct aev_io {

    int fd;          /* file descriptor */
    int evmask;      /* event mask */
    aev_io_cb cb;   /* fd handler */
    void *data;      /* user data */
};


/* aev_io_init: initialize IO watcher
 *
 * IO watcher contains file dscriptior, callback function, and event mask.
 * It will set @fd to non-block.
 *
 * @w: pointer of watcher
 * @fd: file descriptor
 * @cb: callback function
 * @evmask: event mask
 * @data: user data
 */
int aev_io_init(aev_io *w, int fd, aev_io_cb cb,
               int evmask, void *data);


/* aev_io_start: add the watcher to loop and then start watcher
 *
 * @loop: which loop is  attached
 * @w: wathcher
 */
int aev_io_start(struct aev_loop *loop, aev_io *w);


/* aev_io_stop: delete the watcher from loop
 *
 * @loop: which loop is  attached
 * @w: wathcher
 */
int aev_io_stop(struct aev_loop *loop, aev_io *w);




typedef struct aev_timer aev_timer;

/* callback function type of IO watcher */
typedef void (*aev_timer_cb)(struct aev_loop *loop, aev_timer *w);

struct aev_timer {

    int ident;                /* id of timer */
    unsigned long timeout;    /* millisecond */
    int periodic;             /* bool. default, timer is oneshot */
    aev_timer_cb cb;          /* fd handler */
    void *data;               /* user data */
};

#define AEV_TIMER_INIT(CB, MS, PERIODIC) \
    do {        \
        if(CB) w->cb = CB; \
        w->timeout = MS; \
        w->periodic = PERIODIC; \
    }while(0)


/* aev_timer_init: initialize timer watcher
 *
 * @w: timer watcher
 * @cb: callback function invoked after timeout
 * @ms: timeout
 * @periodic: if it's 0, timer is oneshot, else it's periodic
 */
int aev_timer_init(aev_timer *w, aev_timer_cb cb, unsigned long ms,
                   char periodic);


/* aev_timer_set: adjust timer attribute of timer watcher
 *
 * it needs to invoide aev_timer_restart to apply latest settings
 *
 * @ms: timeout
 * @periodic: if it's 0, timer is oneshot, else it's periodic
 */
void aev_timer_set(aev_timer *w, unsigned long ms, char periodic);


/* aev_timer_start: start timer watcher */
int aev_timer_start(struct aev_loop *loop, aev_timer *w);


/* aev_timer_restart: restart timer watcher */
int aev_timer_restart(struct aev_loop *loop, aev_timer *w);


/* aev_timer_stop: stop timer watcher */
int aev_timer_stop(struct aev_loop *loop, aev_timer *w);



#endif
