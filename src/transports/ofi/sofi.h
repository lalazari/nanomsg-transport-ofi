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
#include "../../aio/worker.h"
#include "../../utils/thread.h"
#include "../../utils/mutex.h"

/* SOFI Source */
#define NN_BTCP_SRC_SOFI     1000

/* Events from the SOFI */
#define NN_SOFI_DISCONNECTED 2001
#define NN_SOFI_STOPPED      2002

/**
 * Various system structures stored in a shared memory region
 */
struct nn_ofi_sys_ptrs {
    uint8_t inhdr  [8];  /* Input header */
    uint8_t outhdr [8];  /* Out header */
    uint8_t sphdr  [64]; /* Protocol header for various uses */
};

/* Shared, Connected OFI FSM */
struct nn_sofi {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state, instate, outstate;
    int error;

    /*  Pipe connecting this inproc connection to the nanomsg core. */
    struct nn_pipebase pipebase;

    /*  Events generated by aofi state machine. */
    struct nn_fsm_event disconnected;

    /* The high-level api structures */
    struct ofi_resources        * ofi;
    struct ofi_active_endpoint  * ep;

    /* Input buffers */
    struct nn_msg inmsg;
    struct nn_chunk * inmsg_chunk;
    struct ofi_mr * mr_inmsg;

    /* Output buffers */
    struct nn_msg outmsg;

    /* This member can be used by owner to keep individual atcps in a list. */
    struct nn_list_item item;

    /* Asynchronous communication with the worker thread */
    struct nn_worker *worker;
    struct nn_worker_task task_rx;
    struct nn_worker_task task_tx;
    struct nn_worker_task task_error;
    struct nn_worker_task task_disconnect;

    /* Shutdown timer */
    struct nn_timer shutdown_timer;

    /* Keepalive configuration */
    struct nn_timer keepalive_timer;
    uint8_t keepalive_tx_ctr;
    uint8_t keepalive_rx_ctr;

    /* The OFI Poller thread and sync efd */
    struct nn_thread thread;
    struct nn_mutex rx_sync;
    struct nn_mutex tx_sync;

    /* First draft of smart MM */
    int                     slab_size, recv_buffer_size;
    struct ofi_mr           *mr_slab, *mr_user;
    void                    *mr_slab_ptr, /* *mr_slab_data_in, */ *ptr_slab_out, *mr_slab_inmsg;
    struct nn_ofi_sys_ptrs  *ptr_slab_sysptr;

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
