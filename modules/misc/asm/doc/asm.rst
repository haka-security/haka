.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Disassembler
============

.. haka:module:: asm

Stream disassembler module.

**Usage:**

::

    local asm = require('misc/asm')

API
---

Disassembler syntax
-------------------

.. haka:function:: new_disassembler(architecture, mode) -> asm_handle

    :param architecture: Hardware architecture.
    :ptype architecture: string
    :param mode: Hardware mode.
    :ptype mode: string
    :return asm_handle: Disassembler handler.
    :rtype asm_handle: AsmHandle

    Initialize the disassembler module with selected architecture and mode.

.. haka:function:: AsmHandle:new_instruction(address = 0) -> asm_inst

    :param address: Instruction address.
    :ptype address: int
    :return asm_inst: Instruction.
    :rtype asm_inst: AsmInstruction

    Create a new instruction.



Supported architecture
~~~~~~~~~~~~~~~~~~~~~~

    * arm
    * arm64
    * mips
    * x86
    * ppc

Supported mode
~~~~~~~~~~~~~~

    * arm
    * 16
    * 32
    * 64
    * thumb

.. haka:class:: AsmHandle

    .. haka:function:: AsmHandle:setsyntax(syntax)

        :param syntax: Syntax flavor.
        :ptype syntax: string

        Set the assembly syntax ('att' or 'intel').

    .. haka:function:: AsmHandle:disassemble(code, inst) -> ret

        :param code: Code to disassemble.
        :ptype code: :haka:class:`vbuffer_iterator`
        :param inst: Instruction.
        :ptype inst: AsmInstruction.
        :return ret: true in case of succesfful dissassembly; false otherwise (e.g. broken instruction).
        :rtype ret: bool

        .. note:: Disassembly skips bad instructions. Whenever a bad instruction is encountered, the mnemonic instruction field is set to ``(bad)``. Disasembly stops when it reaches the end of the stream or when it encounters a broken instruction.

.. haka:class:: AsmInstruction

    .. haka:attribute:: AsmInstruction:id

        :type: number

        Instruction id.

    .. haka:attribute:: AsmInstruction:address

        :type: number

        Instruction Address.

    .. haka:function:: AsmInstruction:mnemonic() -> mnemonic

        :return mnemonic: Instruction mnemonic.
        :rtype mnemonic: string

        .. note:: The mnemonic is set to ``(bad)`` when the disassembler encounters an invalid opcode.

    .. haka:function:: AsmInstruction:op_str() -> operands

        :return operands: Instruction operands.
        :rtype operands: string

    .. haka:attribute:: AsmInstruction:size

        :type: number

        Instruction size.

    .. haka:function:: AsmInstruction:bytes() -> bytes

        :return bytes: Instruction byte sequence.
        :rtype bytes: string

Example
-------

::

    local asm_module = require('misc/asm')

    inst = asm_module.new_instruction()
    asm = asm_module.new_disassembler('x86', '32')
    asm:setsyntax('att')

    local code = haka.vbuffer_from("\x41\x42\x48\x8b\x05\xb8\x13\x60\x60")
    local start = code:pos('begin')

    local size, bytes
    while asm:disassemble(start, inst) do
        io.write(string.format("0x%08x %-8s %-16s ", inst.address, inst:mnemonic(), inst:op_str()))
        size = inst.size
        bytes = inst:bytes()
        for i = 1,inst.size do
            io.write(string.format('%02X ', bytes:byte(i)))
        end
        print("")
    end
