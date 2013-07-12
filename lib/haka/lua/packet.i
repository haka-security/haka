%module packet

%include "haka/lua/swig.si"
%include "haka/lua/packet.si"
%include "haka/lua/object.si"

%{
#include <haka/packet.h>
#include <haka/packet_module.h>
#include <haka/error.h>

void lua_pushppacket(lua_State *L, struct packet *pkt)
{
	if (!lua_object_push(L, pkt, &pkt->lua_object, SWIGTYPE_p_packet, 1)) {
		lua_error(L);
	}
}

%}

%rename(ACCEPT) FILTER_ACCEPT;
%rename(DROP) FILTER_DROP;

enum filter_result { FILTER_ACCEPT, FILTER_DROP };

%nodefaultctor;
%nodefaultdtor;

%newobject packet::forge;

struct packet {
	%extend {
		%immutable;
		size_t length;
		const char *dissector;
		const char *next_dissector;

		~packet()
		{
			if ($self) {
				packet_release($self);
			}
		}

		size_t __len(void *dummy)
		{
			return packet_length($self);
		}

		int __getitem(int index)
		{
			--index;
			if (index < 0 || index >= packet_length($self)) {
				error(L"out-of-bound index");
				return 0;
			}
			return packet_data($self)[index];
		}

		void __setitem(int index, int value)
		{
			--index;
			if (index < 0 || index >= packet_length($self)) {
				error(L"out-of-bound index");
				return;
			}
			packet_data_modifiable($self)[index] = value;
		}

		%rename(drop) _drop;
		void _drop()
		{
			packet_drop($self);
		}

		%rename(accept) _accept;
		void _accept()
		{
			packet_accept($self);
		}

		struct packet *forge()
		{
			packet_accept($self);
			return NULL;
		}
	}
};

%rename(NORMAL) MODE_NORMAL;
%rename(PASSTHROUGH) MODE_PASSTHROUGH;

enum packet_mode { MODE_NORMAL, MODE_PASSTHROUGH };

%rename(mode) packet_mode;
enum packet_mode packet_mode();

%{
size_t packet_length_get(struct packet *pkt) {
	return packet_length(pkt);
}

const char *packet_dissector_get(struct packet *pkt) {
	return "raw";
}

const char *packet_next_dissector_get(struct packet *pkt) {
	return packet_dissector(pkt);
}
%}
