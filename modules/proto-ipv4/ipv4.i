%module ipv4
%{
#include "ipv4.h"
%}

%include haka/swig.i

struct addr {
	%extend {
		addr(const char *str) {
			return (struct addr*)(ptrdiff_t)ipv4_addr_from_string(str);
		}

		addr(unsigned int addr) {
			return (struct addr*)(ptrdiff_t)addr;
		}

		addr(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
			return (struct addr*)(ptrdiff_t)ipv4_addr_from_bytes(a, b, c, d);
		}

		~addr() {
		}

		bool __eq(struct addr *addr) const
		{
			return (uint32)(ptrdiff_t)$self == (uint32)(ptrdiff_t)addr;
		}

		bool __lt(struct addr *addr) const
		{
			return (uint32)(ptrdiff_t)$self < (uint32)(ptrdiff_t)addr;
		}

		bool __le(struct addr *addr) const
		{
			return (uint32)(ptrdiff_t)$self <= (uint32)(ptrdiff_t)addr;
		}

		const char *__tostring()
		{
			static char buffer[16];
			ipv4_addr_to_string((uint32)(ptrdiff_t)$self, buffer, sizeof(buffer));
			return buffer;
		}
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
		struct addr *src;
		struct addr *dst;

		%immutable;
		struct ipv4_flags *flags;

		bool verifyChecksum()
		{
			return ipv4_verify_checksum($self);
		}

		void computeChecksum()
		{
			ipv4_compute_checksum($self);
		}

		%rename(forge) _forge;
		void _forge()
		{
			ipv4_forge($self);
		}
	}
};

%rename(dissect) ipv4_dissect;
%newobject ipv4_dissect;
struct ipv4 *ipv4_dissect(struct packet *packet);

%{

#define IPV4_INT_GETSET(field) \
	unsigned int ipv4_##field##_get(struct ipv4 *ip) { return ipv4_get_##field(ip); } \
	void ipv4_##field##_set(struct ipv4 *ip, unsigned int v) { ipv4_set_##field(ip, v); }

IPV4_INT_GETSET(version);
IPV4_INT_GETSET(tos);
IPV4_INT_GETSET(len);
IPV4_INT_GETSET(id);
IPV4_INT_GETSET(ttl);
IPV4_INT_GETSET(proto);
IPV4_INT_GETSET(checksum);


#define IPV4_INT_GETSET_MULT(field, factor) \
    unsigned int ipv4_##field##_get(struct ipv4 *ip) { return ipv4_get_##field(ip) << (factor); } \
    void ipv4_##field##_set(struct ipv4 *ip, unsigned int v) { ipv4_set_##field(ip, (v) >> (factor)); }

IPV4_INT_GETSET_MULT(hdr_len, 2);
IPV4_INT_GETSET_MULT(frag_offset, 3);

struct addr *ipv4_src_get(struct ipv4 *ip) { return (struct addr*)(ptrdiff_t)ipv4_get_src(ip); }
void ipv4_src_set(struct ipv4 *ip, struct addr *v) { ipv4_set_src(ip, (uint32)(ptrdiff_t)v); }
struct addr *ipv4_dst_get(struct ipv4 *ip) { return (struct addr*)(ptrdiff_t)ipv4_get_dst(ip); }
void ipv4_dst_set(struct ipv4 *ip, struct addr *v) { ipv4_set_dst(ip, (uint32)(ptrdiff_t)v); }

struct ipv4_flags *ipv4_flags_get(struct ipv4 *ip) { return (struct ipv4_flags *)ip; }

#define IPV4_FLAGS_GETSET(field) \
		bool ipv4_flags_##field##_get(struct ipv4_flags *flags) { return ipv4_get_flags_##field((struct ipv4 *)flags); } \
		void ipv4_flags_##field##_set(struct ipv4_flags *flags, bool v) { return ipv4_set_flags_##field((struct ipv4 *)flags, v); }

IPV4_FLAGS_GETSET(rb);
IPV4_FLAGS_GETSET(df);
IPV4_FLAGS_GETSET(mf);

unsigned int ipv4_flags_all_get(struct ipv4_flags *flags) { return ipv4_get_flags((struct ipv4 *)flags); }
void ipv4_flags_all_set(struct ipv4_flags *flags, unsigned int v) { return ipv4_set_flags((struct ipv4 *)flags, v); }

%}

%luacode {
	getmetatable(ipv4).__call = function (_, pkt)
		return ipv4.dissect(pkt)
	end
}
