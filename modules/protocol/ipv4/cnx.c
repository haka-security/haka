/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#include <haka/cnx.h>
#include <haka/thread.h>
#include <haka/log.h>
#include <haka/error.h>
#include <haka/container/hash.h>

#define CNX_ELEM(var) ((struct cnx_table_elem *)((uint8 *)var - offsetof(struct cnx_table_elem, cnx)))

struct cnx_table_elem {
	hash_head_t       hh;
	struct cnx_table *table;
	struct cnx        cnx;
};

struct cnx_table {
	mutex_t                 mutex;
	struct cnx_table_elem  *head;
	void                  (*cnx_release)(struct cnx *, bool);
	atomic_t                id;
};

static const size_t hash_keysize = sizeof(struct cnx_key);

static struct cnx_table_elem *cnx_find(struct cnx_table *table, struct cnx_key *key, int *direction, bool *dropped);
static void cnx_remove(struct cnx_table *table, struct cnx_table_elem *elem);
static void cnx_release(struct cnx_table *table, struct cnx_table_elem *elem, bool freemem);


struct cnx_table *cnx_table_new(void (*cnx_release)(struct cnx *, bool))
{
	struct cnx_table *table = malloc(sizeof(struct cnx_table));
	if (!table) {
		error("memory error");
		return NULL;
	}

	/* Init a recursive mutex to avoid dead-lock on signal handling when
	 * running in single thread mode.
	 */
	UNUSED bool ret = mutex_init(&table->mutex, true);
	assert(ret);

	table->head = NULL;
	table->cnx_release = cnx_release;
	atomic_set(&table->id, 0);

	return table;
}

void cnx_table_release(struct cnx_table *table)
{
	struct cnx_table_elem *elem, *tmp;

	HASH_ITER(hh, table->head, elem, tmp) {
		HASH_DEL(table->head, elem);

		{
			char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

			ipv4_addr_to_string(elem->cnx.key.srcip, srcip, IPV4_ADDR_STRING_MAXLEN+1);
			ipv4_addr_to_string(elem->cnx.key.dstip, dstip, IPV4_ADDR_STRING_MAXLEN+1);
			messagef(HAKA_LOG_DEBUG, "cnx", "release connection %s:%u -> %s:%u",
					srcip, elem->cnx.key.srcport, dstip, elem->cnx.key.dstport);
		}

		cnx_release(table, elem, true);
	}

	mutex_destroy(&table->mutex);
	free(table);
}

static void cnx_insert(struct cnx_table *table, struct cnx_table_elem *elem)
{
	mutex_lock(&table->mutex);

	HASH_ADD(hh, table->head, cnx.key, hash_keysize, elem);

	mutex_unlock(&table->mutex);
}

#define EXCHANGE(a, b) { const typeof(a) tmp = a; a = b; b = tmp; }

struct cnx_table_elem *cnx_find(struct cnx_table *table, struct cnx_key *_key,
		int *direction, bool *dropped)
{
	struct cnx_key key;
	struct cnx_table_elem *ptr;

	assert(_key);

	key = *_key;

	mutex_lock(&table->mutex);

	HASH_FIND(hh, table->head, &key, hash_keysize, ptr);
	if (ptr) {
		mutex_unlock(&table->mutex);
		if (direction) *direction = CNX_DIR_IN;
		if (dropped) *dropped = ptr->cnx.dropped;
		return ptr;
	}

	EXCHANGE(key.dstip, key.srcip);
	EXCHANGE(key.dstport, key.srcport);

	HASH_FIND(hh, table->head, &key, hash_keysize, ptr);
	if (ptr) {
		mutex_unlock(&table->mutex);
		if (direction) *direction = CNX_DIR_OUT;
		if (dropped) *dropped = ptr->cnx.dropped;
		return ptr;
	}

	mutex_unlock(&table->mutex);

	if (dropped) *dropped = false;
	return NULL;
}

static void cnx_remove(struct cnx_table *table, struct cnx_table_elem *elem)
{
	mutex_lock(&table->mutex);
	HASH_DEL(table->head, elem);
	mutex_unlock(&table->mutex);
}

static void cnx_release(struct cnx_table *table, struct cnx_table_elem *elem, bool freemem)
{
	if (table->cnx_release) {
		table->cnx_release(&elem->cnx, freemem);
	}

	if (freemem) {
		lua_ref_clear(&elem->cnx.lua_priv);
		lua_object_release(&elem->cnx, &elem->cnx.lua_object);
		free(elem);
	}
}

struct cnx *cnx_new(struct cnx_table *table, struct cnx_key *key)
{
	struct cnx_table_elem *elem;
	bool dropped;
	int i;

