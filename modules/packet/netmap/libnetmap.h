#ifndef LIBNETMAP_H
#define LIBNETMAP_H

#include <stdio.h>

#define NETMAP_WITH_LIBS
#include <net/netmap_user.h>

int pkt_queued(struct nm_desc *d, int tx);

#endif // LIBNETMAP_H
