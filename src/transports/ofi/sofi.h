/*
    Copyright (c) 2015 Ioannis Charalampidis  All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#ifndef NN_SOFI_INCLUDED
#define NN_SOFI_INCLUDED

#include "hlapi.h"

#include "../../transport.h"
#include "../../aio/fsm.h"
#include "../../aio/timer.h"
#include "../../utils/mutex.h"
#include "../../utils/thread.h"

/* SOFI Source */
#define NN_BTCP_SRC_SOFI     1000

/* Events from the SOFI */
#define NN_SOFI_DISCONNECTED 2001
#define NN_SOFI_STOPPED      2002

/* Shared, Connected OFI FSM */
struct nn_sofi {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  Pipe connecting this inproc connection to the nanomsg core. */
    struct nn_pipebase pipebase;

    /*  Events generated by aofi state machine. */
    struct nn_fsm_event disconnected;

    /* The high-level api structures */
    struct ofi_resources        * ofi;
    struct ofi_active_endpoint  * ep;

    /* Input buffers */
    struct nn_msg inmsg;

    /* Output buffers */
    struct nn_msg outmsg;
    uint8_t outhdr [8];

    /* This member can be used by owner to keep individual atcps in a list. */
    struct nn_list_item item;

    /* Timer used to notify the termination of the socket thread */
    struct nn_timer disconnect_timer;

    /* The OFI Poller thread */
    struct nn_thread thread;
    
};

/*  Initialize the state machine */
void nn_sofi_init (struct nn_sofi *self, 
    struct ofi_resources *ofi, struct ofi_active_endpoint *ep, 
    struct nn_epbase *epbase, int src, struct nn_fsm *owner);

/* Check if FSM is idle */
int nn_sofi_isidle (struct nn_sofi *self);

/*  Stop the state machine */
void nn_sofi_stop (struct nn_sofi *sofi);

/*  Cleanup the state machine */
void nn_sofi_term (struct nn_sofi *sofi);

#endif
