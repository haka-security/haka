-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local tcp_connection = require("protocol/tcp_connection")
local module = {}

--
-- Constants
--
local CMD = {
	  ['HELO'] = 'required',
	  ['EHLO'] = 'required',
	  ['MAIL'] = 'required',
	  ['RCPT'] = 'required',
	  ['DATA'] = 'none',
	 ['RESET'] = 'none',
	['VERIFY'] = 'required',
	['EXPAND'] = 'required',
	  ['HELP'] = 'optional',
	  ['NOOP'] = 'optional',
	  ['QUIT'] = 'none'
}

--
-- Dissector
--
local SmtpDissector = haka.dissector.new{
	type = tcp_connection.helper.TcpFlowDissector,
	name = 'smtp'
}

function module.dissect(flow)
	SmtpDissector:dissect(flow)
end

function module.install_tcp_rule(port)
	SmtpDissector:install_tcp_rule(port)
end

function SmtpDissector.method:push_data(sub, last)
	assert(self.mail)

	local mail_iter

	if sub then mail_iter = self.mail:push(sub) end
	if last then self.mail:finish() end

	self:trigger('mail_content', self.mail, mail_iter)
	self.mail:pop()
end

--
-- Events
--
SmtpDissector:register_event('command')
SmtpDissector:register_event('response')
SmtpDissector:register_streamed_event('mail_content')

--
--	Grammar
--
SmtpDissector.grammar = haka.grammar.new("smtp", function ()
	-- terminal tokens
	WS = token('[[:blank:]]+')
	CRLF = token('[%r]?[%n]')
	SEP = token('[- ]')
	COMMAND = token('[[:alpha:]]+')
	MESSAGE = token('[^%r%n]*')
	CODE = token('[0-9]{3}')

	PARAM = record{
		WS,
		field('parameter', MESSAGE)
	}

	-- smtp command
	smtp_command = record{
		field('command', COMMAND),
		branch(
			{
				required = PARAM,
				optional = optional(PARAM,
					function(self, ctx)
						local la = ctx:lookahead()
						return not (la == 0xa or la == 0xd)
					end
				),
				none = empty()
			},
			function (self, ctx)
				return CMD[self.command]
			end
		),
		CRLF
	}

	-- smtp response
	smtp_response = record{
		field('code', CODE),
		field('sep', SEP),
		field('parameter', MESSAGE),
		CRLF
	}

	smtp_responses = field('responses',
		array(smtp_response)
			:untilcond(function (elem, ctx)
				return elem and elem.sep == ' '
			end)
		)

	-- smtp data
	smtp_data = record{
		field('data', bytes()
			:untiltoken("%r?%n%.%r?%n")
			:chunked(function (self, sub, last, ctx)
				ctx.user:push_data(sub, last)
			end)),
		token("%r?%n%.%r?%n")
	}

	export(smtp_command, smtp_responses, smtp_data)
end)

--
-- State machine
--

SmtpDissector.state_machine = haka.state_machine.new("smtp", function ()
	state_type(BidirectionalState)

	session_initiation = state(nil, SmtpDissector.grammar.smtp_responses)
	client_initiation = state(SmtpDissector.grammar.smtp_command, nil)
	response = state(nil, SmtpDissector.grammar.smtp_responses)
	command = state(SmtpDissector.grammar.smtp_command, nil)
	data_transmission = state(SmtpDissector.grammar.smtp_data, nil)

	any:on{
		event = events.fail,
		execute = function (self)
			self:drop()
		end,
	}

	any:on{
		event = events.missing_grammar,
		execute = function (self, direction, payload)
			local description
			if direction == 'up' then
				description = "unexpected client command"
			else
				description = "unexpected server response"
			end
			haka.alert{
				description = description,
				severity = 'low'
			}
		end,
		jump = fail,
	}

	session_initiation:on{
		event = events.parse_error,
		execute = function (self, err)
			haka.alert{
				description = string.format("invalid smtp response %s", err),
				severity = 'high'
			}
		end,
		jump = fail,
	}

	session_initiation:on{
		event = events.down,
		when = function (self, res) return res.responses[1].code == '220' end,
		execute = function (self, res)
			self:trigger('response', res)
		end,
		jump = client_initiation,
	}

	session_initiation:on{
		event = events.down,
		execute = function (self, res)
			haka.alert{
				description = string.format("unavailable service: %s", status),
				severity = 'low'
			}
		end,
		jump = fail,
	}

	client_initiation:on{
		event = events.parse_error,
		execute = function (self, err)
			haka.alert{
				description = string.format("invalid smtp command %s", err),
				severity = 'low'
			}
		end,
		jump = fail,
	}

	client_initiation:on{
		event = events.up,
		when = function (self, res)
			local command = string.upper(res.command)
			return command == 'EHLO' or command == 'HELO'
		end,
		execute = function (self, res)
			self.command = res
			self:trigger('command', res)
		end,
		jump = response,
	}

	client_initiation:on{
		event = events.up,
		execute = function (self, res)
			haka.alert{
				description = string.format("invalid client initiation command"),
				severity = 'low'
			}
		end,
		jump = fail,
	}

	response:on{
		event = events.parse_error,
		execute = function (self, err)
			haka.alert{
				description = string.format("invalid smtp response %s", err),
				severity = 'high'
			}
		end,
		jump = fail,
	}

	response:on{
		event = events.down,
		when = function (self, res)
			return res.responses[1].code == '354'
		end,
		execute = function (self, res)
			self:trigger('response', res)
		end,
		jump = data_transmission,
	}

	response:on{
		event = events.down,
		when = function (self, res)
			return res.responses[1].code == '221'
		end,
		execute = function (self, res)
			self:trigger('response', res)
		end,
		jump = finish,
	}

	response:on{
		event = events.down,
		execute = function (self, res)
			self:trigger('response', res)
		end,
		jump = command,
	}

	command:on{
		event = events.parse_error,
		execute = function (self, err)
			haka.alert{
				description = string.format("invalid smtp command %s", err),
				severity = 'low'
			}
		end,
		jump = fail,
	}

	command:on{
		event = events.up,
		execute = function (self, res)
			self.command = res
			self:trigger('command', res)
		end,
		jump = response,
	}

	data_transmission:on{
		event = events.enter,
		execute = function (self)
			self.mail = haka.vbuffer_sub_stream()
		end,
	}

	data_transmission:on{
		event = events.parse_error,
		execute = function (self, err)
			haka.alert{
				description = string.format("invalid data blob %s", err),
				severity = 'low'
			}
		end,
		jump = fail,
	}

	data_transmission:on{
		event = events.up,
		jump = response,
	}

	data_transmission:on{
		event = events.leave,
		execute = function (self)
			self.mail = nil
		end,
	}

	initial(session_initiation)
end)

module.events = SmtpDissector.events

return module
