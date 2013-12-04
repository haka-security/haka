
local my_alert = haka.alert{ severity = 'low', sources = { haka.alert.address(pkt.src) } }
haka.alert.update(my_alert, { completion = 'failed' } )
