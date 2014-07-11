local smtp = require('smtp_partial')

smtp.install_tcp_rule(25)

haka.rule{
	hook = smtp.events.mail_content,
	options = {
		streamed = true,
	},
	eval = function (flow, iter)
		print("== Mail Content ==")
		for sub in iter:foreach_available() do
			io.write(sub:asstring())
		end
		print("== End Mail Content ==")
	end
}
