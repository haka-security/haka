#include "haka/tcp.h"
#include "haka/tcp-connection.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <haka/log.h>
#include <haka/error.h>


struct ctable {
	struct ctable         *prev;
	struct ctable         *next;
	struct tcp_connection tcp_conn;
};

static struct ctable *ct_head = NULL;

struct tcp_connection *tcp_connection_new(const struct tcp *tcp)
{
	struct ctable *ptr = malloc(sizeof(struct ctable));
	if (!ptr) {
		error(L"memory error");
		return NULL;
	}

	ptr->tcp_conn.srcip = ipv4_get_src(tcp->packet);
	ptr->tcp_conn.dstip = ipv4_get_dst(tcp->packet);
	ptr->tcp_conn.srcport = tcp_get_srcport(tcp);
	ptr->tcp_conn.dstport = tcp_get_dstport(tcp);
	ptr->prev = NULL;
	ptr->next = ct_head;

	if (ct_head) {
		ct_head->prev = ptr;
	}

	ct_head = ptr;

	return &ct_head->tcp_conn;
}

struct tcp_connection *tcp_connection_get(const struct tcp *tcp)
{
	struct ctable *ptr = ct_head;
	uint16 srcport, dstport;
	ipv4addr srcip, dstip;

	srcip = ipv4_get_src(tcp->packet);
	dstip = ipv4_get_dst(tcp->packet);
	srcport = tcp_get_srcport(tcp);
	dstport = tcp_get_dstport(tcp);

	while (ptr) {
		if (((ptr->tcp_conn.srcip == srcip) && (ptr->tcp_conn.srcport == srcport) &&
			(ptr->tcp_conn.dstip == dstip) && (ptr->tcp_conn.dstport == dstport)) ||
		   ((ptr->tcp_conn.srcip == dstip) && (ptr->tcp_conn.srcport == dstport) &&
            (ptr->tcp_conn.dstip == srcip) && (ptr->tcp_conn.dstport == srcport))) {
			return &ptr->tcp_conn;
		}
		ptr = ptr->next;
	}

	return NULL;
}

void tcp_connection_close(struct tcp_connection* tcp_conn)
{
	struct ctable *current, *next, *prev;

	current = (struct ctable *)((uint8 *)tcp_conn - offsetof(struct ctable, tcp_conn));
	prev = current->prev;
	next = current->next;

	/* removing head */
	if (!prev) {
		ct_head = next;
		if (next)
			next->prev = NULL;
	}
	/* removing tail */
	else if (!next) {
		prev->next = NULL;
	}
	else {
		prev->next = next;
		next->prev = prev;
	}
	free(current);
}
