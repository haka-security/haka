/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>

#include <haka/module.h>
#include <haka/error.h>
#include <haka/log.h>

#include "asm.h"

enum instruction_status {
	FAIL,
	SUCCESS,
	NEEDMOREDATA,
};

struct asm_handle *asm_initialize(cs_arch arch, cs_mode mode)
{
	struct asm_handle *asm_handle = malloc(sizeof(struct asm_handle));

	if (!asm_handle) {
		error("memory error");
		return NULL;
	}

	asm_handle->pending.size = 0;
	asm_handle->pending.advance = 0;

	if (cs_open(arch, mode, &asm_handle->handle) != CS_ERR_OK) {
		error("cannot initialize asm module");
		free(asm_handle);
		return NULL;
	}

	asm_handle->arch = arch;
	asm_handle->mode = mode;

	cs_option(asm_handle->handle, CS_OPT_DETAIL, CS_OPT_ON);

	return asm_handle;
}

void asm_destroy(struct asm_handle *asm_handle) {
	if (asm_handle) {
		cs_close(&asm_handle->handle);
		free(asm_handle);
	}
}

void asm_set_disassembly_flavor(struct asm_handle *asm_handle, int syntax)
{
	cs_option(asm_handle->handle, CS_OPT_SYNTAX, syntax);
}

int asm_get_mode(struct asm_handle *asm_handle) {
	return asm_handle->mode;
}

int asm_get_arch(struct asm_handle *asm_handle) {

	return asm_handle->arch;
}

uint8 instruction_get_max_length(cs_arch arch, cs_mode mode)
{
	if (arch == CS_ARCH_X86) return 15;
	else if (arch == CS_ARCH_ARM && mode == CS_MODE_THUMB) return 2;
	else return 4;
}

static int asm_disas(struct asm_handle *asm_handle, const uint8_t **code, size_t *size,
	struct asm_instruction *inst, bool eos)
{
	csh *handle;
	cs_insn *instruction;
	cs_arch arch;
	cs_mode mode;
	bool ret = true;
	const uint8_t *ptr_code;
	struct asm_instruction_pending *pending;
	size_t length;

	handle = &asm_handle->handle;
	pending = &asm_handle->pending;

	if (pending->size == 0) {
		ptr_code = *code;
		length = *size;
		ret = cs_disasm_iter(*handle, code, size, &inst->addr, &inst->inst);
	}
	else {
		size_t tmp;
		const uint16_t len = (*size <=  INSTRUCTION_MAX_LEN - pending->size)
			? *size :  INSTRUCTION_MAX_LEN - pending->size;
		memcpy(pending->code + pending->size, *code, len);
		pending->size += len;
		tmp = pending->size;
		ptr_code = pending->code;
		length = tmp;
		ret = cs_disasm_iter(*handle, &ptr_code, &tmp, &inst->addr, &inst->inst);
	}

	/* check disas result */
	if (!ret) {
		mode = asm_handle->mode;
		arch = asm_handle->arch;

		/* broken instruction */
		if (length < instruction_get_max_length(arch, mode) && !eos) {
			if (pending->size == 0) {
				memcpy(pending->code, *code, length);
				pending->size = length;
			}
			return NEEDMOREDATA;
		}

		/* invalid instruction */
		else {
			int skip;
			if (arch == CS_ARCH_X86) skip = 1;
			else if (arch == CS_ARCH_ARM && mode == CS_MODE_THUMB) skip = 2;
			else skip = 4;

			if (eos && length < skip) {
				skip = length;
			}

			inst->addr += skip;
			instruction = &inst->inst;
			strcpy(instruction->mnemonic, "(bad)");
			memcpy(instruction->bytes, ptr_code, skip);
			instruction->size = skip;
			instruction->op_str[0] = '\0';
			return FAIL;
		}
	}
	return SUCCESS;
}

bool asm_vbdisas(struct asm_handle *asm_handle, struct vbuffer_iterator *pos, struct asm_instruction *inst)
{
	const uint8_t *code;
	size_t size, skip = 0, remaining, pending_skip;
	struct vbuffer_iterator iter;
	struct asm_instruction_pending *pending;
	enum instruction_status status = NEEDMOREDATA;
	bool eos;

	if (!vbuffer_iterator_isvalid(pos)) {
		error("invalid vbuffer iterator");
		return false;
	}

	pending = &asm_handle->pending;
	remaining = pending->size;

	if (pending->advance) {
		pending_skip = vbuffer_iterator_advance(pos, pending->advance);
		if (pending_skip != pending->advance) {
			pending->advance -= pending_skip;
			return false;
		}
		pending->advance = 0;
	}

	vbuffer_iterator_copy(pos, &iter);

	while (status == NEEDMOREDATA) {
		code = vbuffer_iterator_mmap(&iter, ALL, &size, 0);
		if (!code) {
			vbuffer_iterator_move(pos, &iter);
			return false;
		}
		eos =  vbuffer_iterator_iseof(&iter);
		status = asm_disas(asm_handle, &code, &size, inst, eos);
	}

	int diff = inst->inst.size - remaining;
	if (diff >= 0) {
		skip = diff;
		pending->size = 0;
	}
	else {
		skip = 0;
		memmove(pending->code, pending->code, -diff);
		pending->size = -diff;
	}

	pending_skip = vbuffer_iterator_advance(pos, skip);
	if (pending_skip != skip) {
		pending->advance = skip - pending_skip;
	}
	else {
		pending->advance = 0;
	}
	return true;
}

void instruction_release(struct asm_instruction *inst)
{
	if (inst) {
		free(inst);
	}
}

uint32 instruction_get_id(struct asm_instruction *inst)
{
	cs_insn instruction = inst->inst;
	return instruction.id;
}

uintptr_t instruction_get_address(struct asm_instruction *inst)
{
	cs_insn instruction = inst->inst;
	return instruction.address;
}

uint16 instruction_get_size(struct asm_instruction *inst)
{
	cs_insn instruction = inst->inst;
	return instruction.size;
}

const uint8 *instruction_get_bytes(struct asm_instruction *inst)
{
	cs_insn *instruction = &inst->inst;
	return instruction->bytes;
}

const char *instruction_get_mnemonic(struct asm_instruction *inst)
{
	cs_insn *instruction = &inst->inst;
	return instruction->mnemonic;
}

const char *instruction_get_operands(struct asm_instruction *inst)
{
	cs_insn *instruction = &inst->inst;
	return instruction->op_str;
}

static int init(struct parameters *args)
{
	return 0;
}

static void cleanup()
{
}

struct module ASM_MODULE = {
	type:        MODULE_EXTENSION,
	name:        "Disassembler",
	description: "Instruction disassembler",
	api_version: HAKA_API_VERSION,
	init:        init,
	cleanup:     cleanup
};
