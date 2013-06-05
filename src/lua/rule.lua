
local __dissectors = {}
local __rule_groups = {}

function haka.dissector(d)
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

function haka.rule_group(group)
	group.rule = function (g, r)
		for _, h in pairs(r.hooks) do
			if not g.__hooks[h] then
				g.__hooks[h] = {}
			end

			table.insert(g.__hooks[h], r)
		end
	end

	group.__hooks = {}

	__rule_groups[group.name] = group
	return group
end

local __default_rule_group = haka.rule_group {
	name = "default"
}

function haka.rule(r)
	return __default_rule_group:rule(r)
end

local function _rule_group_eval(hook, group, pkt)
	local rules = group.__hooks[hook]
	if rules then
		if group.init then
			group:init(pkt)
		end

		for _, r in pairs(rules) do
			local ret = r.eval(nil, pkt)
			if group.continue then
				if not group:continue(ret) then
					return true
				end
			end

			if not pkt:valid() then
				return false
			end
		end

		if group.fini then
			group:fini(pkt)
		end
	end
	return true
end

function haka.rule_summary()
	local total = 0
	local rule_count = {}

	for _, group in pairs(__rule_groups) do
		for hook, rules in pairs(group.__hooks) do
			if not rule_count[hook] then
				rule_count[hook] = 0
			end

			rule_count[hook] = rule_count[hook] + #rules
			total = total + #rules
		end
	end

	if total == 0 then
		haka.log.warning("core", "no registered rule\n")
	else
		for hook, count in pairs(rule_count) do
			haka.log.info("core", "%d rule(s) on hook '%s'", count, hook)
		end

		haka.log.info("core", "%d rule(s) registered\n", total)
	end
end

local function eval_rules(hook, pkt)
	for _, group in pairs(__rule_groups) do
		if not _rule_group_eval(hook, group, pkt) then
			return false
		end
	end
	return true
end
local function filter_down(pkt)
        if pkt.dissector then
                eval_rules(pkt.dissector .. '-down', pkt)
        end

        local pkts = {}
        while true do
                local pktdown = pkt:forge()
                if pktdown then
                        table.insert(pkts, pktdown)
                else
                        break
                end
        end

        for _, pktdown in pairs(pkts) do
                filter_down(pktdown)
        end
end

local function get_dissector(name)
	return __dissectors[name]
end

function haka.rule_hook(name, pkt)
	eval_rules(name, pkt)
	return not pkt:valid()
end

function haka.filter(pkt)
	local guard = {pkt} -- TODO: to be removed (need to avoid Lua to destroy some object still used)
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
		table.insert(guard, pkt)
		eval_rules(dissect.name .. '-up', pkt)
		dissect = nil

		if not pkt:valid() then
			break
		end

		if pkt.nextDissector then
			dissect = get_dissector(pkt.nextDissector)
			if not dissect then
				haka.log.error("core", "cannot create dissector '%s': not registered", pkt.nextDissector)
				pkt:drop()
			end
		end
	end

	filter_down(pkt)
end
