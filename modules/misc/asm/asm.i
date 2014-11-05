/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module asm

%include "haka/lua/swig.si"
%include "haka/lua/object.si"

%{
#include "haka/asm.h"
%}

%nodefaultctor;
%nodefaultdtor;

struct asm_instruction {
    %extend {
        ~asm_instruction() {
            if ($self)
                instruction_release($self);
        }
        %immutable;
        unsigned int id;
        unsigned long address;
        unsigned short size;
        char *bytes;
        char *mnemonic;
        char *op_str;
    }
};

struct asm_handle {
    %extend {
        ~asm_handle() {
            asm_destroy($self);
        }

        void setmode(int mode) {
            asm_set_mode($self, mode);
        }

        void setsyntax(int syntax) {
            asm_set_disassembly_flavor($self, syntax);
        }

        struct asm_instruction *new_inst(unsigned long addr = 0) {
            struct asm_instruction *instruction = malloc(sizeof(struct asm_instruction));
            if (!instruction) {
                error("memory error");
                return NULL;
            }
            instruction->inst = NULL;
            instruction->inst = cs_malloc(*(csh *)$self->handle);
            instruction->addr = addr;
            return instruction;
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

char *asm_instruction_bytes_get(struct asm_instruction *inst)
{
    char *buffer = malloc(sizeof(char) * INSTRUCTION_BYTES);
    if (!buffer) {
        error("memory error");
        return NULL;
    }
    snprintf(buffer, 16, "%s", instruction_get_bytes(inst));
    return buffer;
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

/* Architecture */
%constant int ARCH_PPC = CS_ARCH_PPC;
%constant int ARCH_X86 = CS_ARCH_X86;
%constant int ARCH_ARM = CS_ARCH_ARM;
%constant int ARCH_ARM64 = CS_ARCH_ARM64;
%constant int ARCH_MIPS = CS_ARCH_MIPS;
/* Mode */
%constant int MODE_16 = CS_MODE_16;
%constant int MODE_32 = CS_MODE_32;
%constant int MODE_64 = CS_MODE_64;
%constant int MODE_ARM = CS_MODE_ARM;
%constant int MODE_THUMB = CS_MODE_THUMB;
%constant int MODE_LITTLE_ENDIAN = CS_MODE_LITTLE_ENDIAN;
%constant int MODE_BIG_ENDIAN = CS_MODE_BIG_ENDIAN;
%constant int MODE_MICRO = CS_MODE_MICRO;
/* Syntax */
%constant int ATT = CS_OPT_SYNTAX_ATT;
%constant int INTEL = CS_OPT_SYNTAX_INTEL;

%rename(init) asm_initialize;
%newobject asm_initialize;
struct asm_handle *asm_initialize(int arch, int mode);

%luacode {
    local iterator_type = swig.getclassmetatable('vbuffer_iterator_blocking')

    swig.getclassmetatable('asm_handle')['.fn'].disas = function (self, pos,
inst)
        local meta = getmetatable(pos)
        local block_iter = pos
        if meta == iterator_type then
            block_iter = pos._iter
        end
        repeat
            if self:_disas(block_iter, inst) then
                return true
            end
        until not pos:wait() or block_iter:available() == 0
        return false
    end
}
