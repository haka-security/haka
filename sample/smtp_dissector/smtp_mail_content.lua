local smtp = require('smtp')

smtp.install_tcp_rule(25)

haka.rule{
	hook = smtp.events.mail_content,
	options = {
		streamed = true,
	},
	eval = function (flow, iter)
		local mail_content = {}

		for sub in iter:foreach_available() do
			table.insert(mail_content, sub:asstring())
		end

		print("== Mail Content ==")
		print(table.concat(mail_content))
		print("== End Mail Content ==")
	end
}
