%module ipv4
%{
#include "ipv4.h"
%}

%nodefaultctor;

struct ipv4_flags {
	%extend {
		bool dontFragment;
		bool moreFragment;
		unsigned int value;
	}
};

struct ipv4 {
	%extend {
		~ipv4()
		{
			release($self);
		}

		unsigned int ihl;
		unsigned int version;
		unsigned int tos;
		unsigned int length;
		unsigned int id;
		unsigned int fragmentOffset;
		unsigned int ttl;
		unsigned int protocol;
		unsigned int checksum;
		unsigned int saddr;
		unsigned int daddr;

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
	}
};

%newobject create;
struct ipv4 *create(struct packet *packet);

%{

#define IPV4_INT_GETSET(field) \
	unsigned int ipv4_##field##_get(struct ipv4 *ip) { return ipv4_get_##field(ip); } \
	void ipv4_##field##_set(struct ipv4 *ip, unsigned int v) { ipv4_set_##field(ip, v); }

IPV4_INT_GETSET(ihl);
IPV4_INT_GETSET(version);
IPV4_INT_GETSET(tos);
IPV4_INT_GETSET(length);
IPV4_INT_GETSET(id);
IPV4_INT_GETSET(fragmentOffset);
IPV4_INT_GETSET(ttl);
IPV4_INT_GETSET(protocol);
IPV4_INT_GETSET(checksum);
IPV4_INT_GETSET(saddr);
IPV4_INT_GETSET(daddr);

struct ipv4_flags *ipv4_flags_get(struct ipv4 *ip) { return (struct ipv4_flags *)ip; }

bool ipv4_flags_dontFragment_get(struct ipv4_flags *flags) { return ipv4_get_flags_dontFragment((struct ipv4 *)flags); }
void ipv4_flags_dontFragment_set(struct ipv4_flags *flags, bool v) { return ipv4_set_flags_dontFragment((struct ipv4 *)flags, v); }
bool ipv4_flags_moreFragment_get(struct ipv4_flags *flags) { return ipv4_get_flags_moreFragment((struct ipv4 *)flags); }
void ipv4_flags_moreFragment_set(struct ipv4_flags *flags, bool v) { return ipv4_set_flags_moreFragment((struct ipv4 *)flags, v); }
unsigned int ipv4_flags_value_get(struct ipv4_flags *flags) { return ipv4_get_flags((struct ipv4 *)flags); }
void ipv4_flags_value_set(struct ipv4_flags *flags, unsigned int v) { return ipv4_set_flags((struct ipv4 *)flags, v); }

%}
