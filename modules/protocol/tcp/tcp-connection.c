/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "haka/tcp.h"
#include "haka/tcp-connection.h"
#include <haka/tcp-stream.h>
#include <haka/thread.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/container/hash.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>


struct ctable {
	hash_head_t           hh;
	struct tcp_connection tcp_conn;
	bool                  dropped;
};

static const size_t hash_keysize = 2*sizeof(ipv4addr) + 2*sizeof(uint16);

static void tcp_connection_release(struct ctable *elem, bool freemem);

static struct ctable *cnx_table = NULL;

mutex_t ct_mutex;

static INIT void tcp_connection_init()
{
	/* Init a recursive mutex to avoid dead-lock on signal handling when
	 * running in single thread mode.
	 */
	UNUSED bool ret = mutex_init(&ct_mutex, true);
	assert(ret);
}

static FINI void tcp_connection_fini()
{
	struct ctable *ptr, *tmp;

	HASH_ITER(hh, cnx_table, ptr, tmp) {
		HASH_DEL(cnx_table, ptr);
		tcp_connection_release(ptr, true);
	}

	cnx_table = NULL;

	mutex_destroy(&ct_mutex);
}

void tcp_connection_insert(struct ctable *elem)
{
	mutex_lock(&ct_mutex);

	HASH_ADD(hh, cnx_table, tcp_conn.srcip, hash_keysize, elem);

	mutex_unlock(&ct_mutex);
}

#define EXCHANGE(a, b) { const typeof(a) tmp = a; a = b; b = tmp; }

static struct ctable *tcp_connection_find(struct tcp *tcp,
		bool *direction_in, bool *dropped)
{
	struct tcp_connection elem;
	struct ctable *ptr;

	assert(tcp);

	elem.srcip = ipv4_get_src(tcp->packet);
	elem.dstip = ipv4_get_dst(tcp->packet);
	elem.srcport = tcp_get_srcport(tcp);
	elem.dstport = tcp_get_dstport(tcp);

	mutex_lock(&ct_mutex);

	HASH_FIND(hh, cnx_table, &elem.srcip, hash_keysize, ptr);
	if (ptr) {
		mutex_unlock(&ct_mutex);
		if (direction_in) *direction_in = true;
		if (dropped) *dropped = ptr->dropped;
		return ptr;
	}

	EXCHANGE(elem.dstip, elem.srcip);
	EXCHANGE(elem.dstport, elem.srcport);

	HASH_FIND(hh, cnx_table, &elem.srcip, hash_keysize, ptr);
	if (ptr) {
		mutex_unlock(&ct_mutex);
		if (direction_in) *direction_in = false;
		if (dropped) *dropped = ptr->dropped;
		return ptr;
	}

	mutex_unlock(&ct_mutex);

	if (dropped) *dropped = false;
	return NULL;
}

static void tcp_connection_remove(struct ctable *elem)
{
	mutex_lock(&ct_mutex);
	HASH_DEL(cnx_table, elem);
	mutex_unlock(&ct_mutex);
}

static void tcp_connection_release(struct ctable *elem, bool freemem)
{
	if (elem->tcp_conn.stream_input) {
		tcp_stream_free(elem->tcp_conn.stream_input);
		elem->tcp_conn.stream_input = NULL;
	}

	if (elem->tcp_conn.stream_output) {
		tcp_stream_free(elem->tcp_conn.stream_output);
		elem->tcp_conn.stream_output = NULL;
	}

	if (freemem) {
		lua_ref_clear(&elem->tcp_conn.lua_table);
		lua_object_release(&elem->tcp_conn, &elem->tcp_conn.lua_object);
		free(elem);
	}
}

struct tcp_connection *tcp_connection_new(struct tcp *tcp)
{
	struct ctable *ptr;
	bool dropped;

	ptr = tcp_connection_find(tcp, NULL, &dropped);
	if (ptr) {
		if (dropped) {
			tcp_connection_remove(ptr);
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

	tcp_connection_insert(ptr);

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(ptr->tcp_conn.srcip, srcip, IPV4_ADDR_STRING_MAXLEN);
		ipv4_addr_to_string(ptr->tcp_conn.dstip, dstip, IPV4_ADDR_STRING_MAXLEN);

		messagef(HAKA_LOG_DEBUG, L"tcp-connection", L"opening connection %s:%u -> %s:%u",
				srcip, ptr->tcp_conn.srcport, dstip, ptr->tcp_conn.dstport);
	}

	return &ptr->tcp_conn;
}

struct tcp_connection *tcp_connection_get(struct tcp *tcp, bool *direction_in,
		bool *_dropped)
{
	bool dropped;
	struct ctable *elem = tcp_connection_find(tcp, direction_in, &dropped);
	if (elem) {
		if (dropped) {
			if (_dropped) *_dropped = dropped;
			return NULL;
		}
		else {
			if (_dropped) *_dropped = false;
			return &elem->tcp_conn;
		}
	}
	else {
		if (direction_in) *direction_in = true; /* This is the direction of this connection
												   if is created from this packet */
		if (_dropped) *_dropped = false;
		return NULL;
	}
}

void tcp_connection_close(struct tcp_connection* tcp_conn)
{
	struct ctable *current = (struct ctable *)((uint8 *)tcp_conn - offsetof(struct ctable, tcp_conn));
	assert(tcp_conn);

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(current->tcp_conn.srcip, srcip, IPV4_ADDR_STRING_MAXLEN);
		ipv4_addr_to_string(current->tcp_conn.dstip, dstip, IPV4_ADDR_STRING_MAXLEN);

		messagef(HAKA_LOG_DEBUG, L"tcp-connection", L"closing connection %s:%u -> %s:%u",
				srcip, current->tcp_conn.srcport, dstip, current->tcp_conn.dstport);
	}

	tcp_connection_remove(current);
	tcp_connection_release(current, true);
}

void tcp_connection_drop(struct tcp_connection *tcp_conn)
{
	struct ctable *current = (struct ctable *)((uint8 *)tcp_conn - offsetof(struct ctable, tcp_conn));
	assert(tcp_conn);

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
