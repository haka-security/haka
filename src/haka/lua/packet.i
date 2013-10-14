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

%nodefaultctor;
%nodefaultdtor;

%newobject packet::forge;

struct packet {
	%extend {
		%immutable;
		size_t length;
		const struct time *timestamp;
		const char *dissector;
		const char *name;

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

		void resize(int size);
		void drop();
		%rename(continue) _continue;
		bool _continue();
	}
};

%rename(NORMAL) MODE_NORMAL;
%rename(PASSTHROUGH) MODE_PASSTHROUGH;

enum packet_mode { MODE_NORMAL, MODE_PASSTHROUGH };

%rename(mode) packet_mode;
enum packet_mode packet_mode();

%rename(_create) packet_new;
%newobject packet_new;
struct packet *packet_new(int size = 0);

%rename(_send) packet__send;
void packet__send(struct packet *pkt);

%rename(_inject) packet__inject;
void packet__inject(struct packet *pkt);

%{
size_t packet_length_get(struct packet *pkt) {
	return packet_length(pkt);
}

const struct time *packet_timestamp_get(struct packet *pkt) {
	return packet_timestamp(pkt);
}

const char *packet_dissector_get(struct packet *pkt) {
	return packet_dissector(pkt);
}

const char *packet_name_get(struct packet *pkt) {
	return "raw";
}

void packet__send(struct packet *pkt)
{
	assert(pkt);

	switch (packet_state(pkt)) {
	case STATUS_FORGED:
	case STATUS_NORMAL:
		packet_accept(pkt);
		break;

	case STATUS_SENT:
		error(L"operation not supported");
		return;

	default:
		assert(0);
		return;
	}
}

void packet__inject(struct packet *pkt)
{
	assert(pkt);

	switch (packet_state(pkt)) {
	case STATUS_FORGED:
		packet_send(pkt);
		break;

	case STATUS_NORMAL:
	case STATUS_SENT:
		error(L"operation not supported");
		return;

	default:
		assert(0);
		return;
	}
}

bool packet__continue(struct packet *pkt)
{
	assert(pkt);
	return packet_state(pkt) != STATUS_SENT;
}
%}

%luacode{
	local this = unpack({...})

	local raw_dissector = haka.dissector.new{
		type = haka.dissector.PacketDissector,
		name = 'raw'
	}

	raw_dissector.options.drop_unknown_dissector = false

	function raw_dissector.method:emit()
		if not haka.pcall(haka.context.signal, haka.context, self, raw_dissector.events.receive_packet) then
			return self:drop()
		end

		if not self:continue() then
			return
		end

		local dissector = self.dissector
		if dissector then
			local next_dissector = haka.dissector.get(dissector)
			if next_dissector then
				return next_dissector.receive(self)
			else
				if raw_dissector.options.drop_unknown_dissector then
					haka.log.error("raw", "dissector '%s' is unknown", dissector)
					return self:drop()
				else
					return self:send()
				end
			end
		else
			return self:send()
		end
	end

	function raw_dissector.receive(pkt)
		return pkt:emit()
	end

	function raw_dissector.create(size)
		return this._create(size or 0)
	end

	function raw_dissector.method:send()
		if not haka.pcall(haka.context.signal, haka.context, self, raw_dissector.events.send_packet) then
			return self:drop()
		end

		if not self:continue() then
			return
		end

		return this._send(self)
	end

	function raw_dissector.method:inject()
		return this._inject(self)
	end

	swig.getclassmetatable('packet')['.fn'].send = raw_dissector.method.send
	swig.getclassmetatable('packet')['.fn'].emit = raw_dissector.method.emit
	swig.getclassmetatable('packet')['.fn'].inject = raw_dissector.method.inject

	function haka.filter(pkt)
		raw_dissector.receive(pkt)
	end
}
