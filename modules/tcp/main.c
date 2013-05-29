
#include <haka/module.h>


static int init(int argc, char *argv[])
{
	return 0;
}

static void cleanup()
{
}

/**
 * @defgroup TCP TCP
 * @brief TCP dissector API fucntions and structures.
 * @author Arkoon Network Security
 * @ingroup ExternProtocolModule
 *
 * # Description
 *
 * The module dissects TCP Header
 *
 * The modules allows to access and modify all IP header field values
 *
 * # Usage
 *
 * ### Lua
 * ~~~~~~~~{.lua}
 * require("proto-ipv4")
 * require("proto-tcp")
 * app.install_filter(function (pkt)
 * ip_h = ipv4(pkt)
 * tcp_h = tcp(ip_h)
 * --[ get ip source port value (print)
 * log.debug("filter", "source port: %d", tcp_h.srcport)
 * --[ set desination port value
 * tcp_h.dstport = 8080
 * return packet.ACCEPT
 * end)
 * ~~~~~~~~
 */


struct module HAKA_MODULE = {
	type:        MODULE_EXTENSION,
	name:        L"TCP",
	description: L"TCP protocol",
	author:      L"Arkoon Network Security",
	init:        init,
	cleanup:     cleanup
};
