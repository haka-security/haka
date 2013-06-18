%module tcp
%include haka/lua/ipv4.si

CHECK_FOR_PACKET(struct tcp*,tcp)
CHECK_FOR_PACKET(struct tcp_flags*,tcp flags)
CHECK_FOR_PACKET(struct tcp_payload*,tcp payload)
PACKET_DEPENDANT_CONSTRUCTOR(tcp_dissect,arg1->packet,SWIGTYPE_p_tcp);
PACKET_DEPENDANT_GETTER(tcp::ip,result->packet,SWIGTYPE_p_ipv4);
PACKET_DEPENDANT_GETTER(tcp::forge,result->packet,SWIGTYPE_p_ipv4);
PACKET_DEPENDANT_CONSTRUCTOR(tcp::flags,arg1->packet->packet,SWIGTYPE_p_tcp_flags);
PACKET_DEPENDANT_CONSTRUCTOR(tcp::payload,arg1->packet->packet,SWIGTYPE_p_tcp_payload);
PACKET_DEPENDANT_GETTER(stream::pop,result->packet->packet,SWIGTYPE_p_tcp);

%{
#include <haka/stream.h>
#include <haka/tcp.h>
#include <haka/tcp-connection.h>
#include <haka/tcp-stream.h>
#include <haka/log.h>

struct tcp_payload;

#include <haka/lua/ipv4-addr.si>

%}

%include "haka/lua/swig.si"
%include "haka/lua/stream.si"
%include "haka/lua/ref.si"
%include "typemaps.i"

%nodefaultctor;
%nodefaultdtor;

struct tcp_flags {
	%extend {
		bool fin;
		bool syn;
		bool rst;
		bool psh;
		bool ack;
		bool urg;
		bool ecn;
		bool cwr;
		unsigned char all;
	}
};

struct tcp_payload {
	%extend {
		size_t __len(void *dummy)
		{
			return tcp_get_payload_length((struct tcp *)$self);
		}

		int __getitem(int index)
		{
			const size_t size = tcp_get_payload_length((struct tcp *)$self);

			--index;
			if (index < 0 || index >= size) {
				error(L"out-of-bound index");
				return 0;
			}
			return tcp_get_payload((struct tcp *)$self)[index];
		}

		void __setitem(int index, int value)
		{
			const size_t size = tcp_get_payload_length((struct tcp *)$self);

			--index;
			if (index < 0 || index >= size) {
				error(L"out-of-bound index");
				return;
			}
			tcp_get_payload_modifiable((struct tcp *)$self)[index] = value;
		}
	}
};

struct stream
{
	%extend {
		void init(unsigned int seq)
		{
			tcp_stream_init($self, seq);
		}

		void push(struct tcp *tcp)
		{
			tcp_stream_push($self, tcp);
		}

		struct tcp *pop()
		{
			return tcp_stream_pop($self);
		}

		void seq(struct tcp *tcp)
		{
			tcp_stream_seq($self, tcp);
		}

		void ack(struct tcp *tcp)
		{
			tcp_stream_ack($self, tcp);
		}
	}
};

BASIC_STREAM(stream)

struct tcp {
	%extend {
		~tcp()
		{
			tcp_release($self);
		}
		unsigned int srcport;
		unsigned int dstport;
		unsigned int seq;
		unsigned int ack_seq;
		unsigned int res;
		unsigned int hdr_len;
		unsigned int window_size;
		unsigned int checksum;
		unsigned int urgent_pointer;

		%immutable;
		struct tcp_flags *flags;
		struct tcp_payload *payload;
		struct ipv4 *ip;
		const char *dissector;
		const char *next_dissector;

		bool verify_checksum();
		void compute_checksum();

		struct tcp_connection *newconnection()
		{
			return tcp_connection_new($self);
		}

		struct tcp_connection *getconnection(bool *OUTPUT)
		{
			return tcp_connection_get($self, OUTPUT);
		}

		bool connection_dropped()
		{
			return tcp_connection_isdropped($self);
		}

		struct ipv4 *forge();

		void drop()
		{
			tcp_action_drop($self);
		}

		bool valid();
	}
};


