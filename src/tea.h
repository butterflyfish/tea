#ifndef _TEA_H
#define _TEA_H

#include "serial.h"

typedef struct tea {

    struct aev_loop loop;

    /* serial settings */
    int cs;
    int stopbits;
    speed_t speed;
    enum ser_parity p;
    enum ser_flow_ctrl flow;
} tea_t;

#endif
