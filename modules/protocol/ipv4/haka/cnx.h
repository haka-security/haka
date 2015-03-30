/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAKA_PROTO_IPV4_CNX_H
#define HAKA_PROTO_IPV4_CNX_H

#include <haka/types.h>
#include <haka/ipv4.h>
#include <haka/lua/ref.h>
#include <haka/lua/object.h>

#define CNX_DIR_IN  0
#define CNX_DIR_OUT 1
#define CNX_DIR_CNT 2

struct cnx_table;

struct cnx_stats {
	size_t               packets;
	size_t               bytes;
};

struct cnx_key {
	ipv4addr             srcip;
	ipv4addr             dstip;
	uint16               srcport;
	uint16               dstport;
};

struct cnx {
	struct lua_object    lua_object;
	struct cnx_key       key;
	struct cnx_stats     stats[CNX_DIR_CNT];
	bool                 dropped;
	struct lua_ref       lua_priv;
	uint32               id;
	void                *priv;
};

struct cnx_table *cnx_table_new(void (*cnx_release)(struct cnx *, bool));
void              cnx_table_release(struct cnx_table *table);
bool              cnx_foreach(struct cnx_table *table, bool include_dropped, bool (*callback)(void *data, struct cnx *, int index), void *data);

struct cnx *cnx_new(struct cnx_table *table, struct cnx_key *key);
struct cnx *cnx_get(struct cnx_table *table, struct cnx_key *key, int *direction, bool *dropped);
struct cnx *cnx_get_byid(struct cnx_table *table, uint32 id);

void cnx_close(struct cnx *cnx);
void cnx_drop(struct cnx *cnx);
void cnx_update_stat(struct cnx *cnx, int direction, size_t size);

#endif /* HAKA_PROTO_IPV4_CNX_H */
