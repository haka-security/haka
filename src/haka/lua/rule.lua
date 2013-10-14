
function haka.rule_summary()
	local total = 0

	for event, listeners in pairs(haka.context.connections) do
		haka.log.info("core", "%d rule(s) on event '%s'", #listeners, event.name)
		total = total + #listeners
	end
	
	if total == 0 then
		haka.log.warning("core", "no registered rule\n")
	else
		haka.log.info("core", "%d rule(s) registered\n", total)
	end
end

function haka.rule(r)
	haka.context.connections:register(r.hook, r.eval)
end

-- Load interactive.lua as a different file to allow to compile it
-- with the debugging information
require("interactive")
