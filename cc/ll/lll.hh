#ifndef _LLL_HH_
#define _LLL_HH_

#include "ll.hh"

/* Exported function by Lower Link Layer */

/* Init lll */
extern void lll_init(ll_dev &dev);

/* Clear lll */
extern void lll_clear(ll_dev &dev);

/* lll send/recv */
extern bool lll_send(ll_dev &dev, ll_pdu &pdu);

extern bool lll_recv(ll_dev &dev, ll_pdu &pdu);

#endif

