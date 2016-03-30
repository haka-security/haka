------------------------------------
-- Firewall rules
------------------------------------

local client_network = ipv4.network("192.168.10.0/25");
local server_network = ipv4.network("192.168.20.0/25");

haka.policy {
	on = tcp_connection.policies.new_connection,
	name = "drop by default",
	action = haka.policy.drop_with_alert{
		description = "drop by default"
	}
}

haka.policy {
	on = tcp_connection.policies.new_connection,
	srcip = haka.policy.ipv4.in_network(client_network),
	dstip = haka.policy.ipv4.in_network(server_network),
	dstport = 80,
	name = "authorizing http traffic",
	action = function (policy, ctx)
		http.dissect(ctx)
	end
}

haka.policy {
	on = tcp_connection.policies.new_connection,
	srcip = haka.policy.ipv4.in_network(client_network),
	dstip = haka.policy.ipv4.in_network(server_network),
	dstport = 22,
	name = "authorizing ssh traffic (no available dissector)",
	action = haka.policy.accept
}
