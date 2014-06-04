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
	type = haka.dissector.FlowDissector,
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
	self.state = SmtpDissector.states:instanciate(self)
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
		self.state:transition('update', direction, iter)
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
SmtpDissector:register_event('mail_content', nil, haka.dissector.FlowDissector.stream_wrapper)

--
--	Grammar
--
SmtpDissector.grammar = haka.grammar.new("smtp", function ()
	-- terminal tokens
	EMPTY = token('')
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
				none = EMPTY
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
SmtpDissector.states = haka.state_machine("smtp", function ()
	session_initiation = state {
		up = function (self, iter)
			haka.alert{
				description = "unexpected client command",
				severity = 'low'
			}
			return 'error'
		end,
		down = function (self, iter)
			local err
			self.response, err = SmtpDissector.grammar.smtp_responses:parse(iter, nil, self)
			if err then
				haka.alert{
					description = string.format("invalid smtp response %s", err),
					severity = 'high'
				}
				return 'error'
			end
			local status = self.response.responses[1].code
			if status == '220' then
				self:trigger('response', self.response)
				return 'client_initiation'
			else
				haka.alert{
					description = string.format("unavailable service: %s", status),
					severity = 'low'
				}
				return 'error'
			end
		end
	}

	client_initiation = state {
		up = function (self, iter)
			local err
			self.command, err = SmtpDissector.grammar.smtp_command:parse(iter, nil, self)
			if err then
				haka.alert{
					description = string.format("invalid smtp command %s", err),
					severity = 'low'
				}
				return 'error'
			end
			local command = string.upper(self.command.command)
			if command == 'EHLO' or command == 'HELO' then
				self:trigger('command', self.command)
				return 'response'
			else
				haka.alert{
					description = string.format("invalid client initiation command"),
					severity = 'low'
				}
				return 'error'
			end
		end,
		down = function (self, iter)
			haka.alert{
				description = string.format("unexpected server response"),
				severity = 'low'
			}
			return 'error'
		end,
	}

	response = state {
		up = function (self, iter)
			haka.alert{
				description = "unexpected client command",
				severity = 'low'
			}
			return 'error'
		end,
		down = function (self, iter)
			local err
			self.response, err = SmtpDissector.grammar.smtp_responses:parse(iter, nil, self)
			if err then
				haka.alert{
					description = string.format("invalid smtp response %s", err),
					severity = 'high'
				}
				return 'error'
			end
			self:trigger('response', self.response)
			local status = self.response.responses[1].code
			if status == '354' then
				return 'data_transmission'
			elseif status == 221 then
				return 'finish'
			else
				return 'command'
			end
		end
	}

	command = state {
		up = function (self, iter)
			local err
			self.command, err = SmtpDissector.grammar.smtp_command:parse(iter, nil, self)
			if err then
				haka.alert{
					description = string.format("invalid smtp command %s", err),
					severity = 'low'
				}
				return 'error'
			end
			self:trigger('command', self.command)
			return 'response'
		end,
		down = function (self, iter)
			haka.alert{
				description = string.format("unexpected server response"),
				severity = 'low'
			}
			return 'error'
		end,
	}

	data_transmission = state {
		enter = function (self)
			self.mail = haka.vbuffer_sub_stream()
		end,
		up = function (self, iter)
			local data, err = SmtpDissector.grammar.smtp_data:parse(iter, nil, self)
			if err then
				haka.alert{
					description = string.format("invalid data blob %s", err),
					severity = 'low'
				}
				return 'error'
			end
			local end_data = data.data:asstring() == '.\r\n'
			local mail_iter = nil
			if end_data then
				self.mail:finish()
			else
				mail_iter = self.mail:push(data.data)
			end
			self:trigger('mail_content', self.mail, mail_iter)
			self.mail:pop()
			if end_data then
				return 'response'
			end
		end,
		down = function (self, iter)
			haka.alert{
				description = string.format("unexpected server response"),
				severity = 'low'
			}
			return 'error'
		end,
		leave = function (self)
			self.mail = nil
		end,
	}

	default_transitions{
		error = function (self)
			self:drop()
		end,
		update = function (self, direction, iter)
			return self.state:transition(direction, iter)
		end
	}


	initial(session_initiation)
end)

module.events = SmtpDissector.events

return module

