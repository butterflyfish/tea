#ifndef _TEA_H
#define _TEA_H

#include "serial.h"

typedef struct tea {

    struct aev_loop loop;

    int backlog;

    aev_io telnet;

    /* serial settings */
    int cs;
    int stopbits;
    speed_t speed;
    enum ser_parity p;
    enum ser_flow_ctrl flow;
} tea_t;

#define TEA_ESC_KEY           29 /* Ctrl-] */
#define TEA_ESC_KEY_STR       "Ctrl-]"

#endif
