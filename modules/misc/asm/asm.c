/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>

#include <haka/module.h>
#include <haka/error.h>
#include <haka/log.h>

#include "haka/asm.h"

enum instruction_status {
	FAIL,
	SUCCESS,
	NEEDMOREDATA,
};

static size_t asm_callback(const char *buffer, size_t size, size_t offset, uint16_t current, void *user_data)
{
	struct asm_instruction_pending *pending = (struct asm_instruction_pending *)user_data;
	uint16_t length = pending->size;
	pending->skip = true;
	assert(current <= INSTRUCTION_MAX_LEN);
	/* invalid instruction */
	if (current < size) {
		pending->size = INSTRUCTION_MAX_LEN + 1;
	}
	/* broken instruction */
	else {
		if (pending->size == 0) {
			memcpy(pending->code + length, buffer + length, current - length);
			pending->size += current;
		}
	}
	return 0;
}

struct asm_handle *asm_initialize(int arch, int mode)
{
	csh *handle;
	struct asm_handle *asm_handle = malloc(sizeof(struct asm_handle));

	if (!asm_handle) {
		error("memory error");
		return NULL;
	}

	handle = malloc(sizeof(csh));
	if (!handle) {
		error("memory error");
		return NULL;
	}
	asm_handle->handle = handle;

	asm_handle->pending = malloc(sizeof(struct asm_instruction_pending));
	if (!asm_handle->pending) {
		error("memory error");
		return NULL;
	}
	asm_handle->pending->code = malloc(sizeof(15));
	asm_handle->pending->size = 0;
	asm_handle->pending->skip = false;

	asm_handle->skipdata = malloc(sizeof(cs_opt_skipdata));
	if (!asm_handle->skipdata) {
		error("memory error");
		return NULL;
	}
	asm_handle->skipdata->callback = (cs_skipdata_cb_t)asm_callback;
	asm_handle->skipdata->user_data = asm_handle->pending;

	if (cs_open(arch, mode, handle) != CS_ERR_OK) {
		error("cannot initialize asm module");
		return NULL;
	}

	asm_handle->arch = arch;
	asm_handle->mode = mode;

    cs_option(*handle, CS_OPT_SKIPDATA_SETUP, (size_t)asm_handle->skipdata);
    cs_option(*handle, CS_OPT_SKIPDATA, CS_OPT_ON);

	cs_option(*handle, CS_OPT_DETAIL, CS_OPT_ON);

	return asm_handle;
}

void asm_destroy(struct asm_handle *asm_handle) {
	if (asm_handle) {
		free(asm_handle->skipdata);
		cs_close((csh *)asm_handle->handle);
	}
}

void asm_set_disassembly_flavor(struct asm_handle *asm_handle, int syntax)
{
	csh *handle = asm_handle->handle;
	cs_option(*handle, CS_OPT_SYNTAX, syntax);
}

void asm_set_mode(struct asm_handle *asm_handle, int mode) {
	csh *handle = asm_handle->handle;
	cs_option(*handle, CS_OPT_MODE, mode);
	asm_handle->mode = mode;
}

int asm_get_mode(struct asm_handle *asm_handle) {
	return asm_handle->mode;
}

int asm_get_arch(struct asm_handle *asm_handle) {
	return asm_handle->arch;
}

int asm_disas(struct asm_handle *asm_handle, const uint8_t **code, size_t *size,
	struct asm_instruction *inst)
{
	csh *handle;
	struct asm_instruction_pending *pending;
	size_t len, tmp;
	enum instruction_status status;

	handle = asm_handle->handle;
	pending = asm_handle->pending;
	pending->skip = false;

	if (pending->size > 0) {
		len = (*size <=  INSTRUCTION_MAX_LEN - pending->size) ? *size :  INSTRUCTION_MAX_LEN - pending->size;
		memcpy(pending->code + pending->size, *code, len);
		pending->size += len;
		tmp = pending->size;
		cs_disasm_iter(*handle, (const uint8_t **)&pending->code, &tmp, &inst->addr, inst->inst);

		if (!pending->skip) {
			memset(pending->code, 0, pending->size);
			pending->size = 0;
		}
	}

	else {
		cs_disasm_iter(*handle, code, size, &inst->addr, inst->inst);
	}

	/* check disas result */
	if (pending->size ==  INSTRUCTION_MAX_LEN + 1) {
        strcpy(inst->inst->mnemonic, "(bad)");
		status = FAIL;
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
	size_t size, skip = 0;
	int arch, mode;
	struct vbuffer_iterator iter;
	enum instruction_status status = NEEDMOREDATA;

	if (!vbuffer_iterator_isvalid(pos)) {
		error("invalid vbuffer iterator");
		return false;
	}
	vbuffer_iterator_copy(pos, &iter);

	while (status == NEEDMOREDATA) {
		code = vbuffer_iterator_mmap(pos, ALL, &size, 0);
		if (!code) {
			vbuffer_iterator_clear(&iter);
			return false;
		}
		status = asm_disas(asm_handle, &code, &size, inst);
	}

	if (status == SUCCESS) {
		skip = inst->inst->size;
	}
	else {
		mode = asm_handle->mode;
		arch = asm_handle->arch;
		if (arch == CS_ARCH_X86) skip = 1;
		else if (arch == CS_ARCH_ARM && mode == CS_MODE_THUMB) skip = 2;
		else skip = 4;
		inst->addr += skip;
	}
	vbuffer_iterator_copy(&iter, pos);
	vbuffer_iterator_advance(pos, skip);
	vbuffer_iterator_clear(&iter);
	return true;
}

void instruction_release(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	cs_free(instruction, 1);
	free(inst);
}

unsigned int instruction_get_id(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->id;
}

unsigned long instruction_get_address(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->address;
}

unsigned short instruction_get_size(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->size;
}

char *instruction_get_bytes(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return (char *)instruction->bytes;
}

char *instruction_get_mnemonic(struct asm_instruction *inst)
{
	cs_insn *instruction = inst->inst;
	return instruction->mnemonic;
}

char *instruction_get_operands(struct asm_instruction *inst)
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
