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

#include <sys/epoll.h>
#include <stdlib.h>

#include "aev.h"

static inline int _aev_io_action(int epfd, aev_io *w, int op)
{
    struct epoll_event ee;

    ee.events = 0;
    ee.data.fd = w->fd;
    ee.data.ptr = w;

    if (w->evmask & AEV_READ) ee.events |= EPOLLIN;
    if (w->evmask & AEV_WRITE) ee.events |= EPOLLOUT;
    if (epoll_ctl(epfd, op, w->fd, &ee) == -1) return -1;

    return 0;
}

static inline int _aev_io_start(struct aev_loop *loop, aev_io *w)
{
    int ret;
    ret = _aev_io_action(loop->aevfd, w, EPOLL_CTL_ADD);
    if (0 == ret) aev_ref_get(loop);
    return ret;
}

static inline int _aev_io_stop(struct aev_loop *loop, aev_io *w)
{
    int ret;
    ret = _aev_io_action(loop->aevfd, w, EPOLL_CTL_DEL);
    if (0 == ret) aev_ref_put(loop);
    return ret;
}

int _aev_loop_new( struct aev_loop *loop){

    if (loop->paev == NULL) {
        loop->aevfd = epoll_create1(0);
        loop->paev = malloc(loop->setsize * sizeof(struct epoll_event));
    } else {
        free(loop->paev);
        loop->paev = malloc(loop->setsize * sizeof(struct epoll_event));
    }

    return loop->paev ? 0 : -1;
}

static inline int _aev_run(struct aev_loop *loop){

    int j;
    int evmask = 0;
    int numevents = 0;
    aev_io *w;
    struct epoll_event *events = (struct epoll_event *)(loop->paev);

    numevents = epoll_wait(loop->aevfd, events, loop->setsize, NULL);

    if (numevents < 0)
        return numevents;

    for(j = 0; j < numevents; j++) {
        struct epoll_event *e = events+j;

        if (e->events &= EPOLLIN) evmask |= AEV_READ;
        if (e->events &= EPOLLOUT) evmask |= AEV_WRITE;
        w = (aev_io*)(e->data.ptr);
        w->cb(loop, w, evmask);
    }
    return 0;
}
