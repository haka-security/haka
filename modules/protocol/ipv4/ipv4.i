/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module ipv4

%{
	#include "haka/ipv4.h"
	#include "haka/lua/state.h"

	struct ipv4_flags;
	struct ipv4_payload;

	struct ipv4_addr {
		ipv4addr   addr;
	};

	struct ipv4_network {
		ipv4network  net;
	};

	struct ipv4_addr *ipv4_addr_new(ipv4addr a)
	{
		struct ipv4_addr *ret = malloc(sizeof(struct ipv4_addr));
		if (!ret) {
			return NULL;
		}

		ret->addr = a;
		return ret;
	}

	struct inet_checksum {
		bool    odd;
		int32   value;
	};

	static int lua_inet_checksum_sub(struct vbuffer_sub *buf)
	{
		return SWAP_FROM_IPV4(int16, inet_checksum_vbuffer(buf));
	}

	static int lua_inet_checksum(struct vbuffer *buf)
	{
		struct vbuffer_sub sub;
		vbuffer_sub_create(&sub, buf, 0, ALL);
		return lua_inet_checksum_sub(&sub);
	}
%}

%include "haka/lua/swig.si"
%include "haka/lua/object.si"
%include "haka/lua/packet.si"
%include "haka/lua/ref.si"
%include "haka/lua/ipv4.si"
%include "haka/lua/ipv4-addr.si"

%rename(addr) ipv4_addr;

struct ipv4_addr {
	%extend {
		ipv4_addr(const char *str) {
			struct ipv4_addr *ret = malloc(sizeof(struct ipv4_addr));
			if (!ret) {
				return NULL;
			}

			ret->addr = ipv4_addr_from_string(str);
			if (check_error()) {
				free(ret);
				return NULL;
			}

			return ret;
		}

		ipv4_addr(int addr) {
			return ipv4_addr_new(addr);
		}

		ipv4_addr(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
			if (a > 255 || b > 255 || c > 255 || d > 255) {
				error(L"invalid IPv4 address format");
				return NULL;
			}

			struct ipv4_addr *ret = malloc(sizeof(struct ipv4_addr));
			if (!ret) {
				return NULL;
			}

			ret->addr = ipv4_addr_from_bytes(a, b, c, d);
			if (check_error()) {
				free(ret);
				return NULL;
			}

			return ret;
		}

		~ipv4_addr() {
			if ($self)
				free($self);
		}

		bool __eq(struct ipv4_addr *addr) const
		{
			return $self->addr == addr->addr;
		}

		bool __lt(struct ipv4_addr *addr) const
		{
			return $self->addr < addr->addr;
		}

		bool __le(struct ipv4_addr *addr) const
		{
			return $self->addr <= addr->addr;
		}

		void __tostring(char **TEMP_OUTPUT)
		{
			*TEMP_OUTPUT = malloc(IPV4_ADDR_STRING_MAXLEN + 1);
			if (!*TEMP_OUTPUT) {
				error(L"memory error");
				return;
			}

			ipv4_addr_to_string($self->addr, *TEMP_OUTPUT, IPV4_ADDR_STRING_MAXLEN + 1);
		}

		int packed;
	}
};

%rename(inet_checksum) checksum_partial;
struct checksum_partial {
	%extend {
		checksum_partial() {
			struct checksum_partial *ret = malloc(sizeof(struct checksum_partial));
			if (!ret) {
				error(L"memory error");
				return NULL;
			}
			*ret = checksum_partial_init;
			return ret;
		}

		~checksum_partial() {
			free($self);
		}

		void process(struct vbuffer_sub *sub) {
			inet_checksum_vbuffer_partial($self, sub);
		}

		void process(struct vbuffer *buf) {
			struct vbuffer_sub sub;
			vbuffer_sub_create(&sub, buf, 0, ALL);
			inet_checksum_vbuffer_partial($self, &sub);
		}

		%rename(reduce) _reduce;
		int _reduce() {
			return SWAP_FROM_IPV4(int16, inet_checksum_reduce($self));
		}
	}
};

%{
	int ipv4_addr_packed_get(struct ipv4_addr *addr) { return addr->addr; }
	void ipv4_addr_packed_set(struct ipv4_addr *addr, int packed) { addr->addr = packed; }
%}

