/* telnet.c: map serial over raw telnet

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "aev.h"
#include "terminal.h"
#include "tea.h"


static void
new_telnet_connection(struct aev_loop *loop, aev_io *w, int envmask);


/* create tcp/udp socket and bind to ip/port */
static int
create_and_bind(const char *ip, const char * port, char istcp){

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int ret, fd;
    int yes = 1;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = istcp ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    ret = getaddrinfo(ip, port, &hints, &result);
    if (ret) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    for (rp = result; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if ( fd < 0 ) {
            perror("socket");
            continue;
        }

        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            close(fd);
            return -1;
        }

        ret = bind(fd, rp->ai_addr, rp->ai_addrlen);
        if ( ret == 0)
            break;

        perror("bind");
        close(fd);
    }

    if (rp == NULL)
    {
      fprintf (stderr, "create_and_bind: could not bind\n");
      return -1;
    }

    freeaddrinfo (result);

    return fd;
}

int
start_telnet_server(tea_t *tea, const char *ip, const char * port) {

    int fd;

    fd = create_and_bind(ip, port, 1);
    if (fd == -1)
        return -1;

    if ( listen(fd, tea->backlog) < 0 ) {
        perror("listen");
        return -1;
    }

    aev_io_init(&tea->telnet, fd, new_telnet_connection, AEV_READ, tea);
    aev_io_start(&tea->loop, &tea->telnet);
    return 0;
}

static void
new_telnet_connection(struct aev_loop *loop, aev_io *w, int envmask){

    struct sockaddr addr;
    socklen_t len = sizeof addr;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    int cfd;
    int ret;
    tea_t *tea = w->data;

    cfd = accept (w->fd, &addr, &len);
    ret = getnameinfo (&addr, len,
                           hbuf, sizeof hbuf,
                           sbuf, sizeof sbuf,
                           NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret == 0)
    {
        printf("Accepted connection on socket fd %d "
             "(host=%s, port=%s)\n", cfd, hbuf, sbuf);
    }

    new_terminal(tea, cfd, cfd);
}
