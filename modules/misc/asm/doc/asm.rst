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

.. haka:function:: init(architecture, mode) -> asm_handle

    :param architecture: Hardware architecture.
    :ptype architecture: int
    :param mode: Hardware mode.
    :ptype mode: int
    :return asm_handle: Disassembler handler.
    :rtype asm_handle: AsmHandle

    Initialize the disassembler module with selected architecture and mode.

Supported architecture
~~~~~~~~~~~~~~~~~~~~~~

    .. haka:attribute:: AsmInstruction:AsmHandle:ARCH_PPC
    .. haka:attribute:: AsmInstruction:AsmHandle:ARCH_X86
    .. haka:attribute:: AsmInstruction:AsmHandle:ARCH_ARM
    .. haka:attribute:: AsmInstruction:AsmHandle:ARCH_ARM64
    .. haka:attribute:: AsmInstruction:AsmHandle:ARCH_MIPS

Supported mode
~~~~~~~~~~~~~~

    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_16
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_32
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_64
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_ARM
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_THUMB
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_LITTLE_ENDIAN
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_BIG_ENDIAN
    .. haka:attribute:: AsmInstruction:AsmHandle:MODE_MICRO

Syntax flavor
~~~~~~~~~~~~~

    .. haka:attribute:: AsmInstruction:AsmHandle:ATT
    .. haka:attribute:: AsmInstruction:AsmHandle:INTEL

.. haka:class:: AsmHandle

    .. haka:function:: AsmHandle:setsyntax(syntax)

        :param syntax: Syntax flavor.
        :ptype syntax: int

        Set the assembly syntax.

    .. haka:function:: AsmHandle:setmode(mode)

        :param mode: Hardware mode.
        :ptype mode: int.

        Set the hardware mode.

    .. haka:function:: AsmHandle:new_inst(address = 0) -> asm_inst

        :param address: Instruction address.
        :ptype address: int
        :return asm_inst: Instruction.
        :rtype asm_inst: AsmInstruction

        Create a new instruction.

    .. haka:function:: AsmHandle:disas(code, inst) -> ret

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

    .. haka:attribute:: AsmInstruction:mnemonic

        :type: string

        Instruction mnemonic.

    .. haka:attribute:: AsmInstruction:op_str

        :type: string

        Instruction operands.

    .. haka:attribute:: AsmInstruction:size

        :type: number

        Instruction size.

    .. haka:attribute:: AsmInstruction:bytes

        :type: string

        Instruction byte sequence.


Example
-------

::

    local asm_module = require('misc/asm')

    asm = asm_module.init(asm_module.ARCH_X86, asm_module.MODE_32)
    asm:setsyntax(asm_module.ATT)

    local inst = asm:new_inst()
    local code = haka.vbuffer_from("\x41\x42\x48\x8b\x05\xb8\x13\x60\x60"):pos('begin')

    while asm:disas(code, inst) do
        io.write(string.format("0x%08x %-8s %-16s ", inst.address, inst.mnemonic, inst.op_str))
        for i = 1,inst.size do
            io.write(string.format('%02X ', inst.bytes:byte(i)))
        end
        print("")
    end

