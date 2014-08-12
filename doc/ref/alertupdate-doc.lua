
local my_alert = haka.alert{ severity = 'low', sources = { haka.alert.address(pkt.src) } }
my_alert:update{ completion = 'failed' }
