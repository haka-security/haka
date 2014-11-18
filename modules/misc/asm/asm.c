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

static size_t update_pending_bytes(const char *buffer, size_t size, size_t offset, uint16_t current, void *user_data)
{
	struct asm_instruction_pending *pending = (struct asm_instruction_pending *)user_data;
	pending->skip = true;
	assert(current <= INSTRUCTION_MAX_LEN);

	if (pending->size == 0) {
		memcpy(pending->code, buffer, current);
		pending->size = current;
	}

	/* invalid instruction */
	if (current < size) {
		pending->status = 0;
	}
	/* broken instruction */
	else {
		pending->status = 1;
	}
	return 0;
}

struct asm_handle *asm_initialize(cs_arch arch, cs_mode mode)
{
	struct asm_handle *asm_handle = malloc(sizeof(struct asm_handle));

	if (!asm_handle) {
		error("memory error");
		return NULL;
	}

	asm_handle->pending.size = 0;
	asm_handle->pending.skip = false;
	asm_handle->pending.status = true;
	asm_handle->pending.advance = 0;

	asm_handle->skipdata.callback = (cs_skipdata_cb_t)update_pending_bytes;
	asm_handle->skipdata.user_data = &asm_handle->pending;

	if (cs_open(arch, mode, &asm_handle->handle) != CS_ERR_OK) {
		error("cannot initialize asm module");
		free(asm_handle);
		return NULL;
	}

	asm_handle->arch = arch;
	asm_handle->mode = mode;

	cs_option(asm_handle->handle, CS_OPT_SKIPDATA_SETUP, (size_t)&asm_handle->skipdata);
	cs_option(asm_handle->handle, CS_OPT_SKIPDATA, CS_OPT_ON);

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

void asm_set_mode(struct asm_handle *asm_handle, int mode) {
	cs_option(asm_handle->handle, CS_OPT_MODE, mode);
	asm_handle->mode = mode;
}

int asm_get_mode(struct asm_handle *asm_handle) {
	return asm_handle->mode;
}

int asm_get_arch(struct asm_handle *asm_handle) {

	return asm_handle->arch;
}

static int asm_disas(struct asm_handle *asm_handle, const uint8_t **code, size_t *size,
	struct asm_instruction *inst)
{
	csh *handle;
	cs_insn *instruction;
	cs_arch arch;
	cs_mode mode;
	int skip;
	bool ret = true;
	const uint8_t *ptr_code;
	struct asm_instruction_pending *pending;
	size_t tmp;
	enum instruction_status status;

	handle = &asm_handle->handle;
	pending = &asm_handle->pending;
	pending->skip = false;

	if (pending->size == 0) {
		ret = cs_disasm_iter(*handle, code, size, &inst->addr, inst->inst);
	}
	else {
		const uint16_t len = (*size <=  INSTRUCTION_MAX_LEN - pending->size)
			? *size :  INSTRUCTION_MAX_LEN - pending->size;
		memcpy(pending->code + pending->size, *code, len);
		pending->size += len;
		tmp = pending->size;
		ptr_code = pending->code;
		ret = cs_disasm_iter(*handle, &ptr_code, &tmp, &inst->addr, inst->inst);
		if (!pending->skip) {
			pending->size = 0;
		}
	}

	/* check disas result */
	if (!ret && pending->status) {
		update_pending_bytes((const char *)*code, *size, 0, *size , pending);
	}

	if (!pending->status) {
		mode = asm_handle->mode;
		arch = asm_handle->arch;
		if (arch == CS_ARCH_X86) skip = 1;
		else if (arch == CS_ARCH_ARM && mode == CS_MODE_THUMB) skip = 2;
		else skip = 4;

		inst->addr += skip;
		instruction = inst->inst;
		strcpy(instruction->mnemonic, "(bad)");
		memcpy(instruction->bytes, pending->code, skip);
		instruction->size = skip;
		instruction->op_str[0] = '\0';

		status = FAIL;
		pending->status = 1;
	}
	else {
		if (pending->skip) {
			status = NEEDMOREDATA;
		}
		else {
			status = SUCCESS;
		}
	}

	return status;
}

bool asm_vbdisas(struct asm_handle *asm_handle, struct vbuffer_iterator *pos, struct asm_instruction *inst)
{
	const uint8_t *code;
	size_t size, skip = 0, remaining, pending_skip;
	struct vbuffer_iterator iter;
	struct asm_instruction_pending *pending;
	enum instruction_status status = NEEDMOREDATA;

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
		code = vbuffer_iterator_mmap(pos, ALL, &size, 0);
		if (!code) {
			return false;
		}
		status = asm_disas(asm_handle, &code, &size, inst);
	}

	int diff = inst->inst->size - remaining;
	if (diff >= 0) {
		skip = diff;
		pending->size = 0;
	}
	else {
		skip = 0;
		memmove(pending->code, pending->code, -diff);
		pending->size = -diff;
	}

	vbuffer_iterator_copy(&iter, pos);
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
	cs_insn *instruction = inst->inst;
	cs_free(instruction, 1);
	free(inst);
}

uint32 instruction_get_id(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->id;
}

uintptr_t instruction_get_address(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->address;
}

uint16 instruction_get_size(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->size;
}

const uint8 *instruction_get_bytes(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->bytes;
}

const char *instruction_get_mnemonic(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->mnemonic;
}

const char *instruction_get_operands(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
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
