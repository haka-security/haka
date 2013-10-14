
local rule_group = class('RuleGroup')

function rule_group.method:__init(args)
	self.rules = {}
	self.init = args.init
	self.continue = args.continue
	self.fini = args.fini
end

function rule_group.method:rule(eval)
	table.insert(self.rules, eval)
end

function rule_group.method:eval(...)
	self.init(...)

	for _, rule in ipairs(self.rules) do
		local ret = rule(...)

		if not self.continue(ret, ...) then
			return
		end
	end

	self.fini(...)
end


function haka.rule_group(args)
	if not args.hook then
		error("no hook defined for rule group")
	end

	local group = rule_group:new(args)
	haka.context.connections:register(args.hook, function (...) group:eval(...) end)
	return group
end
