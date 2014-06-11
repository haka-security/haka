#include "libnetmap.h"

/*
 * how many packets on this set of queues ?
 */
int
pkt_queued(struct nm_desc *d, int tx)
{
        u_int i, tot = 0;

        if (tx) {
                for (i = d->first_tx_ring; i <= d->last_tx_ring; i++) {
                        tot += nm_ring_space(NETMAP_TXRING(d->nifp, i));
                }
        } else {
                for (i = d->first_rx_ring; i <= d->last_rx_ring; i++) {
                        tot += nm_ring_space(NETMAP_RXRING(d->nifp, i));
                }
        }
        return tot;
}
