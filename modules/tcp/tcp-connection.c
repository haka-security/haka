#include "haka/tcp.h"
#include "haka/tcp-connection.h"
#include <haka/tcp-stream.h>
#include <haka/thread.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#include <haka/log.h>
#include <haka/error.h>


struct ctable {
	struct ctable         *prev;
	struct ctable         *next;
	struct tcp_connection tcp_conn;
	bool                  dropped;
};

static struct ctable *ct_head = NULL;

mutex_t ct_mutex = PTHREAD_MUTEX_INITIALIZER;


void tcp_connection_insert(struct ctable **head, struct ctable *elem)
{
	mutex_lock(&ct_mutex);

	elem->next = *head;
	if (*head) (*head)->prev = elem;
	*head = elem;

	mutex_unlock(&ct_mutex);
}

static struct ctable *tcp_connection_find(struct ctable *head, const struct tcp *tcp,
		bool *direction_in, bool *dropped)
{
	struct ctable *ptr;
	uint16 srcport, dstport;
	ipv4addr srcip, dstip;

	assert(tcp);

	srcip = ipv4_get_src(tcp->packet);
	dstip = ipv4_get_dst(tcp->packet);
	srcport = tcp_get_srcport(tcp);
	dstport = tcp_get_dstport(tcp);

	mutex_lock(&ct_mutex);

	ptr = head;
	while (ptr) {
		if ((ptr->tcp_conn.srcip == srcip) && (ptr->tcp_conn.srcport == srcport) &&
		    (ptr->tcp_conn.dstip == dstip) && (ptr->tcp_conn.dstport == dstport)) {
			mutex_unlock(&ct_mutex);
			if (direction_in) *direction_in = true;
			if (dropped) *dropped = ptr->dropped;
			return ptr;
		}
		if ((ptr->tcp_conn.srcip == dstip) && (ptr->tcp_conn.srcport == dstport) &&
		    (ptr->tcp_conn.dstip == srcip) && (ptr->tcp_conn.dstport == srcport)) {
			mutex_unlock(&ct_mutex);
			if (direction_in) *direction_in = false;
			if (dropped) *dropped = ptr->dropped;
			return ptr;
		}
		ptr = ptr->next;
	}

	mutex_unlock(&ct_mutex);

	if (dropped) *dropped = false;
	return NULL;
}

static void tcp_connection_remove(struct ctable **head, struct ctable *elem)
{
	mutex_lock(&ct_mutex);

	if (elem->prev) {
		elem->prev->next = elem->next;
	}
	else {
		assert(*head == elem);
		*head = elem->next;
	}

	if (elem->next) elem->next->prev = elem->prev;

	mutex_unlock(&ct_mutex);

	elem->prev = NULL;
	elem->next = NULL;
}

static void tcp_connection_release(struct ctable *elem, bool freemem)
{
	lua_ref_clear(&elem->tcp_conn.lua_table);

	if (elem->tcp_conn.stream_input) {
		stream_destroy(elem->tcp_conn.stream_input);
		elem->tcp_conn.stream_input = NULL;
	}

	if (elem->tcp_conn.stream_output) {
		stream_destroy(elem->tcp_conn.stream_output);
		elem->tcp_conn.stream_output = NULL;
	}

	if (freemem) {
		lua_object_release(&elem->tcp_conn, &elem->tcp_conn.lua_object);
		free(elem);
	}
}


struct tcp_connection *tcp_connection_new(const struct tcp *tcp)
{
	struct ctable *ptr;
	bool dropped;

	ptr = tcp_connection_find(ct_head, tcp, NULL, &dropped);
	if (ptr) {
		if (dropped) {
			tcp_connection_remove(&ct_head, ptr);
			tcp_connection_release(ptr, true);
		}
		else {
			error(L"connection already exists");
			return NULL;
		}
	}

	ptr = malloc(sizeof(struct ctable));
	if (!ptr) {
		error(L"memory error");
		return NULL;
	}

	lua_object_init(&ptr->tcp_conn.lua_object);
	ptr->tcp_conn.srcip = ipv4_get_src(tcp->packet);
	ptr->tcp_conn.dstip = ipv4_get_dst(tcp->packet);
	ptr->tcp_conn.srcport = tcp_get_srcport(tcp);
	ptr->tcp_conn.dstport = tcp_get_dstport(tcp);
	lua_ref_init(&ptr->tcp_conn.lua_table);
	ptr->tcp_conn.stream_input = tcp_stream_create();
	ptr->tcp_conn.stream_output = tcp_stream_create();
	ptr->dropped = false;

	ptr->prev = NULL;
	ptr->next = NULL;

	tcp_connection_insert(&ct_head, ptr);

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(ptr->tcp_conn.srcip, srcip, IPV4_ADDR_STRING_MAXLEN);
		ipv4_addr_to_string(ptr->tcp_conn.dstip, dstip, IPV4_ADDR_STRING_MAXLEN);

		messagef(HAKA_LOG_DEBUG, L"tcp-connection", L"opening connection %s:%u -> %s:%u",
				srcip, ptr->tcp_conn.srcport, dstip, ptr->tcp_conn.dstport);
	}

	return &ptr->tcp_conn;
}

struct tcp_connection *tcp_connection_get(const struct tcp *tcp, bool *direction_in,
		bool *_dropped)
{
	bool dropped;
	struct ctable *elem = tcp_connection_find(ct_head, tcp, direction_in, &dropped);
	if (elem) {
		if (dropped) {
			if (_dropped) *_dropped = dropped;
			return NULL;
		}
		else {
			return &elem->tcp_conn;
		}
	}
	else {
		return NULL;
	}
}

void tcp_connection_close(struct tcp_connection* tcp_conn)
{
	struct ctable *current = (struct ctable *)((uint8 *)tcp_conn - offsetof(struct ctable, tcp_conn));

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(current->tcp_conn.srcip, srcip, IPV4_ADDR_STRING_MAXLEN);
		ipv4_addr_to_string(current->tcp_conn.dstip, dstip, IPV4_ADDR_STRING_MAXLEN);

		messagef(HAKA_LOG_DEBUG, L"tcp-connection", L"closing connection %s:%u -> %s:%u",
				srcip, current->tcp_conn.srcport, dstip, current->tcp_conn.dstport);
	}

	tcp_connection_remove(&ct_head, current);
	tcp_connection_release(current, true);
}

void tcp_connection_drop(struct tcp_connection *tcp_conn)
{
	struct ctable *current = (struct ctable *)((uint8 *)tcp_conn - offsetof(struct ctable, tcp_conn));

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(current->tcp_conn.srcip, srcip, IPV4_ADDR_STRING_MAXLEN);
		ipv4_addr_to_string(current->tcp_conn.dstip, dstip, IPV4_ADDR_STRING_MAXLEN);

		messagef(HAKA_LOG_DEBUG, L"tcp-connection", L"dropping connection %s:%u -> %s:%u",
				srcip, current->tcp_conn.srcport, dstip, current->tcp_conn.dstport);
	}

	tcp_connection_release(current, false);
	current->dropped = true;
}
