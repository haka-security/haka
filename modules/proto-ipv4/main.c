
#include <haka/module.h>


static int init(int argc, char *argv[])
{
	return 0;
}

static void cleanup()
{
}

/**
 * @defgroup IPv4 IPv4
 * @brief IPv4 dissector API fucntions and structures.
 * @author Arkoon Network Security
 * @ingroup ExternProtocolModule
 *
 * # Description
 *
 * The module dissects IPv4 Header
 * 
 * The modules allows to access and modify all IP header field values 
 * 
 * # Usage
 *
 * ### Lua
 * ~~~~~~~~{.lua}
 * ipv4 = require("proto-ipv4")
 * app.install_filter(function (pkt)
 * ip = ipv4(pkt)
 * --[ get ip header field value (print)
 * log.debug("filter", "version: %d", ip.version)
 * --[ set ip header field value
 * ip.ttl = 20
 * return packet.ACCEPT
 * end)
 * ~~~~~~~~
 */

struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        L"IPv4",
	description: L"IPv4 protocol",
	author:      L"Arkoon Network Security",
	init:        init,
	cleanup:     cleanup
};
