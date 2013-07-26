%module tcp

%{
#include <haka/stream.h>
#include <haka/tcp.h>
#include <haka/tcp-connection.h>
#include <haka/tcp-stream.h>
#include <haka/log.h>

struct tcp_payload;
struct tcp_flags;

%}

%include "haka/lua/ipv4-addr.si"
%include "haka/lua/swig.si"
%include "haka/lua/stream.si"
%include "haka/lua/ref.si"
%include "haka/lua/ipv4.si"
%include "typemaps.i"

%nodefaultctor;
%nodefaultdtor;

LUA_OBJECT_CAST(struct tcp_flags, struct tcp);

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

STRUCT_UNKNOWN_KEY_ERROR(tcp_flags);

LUA_OBJECT_CAST(struct tcp_payload, struct tcp);

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

		void resize(int size)
		{
			tcp_resize_payload((struct tcp *)$self, size);
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(tcp_payload);

LUA_OBJECT(struct tcp);
LUA_OBJECT(struct tcp_connection);
LUA_OBJECT(struct stream);

%newobject stream::pop;

struct stream
{
	%extend {
		%immutable;
		unsigned int lastseq;

		void init(unsigned int seq)
		{
			tcp_stream_init($self, seq);
		}

		void push(struct tcp *DISOWN_SUCCESS_ONLY)
		{
			tcp_stream_push($self, DISOWN_SUCCESS_ONLY);
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

%newobject tcp::forge;

struct tcp {
	%extend {
		~tcp()
		{
			if ($self)
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

		struct tcp_connection *getconnection(bool *OUTPUT1, bool *OUTPUT2)
		{
			return tcp_connection_get($self, OUTPUT1, OUTPUT2);
		}

		struct ipv4 *forge();

		void drop()
		{
			tcp_action_drop($self);
		}

		void send()
		{
			tcp_action_send($self);
		}

		bool valid();
	}
};

STRUCT_UNKNOWN_KEY_ERROR(tcp);

%newobject tcp_connection::srcip;
%newobject tcp_connection::dstip;

struct tcp_connection {
	%extend {
		struct lua_ref data;

		%immutable;
		struct ipv4_addr *srcip;
		struct ipv4_addr *dstip;
		unsigned int srcport;
		unsigned int dstport;

		~tcp_connection()
		{
			if ($self)
				tcp_connection_drop($self);
		}

		void close();
		void drop();

		struct stream *stream(bool direction_in)
		{
			return tcp_connection_get_stream($self, direction_in);
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(tcp_connection);

%rename(dissect) tcp_dissect;
%newobject tcp_dissect;
struct tcp *tcp_dissect(struct ipv4 *DISOWN_SUCCESS_ONLY);

%rename(create) tcp_create;
%newobject tcp_create;
struct tcp *tcp_create(struct ipv4 *DISOWN_SUCCESS_ONLY);

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

unsigned int stream_lastseq_get(struct stream *s) { return tcp_stream_lastseq(s); }

%}

%luacode {
	haka.dissector {
		name = "tcp",
		dissect = tcp.dissect
	}

	require("tcp-connection")

	local ipv4 = require("ipv4")
	ipv4.register_proto(6, "tcp")
}
