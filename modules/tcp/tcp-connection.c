#include "haka/tcp.h"
#include "haka/tcp-connection.h"
#include <haka/tcp-stream.h>
#include <haka/thread.h>

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
mutex_t ct_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	ptr->tcp_conn.state = 0;
	ptr->tcp_conn.stream_input = tcp_stream_create(tcp_get_seq(tcp)+1);
	ptr->tcp_conn.stream_output = NULL;

	ptr->prev = NULL;

	mutex_lock(&ct_mutex);

	ptr->next = ct_head;

	if (ct_head) {
		ct_head->prev = ptr;
	}

	ct_head = ptr;

	mutex_unlock(&ct_mutex);

	return &ptr->tcp_conn;
}

struct tcp_connection *tcp_connection_get(const struct tcp *tcp, bool *direction_in)
{
	struct ctable *ptr;
	uint16 srcport, dstport;
	ipv4addr srcip, dstip;

	srcip = ipv4_get_src(tcp->packet);
	dstip = ipv4_get_dst(tcp->packet);
	srcport = tcp_get_srcport(tcp);
	dstport = tcp_get_dstport(tcp);

	mutex_lock(&ct_mutex);

	ptr = ct_head;
	while (ptr) {
		if ((ptr->tcp_conn.srcip == srcip) && (ptr->tcp_conn.srcport == srcport) &&
		    (ptr->tcp_conn.dstip == dstip) && (ptr->tcp_conn.dstport == dstport)) {
			mutex_unlock(&ct_mutex);
			if (direction_in) *direction_in = true;
			return &ptr->tcp_conn;
		}
		if ((ptr->tcp_conn.srcip == dstip) && (ptr->tcp_conn.srcport == dstport) &&
		    (ptr->tcp_conn.dstip == srcip) && (ptr->tcp_conn.dstport == srcport)) {
			mutex_unlock(&ct_mutex);
			if (direction_in) *direction_in = false;
			return &ptr->tcp_conn;
		}
		ptr = ptr->next;
	}

	mutex_unlock(&ct_mutex);

	return NULL;
}

void tcp_connection_close(struct tcp_connection* tcp_conn)
{
	struct ctable *current, *next, *prev;

	current = (struct ctable *)((uint8 *)tcp_conn - offsetof(struct ctable, tcp_conn));

	mutex_lock(&ct_mutex);

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

	mutex_unlock(&ct_mutex);

	free(current);
}
