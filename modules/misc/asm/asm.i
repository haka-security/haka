/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module asm

%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%{
#include "asm.h"

struct asm_instruction *new_instruction(unsigned long addr) {
    struct asm_instruction *instruction = malloc(sizeof(struct asm_instruction));
    if (!instruction) {
        error("memory error");
        return NULL;
    }
    memset(instruction, 0, sizeof(struct asm_instruction));
    instruction->inst.detail = &instruction->detail;
    instruction->addr = addr;
    return instruction;
}
%}

%nodefaultctor;
%nodefaultdtor;

struct asm_instruction {
    %extend {
        ~asm_instruction() {
            if ($self) {
                instruction_release($self);
            }
        }

        void bytes(char **TEMP_OUTPUT, size_t *TEMP_SIZE)
        {
            unsigned short size = instruction_get_size($self);
            if (size == 0) {
                return ;
            }
            char *buffer = malloc(sizeof(char) * size);
            if (!buffer) {
                error("memory error");
                return;
            }
            memcpy(buffer, instruction_get_bytes($self), size);
            *TEMP_OUTPUT = buffer;
            *TEMP_SIZE = size;
        }

        void op_str(const char **TEMP_OUTPUT)
        {
            char *buffer = malloc(sizeof(char) * INSTRUCTION_OPERANDS);
            if (!buffer) {
                error("memory error");
                return;
            }
            snprintf(buffer, 160, "%s", instruction_get_operands($self));
            *TEMP_OUTPUT = buffer;
        }

        void mnemonic(const char **TEMP_OUTPUT)
        {
            char *buffer = malloc(sizeof(char) * INSTRUCTION_MNEMONIC);
            if (!buffer) {
                error("memory error");
                return;
            }
            snprintf(buffer, 32, "%s", instruction_get_mnemonic($self));
            *TEMP_OUTPUT = buffer;
        }

        %immutable;
        unsigned int id;
        unsigned long address;
        unsigned short size;
    }
};

struct asm_handle {
    %extend {
        ~asm_handle() {
            asm_destroy($self);
        }

        void _setsyntax(int syntax) {
            asm_set_disassembly_flavor($self, syntax);
        }

        bool _disas(struct vbuffer_iterator *pos, struct asm_instruction
*inst) {
            return asm_vbdisas($self, pos, inst);
        }
    }
};

%{
unsigned int asm_instruction_id_get(struct asm_instruction *inst)
{
    return instruction_get_id(inst);
}

unsigned long asm_instruction_address_get(struct asm_instruction *inst)
{
    return instruction_get_address(inst);
}

unsigned short asm_instruction_size_get(struct asm_instruction *inst)
{
    return instruction_get_size(inst);
}
%}

%newobject asm_initialize;
struct asm_handle *asm_initialize(int arch, int mode);

%newobject new_instruction;
struct asm_instruction *new_instruction(unsigned long addr = 0);

enum {
    CS_ARCH_ARM,
    CS_ARCH_ARM64,
    CS_ARCH_MIPS,
    CS_ARCH_X86,
    CS_ARCH_PPC,
    CS_ARCH_SPARC,
    CS_ARCH_SYSZ,
    CS_ARCH_XCORE,
};

enum {
    CS_MODE_ARM,
    CS_MODE_16,
    CS_MODE_32,
    CS_MODE_64,
    CS_MODE_THUMB,
};

enum {
    CS_OPT_SYNTAX_INTEL,
    CS_OPT_SYNTAX_ATT,
};

%luacode {
    local this = unpack({...})

    local asm_arch = {
        ["arm"] = this.CS_ARCH_ARM,
        ["arm64"] = this.CS_ARCH_ARM64,
        ["mips"] = this.CS_ARCH_MIPS,
        ["x86"] = this.CS_ARCH_X86,
        ["ppc"] = this.CS_ARCH_PPC,
        ["sparc"] = this.CS_ARCH_SPARC,
        ["sysz"] = this.CS_ARCH_SYSZ,
        ["xcore"] = this.CS_ARCH_XCORE,
    }

    local asm_mode = {
        ["arm"] = this.CS_MODE_ARM,
        ["16"] = this.CS_MODE_16,
        ["32"] = this.CS_MODE_32,
        ["64"] = this.CS_MODE_64,
        ["thumb"] = this.CS_MODE_THUMB,
    }

    local asm_syntax = {
        ["intel"] = this.CS_OPT_SYNTAX_INTEL,
        ["att"] = this.CS_OPT_SYNTAX_ATT,
    }

    local asm_initialize = this.asm_initialize
    this.asm_initialize = nil

    function this.new_disassembler(str_arch, str_mode)
        local cs_arch arch = asm_arch[str_arch]
        if not arch then
            error("unsupported hardware architecture")
            return nil
        end
        local cs_mode mode = asm_mode[str_mode]
        if not mode then
            error("unsupported hardware mode")
            return nil
        end
        return asm_initialize(arch, mode)
    end

    swig.getclassmetatable('asm_handle')['.fn'].setsyntax = function (self, str_syntax)
        local cs_mode syntax = asm_syntax[str_syntax]
        if not syntax then
            error("unknown syntax")
        end
        return self:_setsyntax(syntax)
    end

    local iter = swig.getclassmetatable('vbuffer_iterator')
    local iter_block = swig.getclassmetatable('vbuffer_iterator_blocking')

    local function disas(handle, pos, inst)
        if not inst then
            error("invalid instruction")
            return false
        end
        local meta = getmetatable(pos)
        if meta == iter then
            return handle:_disas(pos, inst)
        elseif meta == iter_block then
            local iter = pos._iter
            repeat
                if handle:_disas(iter, inst) then
                    return true
                end
            until not pos:wait()
        else
            error("invalid iterator")
        end
        return false
    end

    swig.getclassmetatable('asm_handle')['.fn'].disassemble = function (self, pos,
inst)
        return disas(self, pos, inst)
    end

    swig.getclassmetatable('asm_handle')['.fn'].dump_instructions = function (self, pos)
        local inst = this.new_instruction()
        while disas(self, pos, inst) do
            io.write(string.format("\t0x%08x %-8s %-32s ",
                inst.address, inst:mnemonic(), inst:op_str()))
            bytes = inst:bytes()
            size = inst.size
            for i = 1, size do
                io.write(string.format('%02x ', bytes:byte(i)))
            end
            print("")
        end
    end
}
