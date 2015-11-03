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

#include <stdio.h>
#include "aev.h"

static void stdin_cb(struct aev_loop *loop, aev_io *w, int evmask)
{
    char buf[1024] = {0};
    puts ("stdin ready");
    aev_io_stop(loop, w);
    fgets(buf, sizeof buf, stdin);
}

static void timer_cb(struct aev_loop *loop, aev_timer *w)
{
    static int counter = 0;
    counter++;
    printf ("timer(ID=%d) is out\n", w->ident);

    if (counter == 2) {
        printf("change timeout of timer(ID=%d) to 2s\n", w->ident);
        aev_timer_set(w, 2000, 1);
        aev_timer_restart(loop, w);
    }

    if(counter >= 3)
        aev_timer_stop(loop, w);
}

static void timer_oneshot(struct aev_loop *loop, aev_timer *w)
{
    printf ("timer(ID=%d) is out, oneshot\n", w->ident);
}


int main(int argc, char *argv[])
{
    struct aev_loop loop;
    struct aev_io w;
    aev_timer tm;
    aev_timer oneshot;

    aev_loop_init(&loop);

    aev_io_init(&w,0,stdin_cb,AEV_READ,NULL);
    aev_io_start(&loop, &w);

    aev_timer_init(&tm,timer_cb,500,1);
    aev_timer_init(&oneshot,timer_oneshot,500,0);

    aev_timer_start(&loop, &tm);
    aev_timer_start(&loop, &oneshot);

    aev_run(&loop);
    return 0;
}