struct tcp_connection {
	%extend {
		struct lua_ref data;

		%immutable;
		struct ipv4_addr *srcip;
		struct ipv4_addr *dstip;
		unsigned int srcport;
		unsigned int dstport;

		void close();
		void drop();

		struct stream *stream(bool direction_in)
		{
			return tcp_connection_get_stream($self, direction_in);
		}
	}
};

%rename(dissect) tcp_dissect;
struct tcp *tcp_dissect(struct ipv4 *packet);

%{

struct ipv4_addr *tcp_connection_srcip_get(struct tcp_connection *tcp_conn) { return ipv4_addr_new(tcp_conn->srcip); }
struct ipv4_addr *tcp_connection_dstip_get(struct tcp_connection *tcp_conn) { return ipv4_addr_new(tcp_conn->dstip); }

#define TCP_CONN_INT_GET(field) \
	unsigned int tcp_connection_##field##_get(struct tcp_connection *tcp_conn) { return tcp_connection_get_##field(tcp_conn); }

TCP_CONN_INT_GET(srcport);
TCP_CONN_INT_GET(dstport);

struct lua_ref tcp_connection_data_get(struct tcp_connection *tcp_conn)
{
	return tcp_conn->lua_table;
}

void tcp_connection_data_set(struct tcp_connection *tcp_conn, struct lua_ref ref)
{
	lua_ref_clear(&tcp_conn->lua_table);
	tcp_conn->lua_table = ref;
}

#define TCP_INT_GETSET(field) \
	unsigned int tcp_##field##_get(struct tcp *tcp) { return tcp_get_##field(tcp); } \
	void tcp_##field##_set(struct tcp *tcp, unsigned int v) { tcp_set_##field(tcp, v); }

TCP_INT_GETSET(srcport);
TCP_INT_GETSET(dstport);
TCP_INT_GETSET(seq);
TCP_INT_GETSET(ack_seq);
TCP_INT_GETSET(hdr_len);
TCP_INT_GETSET(res);
TCP_INT_GETSET(window_size);
TCP_INT_GETSET(checksum);
TCP_INT_GETSET(urgent_pointer);

struct tcp_payload *tcp_payload_get(struct tcp *tcp) { return (struct tcp_payload *)tcp; }

struct tcp_flags *tcp_flags_get(struct tcp *tcp) { return (struct tcp_flags *)tcp; }

const char *tcp_dissector_get(struct tcp *tcp) { return "tcp"; }

const char *tcp_next_dissector_get(struct tcp *tcp) { return "tcp-connection"; }

struct ipv4 *tcp_ip_get(struct tcp *tcp) { return tcp->packet; }

#define TCP_FLAGS_GETSET(field) \
	bool tcp_flags_##field##_get(struct tcp_flags *flags) { return tcp_get_flags_##field((struct tcp *)flags); } \
	void tcp_flags_##field##_set(struct tcp_flags *flags, bool v) { return tcp_set_flags_##field((struct tcp *)flags, v); }

TCP_FLAGS_GETSET(fin);
TCP_FLAGS_GETSET(syn);
TCP_FLAGS_GETSET(rst);
TCP_FLAGS_GETSET(psh);
TCP_FLAGS_GETSET(ack);
TCP_FLAGS_GETSET(urg);
TCP_FLAGS_GETSET(ecn);
TCP_FLAGS_GETSET(cwr);

unsigned int tcp_flags_all_get(struct tcp_flags *flags) { return tcp_get_flags((struct tcp *)flags); }
void tcp_flags_all_set(struct tcp_flags *flags, unsigned int v) { return tcp_set_flags((struct tcp *)flags, v); }


%}

%luacode {
	haka.dissector {
		name = "tcp",
		dissect = tcp.dissect
	}

	require("tcp-connection")

	ipv4.register_proto(6, "tcp")
}
