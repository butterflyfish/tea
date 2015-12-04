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

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_KQUEUE
#include "aev_kqueue.h"
#endif

#ifdef HAVE_EPOLL
#include "aev_epoll.h"
#endif

/* not thread-safe */
int aev_ref_get(struct aev_loop *loop){
    struct aev_ref *ref = &loop->ref;

    ref->count ++;
    return ref->count;
}

/* not thread-safe */
int aev_ref_put(struct aev_loop *loop){
    struct aev_ref *ref = &loop->ref;

    ref->count --;
    if( 0 == ref->count)
        ref->free(loop);
    return ref->count;
}

static void aev_loop_delete(struct aev_loop *loop)
{
    close(loop->aevfd);
}

int aev_loop_init(struct aev_loop *loop){

    memset(loop, 0, sizeof(struct aev_loop));

    loop->ref.count = 0;
    loop->ref.free = aev_loop_delete;

    return _aev_loop_init(loop);
}

int aev_run(struct aev_loop *loop){
    int ret = 0;

    if (loop == NULL || loop->ref.count == 0) return -1;

    while(1) {
        ret = _aev_run(loop);
        if ( ret < 0) break;
    }
    return ret;
}


int aev_io_init(aev_io *w, int fd, aev_io_cb cb,
               int evmask, void *data){

    w->fd = fd;
    w->cb = cb;
    w->evmask = evmask;
    w->data = data;
    return fcntl(fd, F_SETFL, O_NONBLOCK|fcntl(fd,F_GETFL,0));
}

int aev_io_start(struct aev_loop *loop, aev_io *w)
{
    return _aev_io_start(loop,w);
}

int aev_io_stop(struct aev_loop *loop, aev_io *w)
{
    return _aev_io_stop(loop,w);
}



void aev_timer_set(aev_timer *w, unsigned long ms, char periodic) {

    AEV_TIMER_INIT(NULL, ms, periodic);
}

int aev_timer_init(aev_timer *w, aev_timer_cb cb,  unsigned long ms,
                   char periodic) {

    AEV_TIMER_INIT(cb, ms, periodic);

    return _aev_timer_init(w);
}

int aev_timer_restart(struct aev_loop *loop, aev_timer *w) {

    return _aev_timer_restart(loop, w);
}

int aev_timer_start(struct aev_loop *loop, aev_timer *w) {

    int ret;
    ret = _aev_timer_restart(loop, w);
    if (0 == ret) aev_ref_get(loop);
    return ret;
}

int aev_timer_stop(struct aev_loop *loop, aev_timer *w) {

    return _aev_timer_stop(loop, w);
}
