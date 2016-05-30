
haka.alert{
	start_time = pkt.timestamp,
	description = "packet received",
	severity = 'medium',
	confidence = 'high',
	completion = 'failed',
	method = {
		description = "Packet sent on the network",
		ref = { "cve:2O13-XXX", "http://intranet/vulnid?id=115", "cwe:809" }
	},
	sources = { haka.alert.address(pkt.src, "evil.host.fqdn") },
	targets = { haka.alert.address(pkt.dst), haka.alert.service("tcp/22", "ssh") }
}
