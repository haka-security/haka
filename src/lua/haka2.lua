
module("haka2", package.seeall)

local __dissectors = {}
local __rules = {}

function dissector(d)
	if d.name == nil or d.dissect == nil then
		if haka.app.currentThread() == 0 then
			haka.log.error("core", "registering invalid dissector: '%s'", d.name)
		end
	else
		if not __dissectors[d.name] then
			__dissectors[d.name] = d
			if haka.app.currentThread() == 0 then
				haka.log.info("core", "registering new dissector: '%s'", d.name)
			end
		end
	end
end

function rule(r)
	for _, h in pairs(r.hooks) do
		if not __rules[h] then
			__rules[h] = {}
		end

		table.insert(__rules[h], r)
	end
end

function rule_summary()
	if next(__rules) == nil then
		haka.log.warning("core", "no registered rule\n")
	else
		local total = 0
		for hook, rules in pairs(__rules) do
			haka.log.info("core", "%d rule(s) on hook '%s'", #rules, hook)
			total = total + #rules;
		end
		haka.log.info("core", "%d rule(s) registered\n", total)
	end
end

local function eval_rules(hook, pkt)
	local rules = __rules[hook]
	if rules then
		for _, r in pairs(rules) do
			r.eval(nil, pkt)
			if not pkt:valid() then
				return
			end
		end
	end
end

local function filter_down(pkt)
	if pkt.dissector then
		eval_rules(pkt.dissector .. '-down', pkt)
	end

	while true do
		local pktdown = pkt:forge()
		if pktdown then
			filter_down(pktdown)
		else
			break
		end
	end
end

local function get_dissector(name)
	return __dissectors[name]
end

function filter(pkt)
	local dissect = get_dissector(pkt.nextDissector)
	while dissect do
		local nextpkt = dissect.dissect(pkt)
		if not nextpkt then
			break
		end
		if not nextpkt:valid() then
			break
		end

		pkt = nextpkt
		eval_rules(dissect.name .. '-up', pkt)
		dissect = nil

		if not pkt:valid() then
			break
		end

		if pkt.nextDissector then
			dissect = get_dissector(pkt.nextDissector)
			if not dissect then
				haka.log.error("core", "cannot create dissector '%s': not registered", pkt.nextDissector)
			end
		end
	end

	filter_down(pkt)
end
