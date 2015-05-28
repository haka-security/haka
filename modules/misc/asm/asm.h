/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _ASM_H_
#define _ASM_H_

#include <haka/types.h>
#include <haka/vbuffer_stream.h>
#include <capstone.h>

#define INSTRUCTION_BYTES    16
#define INSTRUCTION_OPERANDS 160
#define INSTRUCTION_MNEMONIC 32

/* The maximum size of an instruction depends
 * on the architecture. Instruction size does
 * not exceed 15 (on x86).
 */
#define INSTRUCTION_MAX_LEN  15

struct asm_instruction_pending {
	uint8_t                        code[INSTRUCTION_MAX_LEN];
	uint16_t                       size;
	size_t                         advance;
};

struct asm_handle {
	csh                            handle;
	int                            arch;
	int                            mode;
	struct asm_instruction_pending pending;
};

struct asm_instruction {
	uint64_t   addr;
	cs_insn    inst;
	cs_detail  detail;
};

void               instruction_release(struct asm_instruction *inst);
uint32             instruction_get_id(struct asm_instruction *inst);
uintptr_t          instruction_get_address(struct asm_instruction *inst);
uint16             instruction_get_size(struct asm_instruction *inst);
const uint8       *instruction_get_bytes(struct asm_instruction *inst);
const char        *instruction_get_mnemonic(struct asm_instruction *inst);
const char        *instruction_get_operands(struct asm_instruction *inst);
void               instruction_print(struct asm_instruction *inst);

struct asm_handle *asm_initialize(cs_arch arch, cs_mode mode);
void               asm_destroy(struct asm_handle *asm_handle);
void               asm_set_disassembly_flavor(struct asm_handle *asm_handle, int syntax);
int                asm_get_arch(struct asm_handle *asm_handle);
int                asm_get_mode(struct asm_handle *asm_handle);
bool               asm_vbdisas(struct asm_handle *asm_handle, struct vbuffer_iterator *pos,
                       struct asm_instruction *_inst);

#endif
