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

        %immutable;
        unsigned int id;
        unsigned long address;
        unsigned short size;
        char *mnemonic;
        char *op_str;
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

char *asm_instruction_mnemonic_get(struct asm_instruction *inst)
{
    char *buffer = malloc(sizeof(char) * INSTRUCTION_MNEMONIC);
    if (!buffer) {
        error("memory error");
        return NULL;
    }
    snprintf(buffer, 32, "%s", instruction_get_mnemonic(inst));
    return buffer;
}

char *asm_instruction_op_str_get(struct asm_instruction *inst)
{
    char *buffer = malloc(sizeof(char) * INSTRUCTION_OPERANDS);
    if (!buffer) {
        error("memory error");
        return NULL;
    }
    snprintf(buffer, 160, "%s", instruction_get_operands(inst));
    return buffer;
}
%}

%newobject asm_initialize;
struct asm_handle *asm_initialize(int arch, int mode);

%newobject new_instruction;
struct asm_instruction *new_instruction(unsigned long addr = 0);

%luacode {
    local asm_arch = {
        ["arm"] = 0,
        ["arm64"] = 1,
        ["mips"] = 2,
        ["x86"] = 3,
        ["ppc"] = 4,
    }

    local asm_mode = {
        ["arm"] = 0,
        ["16"] = 2,
        ["32"] = 4,
        ["64"] = 8,
        ["thumb"] = 16,
    }

    local asm_syntax = {
        ["intel"] = 1, 
        ["att"] = 2,
    }

    local this = unpack({...})
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

    swig.getclassmetatable('asm_handle')['.fn'].disassemble = function (self, pos,
inst)
        if not inst then
            error("invalid instruction parameter")
            return false
        end
        local meta = getmetatable(pos)
        if meta == iter then
            return self:_disas(pos, inst)
        elseif meta == iter_block then
            local iter = pos._iter
            repeat
                if self:_disas(iter, inst) then
                    return true
                end
            until not pos:wait() or iter:available() == 0
        else
            error("invalid parameters")
        end
        return false
    end
}
