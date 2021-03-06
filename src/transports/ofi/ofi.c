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

#include "ofi.h"
#include "bofi.h"
#include "cofi.h"

#include "../../ofi.h"
#include "../../utils/err.h"
#include "../../utils/alloc.h"
#include "../../utils/fast.h"
#include "../../utils/cont.h"

#include <string.h>

/* OFI-Specific socket options */
struct nn_ofi_optset {
    struct nn_optset base;
    int slab_mr_size;
};

/* Optset interface */
static struct nn_optset *nn_ofi_optset (void);
static void nn_ofi_optset_destroy (struct nn_optset *self);
static int nn_ofi_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen);
static int nn_ofi_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen);
static const struct nn_optset_vfptr nn_ofi_optset_vfptr = {
    nn_ofi_optset_destroy,
    nn_ofi_optset_setopt,
    nn_ofi_optset_getopt
};

/*  nn_transport interface. */
static int  nn_ofi_bind (void *hint, struct nn_epbase **epbase);
static int  nn_ofi_connect (void *hint, struct nn_epbase **epbase);

/**
 * Expose the OFI transport pointer table
 */
static struct nn_transport nn_ofi_vfptr = {
    "ofi",
    NN_OFI,
    NULL,
    NULL,
    nn_ofi_bind,
    nn_ofi_connect,
    nn_ofi_optset,
    NN_LIST_ITEM_INITIALIZER
};

struct nn_transport *nn_ofi = &nn_ofi_vfptr;

/**
 * Create a new bind socket
 */
static int nn_ofi_bind (void *hint, struct nn_epbase **epbase)
{
    return nn_bofi_create(hint, epbase);
}

/**
 * Create a new connected socket
 */
static int nn_ofi_connect (void *hint, struct nn_epbase **epbase)
{
    return nn_cofi_create(hint, epbase);
}

/**
 * Create and return a new nn_ofi_optset
 */
static struct nn_optset *nn_ofi_optset (void)
{
    struct nn_ofi_optset *optset;

    optset = nn_alloc (sizeof (struct nn_ofi_optset), "optset (ofi)");
    alloc_assert (optset);
    optset->base.vfptr = &nn_ofi_optset_vfptr;

    /*  Default values for OFI socket options. */
    optset->slab_mr_size = NN_OFI_DEFAULT_SLABMR_SIZE;

    return &optset->base;
}

static void nn_ofi_optset_destroy (struct nn_optset *self)
{
    struct nn_ofi_optset *optset;

    optset = nn_cont (self, struct nn_ofi_optset, base);
    nn_free (optset);
}

static int nn_ofi_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen)
{
    struct nn_ofi_optset *optset;
    int val;

    optset = nn_cont (self, struct nn_ofi_optset, base);

    /*  At this point we assume that all options are of type int. */
    if (optvallen != sizeof (int))
        return -EINVAL;
    val = *(int*) optval;

    switch (option) {
    case NN_OFI_SLABMR_SIZE:
        if (nn_slow (val != 0 && val != 1))
            return -EINVAL;
        optset->slab_mr_size = val;
        return 0;
    default:
        return -ENOPROTOOPT;
    }
}

static int nn_ofi_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen)
{
    struct nn_ofi_optset *optset;
    int intval;

    optset = nn_cont (self, struct nn_ofi_optset, base);

    switch (option) {
    case NN_OFI_SLABMR_SIZE:
        intval = optset->slab_mr_size;
        break;
    default:
        return -ENOPROTOOPT;
    }
    memcpy (optval, &intval,
        *optvallen < sizeof (int) ? *optvallen : sizeof (int));
    *optvallen = sizeof (int);
    return 0;
}

