/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module tcp

%{
#include <haka/tcp.h>
#include <haka/tcp-connection.h>
#include <haka/tcp-stream.h>
#include <haka/log.h>

struct tcp_payload;
struct tcp_flags;
struct tcp_stream;

%}

%include "haka/lua/ipv4-addr.si"
%include "haka/lua/swig.si"
%include "haka/lua/ref.si"
%include "haka/lua/ipv4.si"
%include "haka/lua/vbuffer.si"
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

LUA_OBJECT(struct tcp);
LUA_OBJECT(struct tcp_connection);
LUA_OBJECT(struct tcp_stream);

%newobject tcp_stream::_push;
%newobject tcp_stream::_pop;

struct tcp_stream
{
	%extend {
		tcp_stream()
		{
			struct tcp_stream *stream = malloc(sizeof(struct tcp_stream));
			if (!stream) {
				error(L"memory error");
				return NULL;
			}

			tcp_stream_create(stream);

			return stream;
		}

		~tcp_stream()
		{
			if ($self) {
				tcp_stream_clear($self);
				free($self);
			}
		}

		%immutable;
		unsigned int lastseq { return tcp_stream_lastseq($self); };
		struct vbuffer_stream *stream { return &$self->stream; };

		%rename(init) _init;
		void _init(unsigned int seq)
		{
			tcp_stream_init($self, seq);
		}

		%rename(push) _push;
		struct vbuffer_iterator *_push(struct tcp *DISOWN_SUCCESS_ONLY)
		{
			struct vbuffer_iterator *iter = malloc(sizeof(struct vbuffer_iterator));
			if (!iter) {
				free(iter);
				error(L"memory error");
				return NULL;
			}

			if (!tcp_stream_push($self, DISOWN_SUCCESS_ONLY, iter)) {
				free(iter);
				return NULL;
			}

			vbuffer_iterator_register(iter);
			iter->meter = 0;

			return iter;
		}

		%rename(pop) _pop;
		struct tcp *_pop()
		{
			return tcp_stream_pop($self);
		}

		%rename(seq) _seq;
		void _seq(struct tcp *tcp)
		{
			tcp_stream_seq($self, tcp);
		}

		%rename(ack) _ack;
		void _ack(struct tcp *tcp)
		{
			tcp_stream_ack($self, tcp);
		}

		%rename(clear) _clear;
		void _clear()
		{
			tcp_stream_clear($self);
			free($self);
		}
	}
};

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
		struct tcp_flags *flags { return (struct tcp_flags *)$self; }
		struct vbuffer *payload { return &$self->payload; }
		struct ipv4 *ip { return $self->packet; }
		const char *name { return "tcp"; }

		bool verify_checksum();
		void compute_checksum();

		void drop()
		{
			tcp_action_drop($self);
		}

		bool _continue()
		{
			assert($self);
			return $self->packet != NULL;
		}
	}
};

STRUCT_UNKNOWN_KEY_ERROR(tcp);

%rename(_dissect) tcp_dissect;
%newobject tcp_dissect;
struct tcp *tcp_dissect(struct ipv4 *DISOWN_SUCCESS_ONLY);

%rename(_create) tcp_create;
%newobject tcp_create;
struct tcp *tcp_create(struct ipv4 *DISOWN_SUCCESS_ONLY);

%rename(_forge) tcp_forge;
%newobject tcp_forge;
struct ipv4 *tcp_forge(struct tcp *pkt);

%{

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
	local this = unpack({...})

	local tcp_dissector = haka.dissector.new{
		type = haka.dissector.PacketDissector,
		name = 'tcp'
	}

	function tcp_dissector:new(pkt)
		return this._dissect(pkt)
	end

	function tcp_dissector.method:receive()
		haka.context:signal(self, tcp_dissector.events['receive_packet'])

		local next_dissector = haka.dissector.get('tcp-connection')
		if next_dissector then
			return next_dissector:receive(self)
		else
			return self:send()
		end
	end

	function tcp_dissector:create(pkt)
		return this._create(pkt)
	end

	function tcp_dissector.method:send()
		haka.context:signal(self, tcp_dissector.events['send_packet'])

		local pkt = this._forge(self)
		while pkt do
			pkt:send()
			pkt = this._forge(self)
		end
	end

	function tcp_dissector.method:inject()
		local pkt = this._forge(self)
		while pkt do
			pkt:inject()
			pkt = this._forge(self)
		end
	end

	function tcp_dissector.method:continue()
		if not self:_continue() then
			return haka.abort()
		end
	end

	swig.getclassmetatable('tcp')['.fn'].send = tcp_dissector.method.send
	swig.getclassmetatable('tcp')['.fn'].receive = tcp_dissector.method.receive
	swig.getclassmetatable('tcp')['.fn'].inject = tcp_dissector.method.inject
	swig.getclassmetatable('tcp')['.fn'].continue = tcp_dissector.method.continue
	swig.getclassmetatable('tcp')['.fn'].error = swig.getclassmetatable('tcp')['.fn'].drop

	local ipv4 = require("protocol/ipv4")
	ipv4.register_protocol(6, tcp_dissector)
}
