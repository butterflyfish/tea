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
#include <stdlib.h>

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

int _aev_loop_new( struct aev_loop *loop){

    if (loop->paev == NULL) {
        loop->aevfd = kqueue();
        loop->paev = malloc(loop->setsize * sizeof(struct kevent));
    } else {
        free(loop->paev);
        loop->paev = malloc(loop->setsize * sizeof(struct kevent));
    }

    return loop->paev ? 0 : -1;
}

static inline int _aev_run(struct aev_loop *loop){

    int j;
    int evmask = 0;
    int numevents = 0;
    aev_io *w;
    struct kevent *events = (struct kevent *)(loop->paev);

    numevents = kevent(loop->aevfd, NULL, 0, events, loop->setsize, NULL);

    if (numevents < 0)
        return numevents;

    for(j = 0; j < numevents; j++) {
        struct kevent *ke = events+j;

        if (ke->filter == EVFILT_READ) evmask |= AEV_READ;
        if (ke->filter == EVFILT_WRITE) evmask |= AEV_WRITE;
        w = (aev_io*)(ke->udata);
        w->cb(loop, w, evmask);
    }
    return 0;
}
