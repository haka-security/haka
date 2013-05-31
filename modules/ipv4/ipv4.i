%module ipv4

%{
	#include "haka/ipv4.h"
	#include "haka/ipv4-address.h"
	#include "haka/ipv4-network.h"

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
%}

%include "haka/swig.i"

%rename(addr) ipv4_addr;
struct ipv4_addr {
	%extend {
		ipv4_addr(const char *str) {
			struct ipv4_addr *ret = malloc(sizeof(struct ipv4_addr));
			if (!ret) {
				return NULL;
			}

			ret->addr = ipv4_addr_from_string(str);
			return ret;
		}

		ipv4_addr(unsigned int addr) {
			return ipv4_addr_new(addr);
		}

		ipv4_addr(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
			struct ipv4_addr *ret = malloc(sizeof(struct ipv4_addr));
			if (!ret) {
				return NULL;
			}

			ret->addr = ipv4_addr_from_bytes(a, b, c, d);
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

		temporary_string __tostring()
		{
			char *buffer = malloc(IPV4_ADDR_STRING_MAXLEN + 1);
			if (!buffer) {
				error(L"memory error");
				return NULL;
			}
			ipv4_addr_to_string($self->addr, buffer, IPV4_ADDR_STRING_MAXLEN + 1);
			return buffer;
		}
	}
};


%rename(network) ipv4_network;
struct ipv4_network {
	%extend {
		ipv4_network(const char *str) {
			struct ipv4_network *ret = malloc(sizeof(struct ipv4_network));
			if (!ret) {
				return NULL;
			}

			ret->net = ipv4_network_from_string(str);
			return ret;
		}

		~ipv4_network() {
			if ($self)
				free($self);
		}

		temporary_string __tostring()
		{
			char *buffer = malloc(IPV4_NET_STRING_MAXLEN + 1);
			if (!buffer) {
				error(L"memory error");
				return NULL;
			}
			ipv4_network_to_string($self->net, buffer,
					IPV4_NET_STRING_MAXLEN + 1);

			return buffer;
		}

		bool contains(struct ipv4_addr *addr)
		{
			return ipv4_check_addr_in_network($self->net, addr->addr);
		}

		%immutable;
		struct ipv4_addr *net;
		unsigned char mask;
	}
};


%nodefaultctor;

struct ipv4_flags {
	%extend {
		bool rb;
		bool df;
		bool mf;
		unsigned int all;
	}
};

struct ipv4_payload {
	%extend {
		size_t __len(void *dummy)
		{
			return ipv4_get_payload_length((struct ipv4 *)$self);
		}

		int __getitem(int index)
		{
			const size_t size = ipv4_get_payload_length((struct ipv4 *)$self);

			--index;
			if (index < 0 || index >= size) {
				error(L"out-of-bound index");
				return 0;
			}
			return ipv4_get_payload((struct ipv4 *)$self)[index];
		}

		void __setitem(int index, int value)
		{
			const size_t size = ipv4_get_payload_length((struct ipv4 *)$self);

			--index;
			if (index < 0 || index >= size) {
				error(L"out-of-bound index");
				return;
			}
			ipv4_get_payload_modifiable((struct ipv4 *)$self)[index] = value;
		}
	}
};

struct ipv4 {
	%extend {
		~ipv4()
		{
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
		struct ipv4_flags *flags;
		struct ipv4_payload *payload;
		const char *dissector;
		const char *nextDissector;

		bool verifyChecksum()
		{
			return ipv4_verify_checksum($self);
		}

		void computeChecksum()
		{
			ipv4_compute_checksum($self);
		}

		void drop()
		{
			ipv4_action_drop($self);
		}

		bool valid();
		struct packet *forge();
	}
};

%rename(dissect) ipv4_dissect;
%newobject ipv4_dissect;
struct ipv4 *ipv4_dissect(struct packet *packet);

%rename(register_proto) ipv4_register_proto_dissector;
void ipv4_register_proto_dissector(int proto, const char *dissector);

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

	struct ipv4_addr *ipv4_src_get(struct ipv4 *ip) { return ipv4_addr_new(ipv4_get_src(ip)); }
	void ipv4_src_set(struct ipv4 *ip, struct ipv4_addr *v) { ipv4_set_src(ip, v->addr); }
	struct ipv4_addr *ipv4_dst_get(struct ipv4 *ip) { return ipv4_addr_new(ipv4_get_dst(ip)); }
	void ipv4_dst_set(struct ipv4 *ip, struct ipv4_addr *v) { ipv4_set_dst(ip, v->addr); }

	struct ipv4_payload *ipv4_payload_get(struct ipv4 *ip) { return (struct ipv4_payload *)ip; }

	const char *ipv4_dissector_get(struct ipv4 *ip) { return "ipv4"; }

	const char *ipv4_nextDissector_get(struct ipv4 *ip) { return ipv4_get_proto_dissector(ip); }

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
	haka.dissector {
		name = "ipv4",
		dissect = ipv4.dissect
	}
}
