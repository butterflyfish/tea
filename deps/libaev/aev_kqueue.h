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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "aev.h"

/* A kevent is identified by an <ident, filter> pair. The ident might be
 * a descriptor (file, socket, stream), a process ID or a signal number,
 * depending on what we want to monitor.
 *
 * <filter> does not support OR. It means one EV_SET one kevent invoking
 *
 * calling kevent() on an empty kqueue will block
 */
static inline int _aev_io_action(int kfd, aev_io *w, uint16_t flags)
{
    struct kevent ke;

    if (w->evmask & AEV_READ) {
        EV_SET(&ke, w->fd, EVFILT_READ, flags, 0, 0, w);
        if (kevent(kfd, &ke, 1, NULL, 0, NULL) == -1) return -1;
    }
    if (w->evmask & AEV_WRITE) {
        EV_SET(&ke, w->fd, EVFILT_WRITE, flags, 0, 0, w);
        if (kevent(kfd, &ke, 1, NULL, 0, NULL) == -1) return -1;
    }
    return 0;
}

static inline int _aev_io_start(struct aev_loop *loop, aev_io *w)
{
    int ret;
    ret = _aev_io_action(loop->aevfd, w, EV_ADD);
    if (0 == ret) aev_ref_get(loop);
    return ret;
}

static inline int _aev_io_stop(struct aev_loop *loop, aev_io *w)
{
    int ret;
    ret = _aev_io_action(loop->aevfd, w, EV_DELETE);
    if (0 == ret) aev_ref_put(loop);
    return ret;
}

static inline int _aev_timer_init(aev_timer *w) {

    w->ident = kqueue(); /* using kqueue to create unique ID */
    if (w->ident < 0)
        return w->ident;

    return 0;
}

static inline int _aev_timer_restart(struct aev_loop *loop, aev_timer *w) {

    struct kevent ke;
    uint16_t flags = EV_ADD;

    if ( 0 == w->periodic )
        flags |= EV_ONESHOT;

    EV_SET(&ke, w->ident, EVFILT_TIMER, flags, 0, w->timeout, w);
    if (kevent(loop->aevfd, &ke, 1, NULL, 0, NULL) == -1) return -1;

    return 0;
}

static inline int _aev_timer_start(struct aev_loop *loop, aev_timer *w) {

    int ret;
    ret = _aev_timer_restart(loop, w);
    if (0 == ret) aev_ref_get(loop);
    return ret;
}

static inline int _aev_timer_stop(struct aev_loop *loop, aev_timer *w) {
    struct kevent ke;

    EV_SET(&ke, w->ident, EVFILT_TIMER, EV_DELETE, 0, w->timeout, w);
    if (kevent(loop->aevfd, &ke, 1, NULL, 0, NULL) == -1) return -1;

    aev_ref_put(loop);

    return 0;
}


static inline int _aev_loop_init( struct aev_loop *loop){

    loop->aevfd = kqueue();
    return loop->aevfd;
}

static inline int _aev_run(struct aev_loop *loop){

    int j;
    int evmask = 0;
    int numevents = 0;
    aev_io *io;
    aev_timer *tm;
    struct kevent events[AEV_MAX_EVENT_SIZE];

    numevents = kevent(loop->aevfd, NULL, 0, events, AEV_MAX_EVENT_SIZE, NULL);

    if (numevents < 0)
        return numevents;

    for(j = 0; j < numevents; j++) {
        struct kevent *ke = events+j;

        if ( ke->filter == EVFILT_TIMER ) {
            tm = (aev_timer *)(ke->udata);
            tm->cb(loop, tm);
            if (0 == tm->periodic)
                aev_ref_put(loop);

            continue;
        }

        if (ke->filter == EVFILT_READ) evmask |= AEV_READ;
        if (ke->filter == EVFILT_WRITE) evmask |= AEV_WRITE;
        io = (aev_io*)(ke->udata);
        io->cb(loop, io, evmask);
    }
    return 0;
}