STRUCT_UNKNOWN_KEY_ERROR(ipv4_addr);

%rename(network) ipv4_network;
%newobject ipv4_network::net;

struct ipv4_network {
	%extend {
		ipv4_network(const char *str) {
			struct ipv4_network *ret = malloc(sizeof(struct ipv4_network));
			if (!ret) {
				return NULL;
			}

			ret->net = ipv4_network_from_string(str);
			if (check_error()) {
				free(ret);
				return NULL;
			}

			return ret;
		}

		ipv4_network(struct ipv4_addr addr, unsigned char mask) {
			if (mask < 0 || mask > 32) {
				error(L"Invalid IPv4 addresss network format");
				return NULL;
			}

			struct ipv4_network *ret = malloc(sizeof(struct ipv4_network));
			if (!ret) {
				return NULL;
			}
			ret->net.net = addr.addr;
			ret->net.mask = mask;
			return ret;
		}

		~ipv4_network() {
			if ($self)
				free($self);
		}

		void __tostring(char **TEMP_OUTPUT)
		{
			*TEMP_OUTPUT = malloc(IPV4_NETWORK_STRING_MAXLEN + 1);
			if (!*TEMP_OUTPUT) {
				error(L"memory error");
				return;
			}

			ipv4_network_to_string($self->net, *TEMP_OUTPUT,
					IPV4_NETWORK_STRING_MAXLEN + 1);
		}

		%rename(contains) _contains;
		bool _contains(struct ipv4_addr *addr)
		{
			if (!addr) {
				error(L"nil argument");
				return false;
			}
			return ipv4_network_contains($self->net, addr->addr);
		}

		%immutable;
		struct ipv4_addr *net;
		unsigned char mask;
	}
};

STRUCT_UNKNOWN_KEY_ERROR(ipv4_network);

LUA_OBJECT_CAST(struct ipv4_flags, struct ipv4);

%nodefaultctor;
%nodefaultdtor;
struct ipv4_flags {
	%extend {
		bool rb;
		bool df;
		bool mf;
		unsigned int all;
	}
};

STRUCT_UNKNOWN_KEY_ERROR(ipv4_flags);

LUA_OBJECT(struct ipv4);
%newobject ipv4::src;
%newobject ipv4::dst;

