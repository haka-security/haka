
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestAsmModule = {}

function TestAsmModule:setUp()
	self.module = require("misc/asm")
	local arch = self.module.ARCH_X86
	local mode = self.module.MODE_32

	self.asm = self.module.init(arch, mode)
	self.inst = self.asm:new_inst()
end

function TestAsmModule:gen_stream(f)
	local code = { "\x88", "\x05", "\x41\x41\x41\x41" }
	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new(stream)
	manager:start(0, f)

	for i, d in ipairs(code) do
		local current = stream:push(haka.vbuffer_from(d))
		if i == #code then
			stream:finish()
		end

		manager:process_all(current)

		while stream:pop() do end
	end
end

function TestAsmModule:test_disas_should_succeed_when_valid_single_byte_inst ()
	-- When
	local code = haka.vbuffer_from('\x90'):pos('begin')
	local ret = self.asm:disas(code, self.inst)
	-- Then
	assertTrue(ret)
	assertEquals(self.inst.mnemonic, "nop")
	assertEquals(self.inst.size, 1)

end

function TestAsmModule:test_disas_should_succeed_when_valid_multiple_byte_inst ()
	-- When
	local code = haka.vbuffer_from('\x8b\x05\xb8\x13\x60\x60'):pos('begin')
	local ret = self.asm:disas(code, self.inst)
	-- Then
	assertTrue(ret)
	assertEquals(self.inst.mnemonic, "mov")
	assertEquals(self.inst.op_str, "eax, dword ptr [0x606013b8]")
	assertEquals(self.inst.size, 6)
end

function TestAsmModule:test_disas_should_succeed_when_inst_overlaps_on_multiple_chunks ()
	-- When
	local code = haka.vbuffer_from("\x41\x41\x48\x8b\x05\xb8\x13\x60\x60\x88")
	code:append(haka.vbuffer_from("\x05"))
	code:append(haka.vbuffer_from("\x90\x41\x42\x43"))

	local start = code:pos('begin')

	local count = 0

	while self.asm:disas(start, self.inst) do
		count = count + 1
	end
	-- Then
	assertEquals(count, 5)
	assertEquals(self.inst.mnemonic, "mov")
	assertEquals(self.inst.op_str, "byte ptr [0x43424190], al")
	assertEquals(self.inst.size, 6)
end

function TestAsmModule:test_disas_should_skip_bad_inst ()
	-- When
	local code = haka.vbuffer_from('\x0f\x0a\x90'):pos('begin')
	self.asm:disas(code, self.inst)
	-- Then
	assertEquals(self.inst.mnemonic, "(bad)")
	-- And when
	self.asm:disas(code, self.inst)
	-- Then
	assertEquals(self.inst.mnemonic, "add")
	assertEquals(self.inst.op_str, "byte ptr [eax], al")
	assertEquals(self.inst.size, 2)
end

function TestAsmModule:test_can_disas_on_blocking_iterator ()
	self:gen_stream(function (iter)
		-- When
		local ret = self.asm:disas(iter, self.inst)
		-- Then
		assertTrue(ret)
	end)
end

addTestSuite('TestAsmModule')

