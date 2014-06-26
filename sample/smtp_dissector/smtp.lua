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
	type = haka.helper.FlowDissector,
	name = 'smtp'
}

SmtpDissector.property.connection = {
	get = function (self)
		self.connection = self.flow.connection
		return self.connection
	end
}

function SmtpDissector.method:__init(flow)
	class.super(SmtpDissector).__init(self)
	self.flow = flow
	self.state = SmtpDissector.state_machine:instanciate(self)
end

function SmtpDissector.method:continue()
	if not self.flow then
		haka.abort()
	end
end

function SmtpDissector.method:drop()
	self.flow:drop()
	self.flow = nil
end

function SmtpDissector.method:reset()
	self.flow:reset()
	self.flow = nil
end

function SmtpDissector.method:receive(stream, current, direction)
	return haka.dissector.pcall(self, function ()
		self.flow:streamed(stream, self.receive_streamed, self, current, direction)

		if self.flow then
			self.flow:send(direction)
		end
	end)
end

function SmtpDissector.method:receive_streamed(iter, direction)
	while iter:wait() do
		self.state:update(iter, direction)
		self:continue()
	end
end


function module.install_tcp_rule(port)
	haka.rule{
		hook = tcp_connection.events.new_connection,
		eval = function (flow, pkt)
			if pkt.dstport == port then
				haka.log.debug('smtp', "selecting smtp dissector on flow")
				module.dissect(flow)
			end
		end
	}
end

function module.dissect(flow)
	flow:select_next_dissector(SmtpDissector:new(flow))
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
	SEP = field('sep', token('[- ]'))
	COMMAND = field('command', token('[[:alpha:]]+'))
	MESSAGE = field('parameter', token('[^%r%n]*'))
	CODE = field('code', token('[0-9]{3}'))
	DATA = field('data', raw_token("[^%r%n]*%r%n"))

	PARAM = record{
		WS,
		MESSAGE
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
		CODE,
		SEP,
		MESSAGE,
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
		DATA
	}

	export(smtp_command, smtp_responses, smtp_data)
end)

--
-- State machine
--

SmtpDissector.state_machine = haka.state_machine.new("smtp", function ()
	state_type(BidirectionnalState)

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
		when = function (self, res) return res.data:asstring() == '.\r\n' end,
		execute = function (self, res)
			self.mail:finish()
			self:trigger('mail_content', self.mail, nil)
			self.mail:pop()
		end,
		jump = response,
	}

	data_transmission:on{
		event = events.up,
		execute = function (self, res)
			local mail_iter = self.mail:push(res.data)
			self:trigger('mail_content', self.mail, mail_iter)
			self.mail:pop()
		end,
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

