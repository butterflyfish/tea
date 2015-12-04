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
#include <sys/timerfd.h>

#include "aev.h"

static inline int _aev_io_action(int epfd, aev_io *w, int op)
{
    struct epoll_event ee;

    ee.events = 0;
    ee.data.ptr = w;

    if (w->evmask & AEV_READ) ee.events |= EPOLLIN;
    if (w->evmask & AEV_WRITE) ee.events |= EPOLLOUT;

    return epoll_ctl(epfd, op, w->fd, &ee);
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

static inline void io_cb(struct aev_loop *loop, void *p) {

    aev_io *w;
    int evmask = 0;
    struct epoll_event *e = (struct epoll_event *)p;

    if (e->events &= EPOLLIN) evmask |= AEV_READ;
    if (e->events &= EPOLLOUT) evmask |= AEV_WRITE;

    w = (aev_io*)(e->data.ptr);
    w->cb(loop, w, evmask);
}

static inline int _aev_timer_init(aev_timer *w) {

    w->ident = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
    if (w->ident < 0)
        return w->ident;

    return 0;
}

static inline int _aev_timer_restart(struct aev_loop *loop, aev_timer *w) {

    struct itimerspec its;
    int ret = 0;
    struct epoll_event ee;

    ee.events = EPOLLIN;
    ee.data.ptr = w;

    /* convert ms to timespec */
    its.it_value.tv_sec = w->timeout / 1000;
    its.it_value.tv_nsec = (w->timeout % 1000) * 1000000;

    if ( AEV_TIMER_ONESHOT == w->evmask) {

        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        ee.events |= EPOLLONESHOT;
    } else {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    }

    ret = epoll_ctl(loop->aevfd, EPOLL_CTL_ADD, w->ident, &ee);
    if ( ret < 0 )
        return ret;

    ret = timerfd_settime(w->ident, 0, &its, NULL);
    if ( ret < 0 )
        return ret;

    return ret;
}

static inline int _aev_timer_stop(struct aev_loop *loop, aev_timer *w) {

    struct epoll_event ee;
    int ret = 0;

    /* In kernel versions before 2.6.9, the EPOLL_CTL_DEL operation required
    a non-null pointer in event, even though this argument is ignored.
    Since Linux 2.6.9, event can be specified as NULL when using
    EPOLL_CTL_DEL.
    */
    ret = epoll_ctl(loop->aevfd, EPOLL_CTL_DEL, w->ident, &ee);
    if (0 == ret) aev_ref_put(loop);
    return ret;
}

static inline void timer_expire_cb(struct aev_loop *loop, void *p)
{
    aev_timer *w = (aev_timer*)p;
    uint64_t exp;

    read(w->ident, &exp, sizeof(uint64_t));
    w->cb(loop, w);
    if ( AEV_TIMER_ONESHOT == w->evmask)
        aev_ref_put(loop);
}

static inline int _aev_loop_init( struct aev_loop *loop){

    loop->aevfd = epoll_create1(0);
    return loop->aevfd;
}

static inline int _aev_run(struct aev_loop *loop){

    int j;
    int numevents = 0;
    aev_io *w;
    struct epoll_event events[AEV_MAX_EVENT_SIZE];
    struct epoll_event *e;

    numevents = epoll_wait(loop->aevfd, events, AEV_MAX_EVENT_SIZE, 0);

    if (numevents < 0)
        return numevents;

    for(j = 0; j < numevents; j++) {
        e = events+j;
        w = (aev_io *)(e->data.ptr);

        if (w->evmask & AEV_TIMER_PERIODIC | w->evmask & AEV_TIMER_ONESHOT) {
            timer_expire_cb(loop, e->data.ptr);
        }
        else {
            io_cb(loop, e);
        }
    }
    return 0;
}
