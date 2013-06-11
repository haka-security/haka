%module packet
%{
#include <haka/packet.h>
#include <haka/packet_module.h>
#include <haka/log.h>
#include <haka/error.h>

#define PACKET_TABLE "haka_lua_packet_table"

void lua_registersingleton(lua_State *L, struct packet *pkt, int new_obj_index, swig_type_info *type_info)
{
	if(pkt->lua_state != L) {
		error(L"Incorrect lua state when pushing a packet");
		return;
	}
	if(new_obj_index < 0) new_obj_index = new_obj_index +1 +lua_gettop(L);
	lua_getfield(L,LUA_REGISTRYINDEX,PACKET_TABLE);
	lua_pushlightuserdata(L,pkt);
	lua_gettable(L,-2);
	lua_pushstring(L,type_info->name);
	lua_pushvalue(L,new_obj_index);
	lua_settable(L,-3);
	lua_pop(L,2);
}
void lua_getsingleton(lua_State *L, struct packet *pkt, swig_type_info *type_info)
{
	if(!pkt) {
		lua_pushnil(L);
		return;
	}
	if(pkt->lua_state != L) {
		error(L"Incorrect lua state when pushing a packet");
		return;
	}
	lua_getfield(L,LUA_REGISTRYINDEX,PACKET_TABLE);
	lua_pushlightuserdata(L,pkt);
	lua_gettable(L,-2);
	lua_pushstring(L,type_info->name);
	lua_gettable(L,-2);
	lua_remove(L,-2);
	lua_remove(L,-2);
}

void lua_pushppacket(lua_State *L, struct packet *pkt)
{
	if(!pkt->lua_state) {
		pkt->lua_state = L;
	} else if(pkt->lua_state != L) {
		error(L"Incorrect lua state when pushing a packet");
		return;
	}
	lua_getfield(L,LUA_REGISTRYINDEX,PACKET_TABLE);
	lua_pushlightuserdata(L,pkt);
	lua_newtable(L);
	lua_settable(L,-3);
	lua_pop(L,1);
	SWIG_NewPointerObj(L, pkt, SWIGTYPE_p_packet, 0);
	lua_registersingleton(L,pkt,-1,SWIGTYPE_p_packet);
}

void lua_invalidatepacket(struct packet *pkt)
{
	lua_State * L= pkt->lua_state;
	lua_getfield(L,LUA_REGISTRYINDEX,PACKET_TABLE);
	lua_pushlightuserdata(L,pkt);
	lua_gettable(L,-2);
	lua_pushnil(L);
	while(lua_next(L,-2)) {
		swig_lua_userdata* usr;
		swig_lua_class* clss;
		usr=(swig_lua_userdata*)lua_touserdata(L,-1);  /* get it */
		clss=(swig_lua_class*)usr->type->clientdata;  /* get the class */
		if (clss && clss->destructor)  /* there is a destroy fn */
		{
			clss->destructor(usr->ptr);  /* bye bye */
		}
		usr->ptr = NULL;
		lua_pop(L,1);
	}
	lua_pop(L,1);
	lua_pushlightuserdata(L,pkt);
	lua_pushnil(L);
	lua_settable(L,-3);
	lua_pop(L,1);
}

%}

%include haka/swig.si
%include haka/packet.si
PACKET_DEPENDANT_GETTER(packet::forge,result,SWIGTYPE_p_packet);

%rename(ACCEPT) FILTER_ACCEPT;
%rename(DROP) FILTER_DROP;

enum filter_result { FILTER_ACCEPT, FILTER_DROP };

%nodefaultctor;
%nodefaultdtor;
%init{
	lua_newtable(L);
	lua_setfield(L,LUA_REGISTRYINDEX,PACKET_TABLE);
}

struct packet {
	%extend {
		%immutable;
		size_t length;
		const char *dissector;
		const char *next_dissector;

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