struct ipv4 {
	%extend {
		~ipv4()
		{
			if ($self)
				ipv4_release($self);
		}

		unsigned int hdr_len;
		unsigned int version;
		unsigned int tos;
		unsigned int len;
		unsigned int id;
		unsigned int frag_offset;
		unsigned int ttl;
		unsigned int proto;
		unsigned int checksum;
		struct ipv4_addr *src;
		struct ipv4_addr *dst;

		%immutable;
		const char *name;
		struct packet *raw;
		struct ipv4_flags *flags;
		struct vbuffer *payload;

		bool verify_checksum();
		void compute_checksum();
		void drop() { ipv4_action_drop($self); }

		bool _continue()
		{
			assert($self);
			return $self->packet != NULL;
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(ipv4);

%rename(_dissect) ipv4_dissect;
%newobject ipv4_dissect;
struct ipv4 *ipv4_dissect(struct packet *DISOWN_SUCCESS_ONLY);

%rename(_create) ipv4_create;
%newobject ipv4_create;
struct ipv4 *ipv4_create(struct packet *DISOWN_SUCCESS_ONLY);

%rename(_forge) ipv4_forge;
%newobject ipv4_forge;
struct packet *ipv4_forge(struct ipv4 *pkt);

%rename(inet_checksum_compute) lua_inet_checksum_sub;
int lua_inet_checksum_sub(struct vbuffer_sub *sub);

%rename(inet_checksum_compute) lua_inet_checksum;
int lua_inet_checksum(struct vbuffer *buf);

%{
	#define IPV4_INT_GETSET(field) \
		unsigned int ipv4_##field##_get(struct ipv4 *ip) { return ipv4_get_##field(ip); } \
		void ipv4_##field##_set(struct ipv4 *ip, unsigned int v) { ipv4_set_##field(ip, v); }

	IPV4_INT_GETSET(version);
	IPV4_INT_GETSET(hdr_len);
	IPV4_INT_GETSET(tos);
	IPV4_INT_GETSET(len);
	IPV4_INT_GETSET(id);
	IPV4_INT_GETSET(frag_offset);
	IPV4_INT_GETSET(ttl);
	IPV4_INT_GETSET(proto);
	IPV4_INT_GETSET(checksum);

	struct packet *ipv4_raw_get(struct ipv4 *ip) { return ip->packet; }

	struct ipv4_addr *ipv4_src_get(struct ipv4 *ip) { return ipv4_addr_new(ipv4_get_src(ip)); }
	void ipv4_src_set(struct ipv4 *ip, struct ipv4_addr *v) { ipv4_set_src(ip, v->addr); }
	struct ipv4_addr *ipv4_dst_get(struct ipv4 *ip) { return ipv4_addr_new(ipv4_get_dst(ip)); }
	void ipv4_dst_set(struct ipv4 *ip, struct ipv4_addr *v) { ipv4_set_dst(ip, v->addr); }

	const char *ipv4_name_get(struct ipv4 *ip) { return "ipv4"; }

	struct vbuffer *ipv4_payload_get(struct ipv4 *ip) { return &ip->payload; }

	struct ipv4_flags *ipv4_flags_get(struct ipv4 *ip) { return (struct ipv4_flags *)ip; }

	#define IPV4_FLAGS_GETSET(field) \
		bool ipv4_flags_##field##_get(struct ipv4_flags *flags) { return ipv4_get_flags_##field((struct ipv4 *)flags); } \
		void ipv4_flags_##field##_set(struct ipv4_flags *flags, bool v) { return ipv4_set_flags_##field((struct ipv4 *)flags, v); }

	IPV4_FLAGS_GETSET(rb);
	IPV4_FLAGS_GETSET(df);
	IPV4_FLAGS_GETSET(mf);

	unsigned int ipv4_flags_all_get(struct ipv4_flags *flags) { return ipv4_get_flags((struct ipv4 *)flags); }
	void ipv4_flags_all_set(struct ipv4_flags *flags, unsigned int v) { return ipv4_set_flags((struct ipv4 *)flags, v); }

	struct ipv4_addr *ipv4_network_net_get(struct ipv4_network *network) { return ipv4_addr_new(network->net.net); }

	unsigned char ipv4_network_mask_get(struct ipv4_network *network) { return network->net.mask; }
%}

%luacode {
	local this = unpack({...})

	local ipv4_protocol_dissectors = {}

	function this.register_protocol(proto, dissector)
		if ipv4_protocol_dissectors[proto] then
			error("IPv4 protocol %d dissector already registered", proto);
		end

		ipv4_protocol_dissectors[proto] = dissector
	end

	local ipv4_dissector = haka.dissector.new{
		type = haka.dissector.PacketDissector,
		name = 'ipv4'
	}

	function ipv4_dissector:new(pkt)
		return this._dissect(pkt)
	end

	function ipv4_dissector:create(pkt)
		return this._create(pkt)
	end

	function ipv4_dissector.method:receive()
		haka.context:signal(self, ipv4_dissector.events['receive_packet'])

		local next_dissector = ipv4_protocol_dissectors[self.proto]
		if next_dissector then
			return next_dissector:receive(self)
		else
			return self:send()
		end
	end

	function ipv4_dissector.method:send()
		haka.context:signal(self, ipv4_dissector.events['send_packet'])

		local pkt = this._forge(self)
		return pkt:send()
	end

	function ipv4_dissector.method:inject()
		local pkt = this._forge(self)
		return pkt:inject()
	end

	function ipv4_dissector.method:continue()
		if not self:_continue() then
			return haka.abort()
		end
	end

	swig.getclassmetatable('ipv4')['.fn'].receive = ipv4_dissector.method.receive
	swig.getclassmetatable('ipv4')['.fn'].send = ipv4_dissector.method.send
	swig.getclassmetatable('ipv4')['.fn'].inject = ipv4_dissector.method.inject
	swig.getclassmetatable('ipv4')['.fn'].continue = ipv4_dissector.method.continue
	swig.getclassmetatable('ipv4')['.fn'].error = swig.getclassmetatable('ipv4')['.fn'].drop

	-- ipv4 Lua full dissector, uncomment to enable
	--[[ipv4 = this
	ipv4.ipv4_protocol_dissectors = ipv4_protocol_dissectors
	require('protocol/ipv4lua')]]
}

%include "cnx.si"