	elem = cnx_find(table, key, NULL, &dropped);
	if (elem) {
		if (dropped) {
			cnx_remove(table, elem);
			cnx_release(table, elem, true);
		}
		else {
			error("cnx already exists");
			return NULL;
		}
	}

	elem = malloc(sizeof(struct cnx_table_elem));
	if (!elem) {
		error("memory error");
		return NULL;
	}

	elem->cnx.lua_object = lua_object_init;
	elem->cnx.key = *key;
	elem->cnx.id = atomic_inc(&table->id);
	elem->cnx.dropped = false;

	for (i=0; i<CNX_DIR_CNT; ++i) {
		elem->cnx.stats[i].packets = 0;
		elem->cnx.stats[i].bytes = 0;
	}

	lua_ref_init(&elem->cnx.lua_priv);
	elem->table = table;

	cnx_insert(table, elem);

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(elem->cnx.key.srcip, srcip, IPV4_ADDR_STRING_MAXLEN+1);
		ipv4_addr_to_string(elem->cnx.key.dstip, dstip, IPV4_ADDR_STRING_MAXLEN+1);
		messagef(HAKA_LOG_DEBUG, "cnx", "opening connection %s:%u -> %s:%u",
				srcip, elem->cnx.key.srcport, dstip, elem->cnx.key.dstport);
	}

	return &elem->cnx;
}

struct cnx *cnx_get_byid(struct cnx_table *table, uint32 id)
{
	struct cnx_table_elem *ptr, *tmp;

	mutex_lock(&table->mutex);

	HASH_ITER(hh, table->head, ptr, tmp) {
		if (ptr->cnx.id == id) {
			mutex_unlock(&table->mutex);
			return &ptr->cnx;
		}
	}

	mutex_unlock(&table->mutex);
	return NULL;
}

struct cnx *cnx_get(struct cnx_table *table, struct cnx_key *key,
		int *direction, bool *_dropped)
{
	bool dropped;
	struct cnx_table_elem *elem = cnx_find(table, key, direction, &dropped);
	if (elem) {
		if (dropped) {
			if (_dropped) *_dropped = dropped;
			return NULL;
		}
		else {
			if (_dropped) *_dropped = false;
			return &elem->cnx;
		}
	}
	else {
		if (direction) *direction = CNX_DIR_IN;
		if (_dropped) *_dropped = false;
		return NULL;
	}
}

bool cnx_foreach(struct cnx_table *table, bool include_dropped, bool (*callback)(void *data, struct cnx *, int index), void *data)
{
	struct cnx_table_elem *ptr, *tmp;
	int index = 0;

	mutex_lock(&table->mutex);

	HASH_ITER(hh, table->head, ptr, tmp) {
		if (include_dropped || !ptr->cnx.dropped) {
			if (!callback(data, &ptr->cnx, index++)) {
				mutex_unlock(&table->mutex);
				return false;
			}
		}
	}

	mutex_unlock(&table->mutex);
	return true;
}

void cnx_close(struct cnx* cnx)
{
	struct cnx_table_elem *elem = CNX_ELEM(cnx);
	assert(cnx);

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(elem->cnx.key.srcip, srcip, IPV4_ADDR_STRING_MAXLEN+1);
		ipv4_addr_to_string(elem->cnx.key.dstip, dstip, IPV4_ADDR_STRING_MAXLEN+1);

		messagef(HAKA_LOG_DEBUG, "cnx", "closing connection %s:%u -> %s:%u",
				srcip, elem->cnx.key.srcport, dstip, elem->cnx.key.dstport);
	}

	cnx_remove(elem->table, elem);
	cnx_release(elem->table, elem, true);
}

void cnx_drop(struct cnx *cnx)
{
	struct cnx_table_elem *elem = CNX_ELEM(cnx);
	assert(cnx);

	{
		char srcip[IPV4_ADDR_STRING_MAXLEN+1], dstip[IPV4_ADDR_STRING_MAXLEN+1];

		ipv4_addr_to_string(elem->cnx.key.srcip, srcip, IPV4_ADDR_STRING_MAXLEN+1);
		ipv4_addr_to_string(elem->cnx.key.dstip, dstip, IPV4_ADDR_STRING_MAXLEN+1);

		messagef(HAKA_LOG_DEBUG, "cnx", "dropping connection %s:%u -> %s:%u",
				srcip, elem->cnx.key.srcport, dstip, elem->cnx.key.dstport);
	}

	cnx_release(elem->table, elem, false);
	elem->cnx.dropped = true;
}

void cnx_update_stat(struct cnx *cnx, int direction, size_t size)
{
	++cnx->stats[direction].packets;
	cnx->stats[direction].bytes += size;
}
