#include "asmgeneric_old.h"
#include <stdlib.h>

const char* geninstrs[] = {
	ASMGEN_INSTRNAMES
};
const char* genregs[] = {
	ASMGEN_REGNAMES
};

int32_t asmgen_regnum(const char* name) {
	if (name == NULL) {
		return -1;
	}
	int32_t i = 0;
	while (genregs[i] != NULL) {
		if (strcmp(name, genregs[i]) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}
bool asmgen_asmln_param(asmdata_t* asmdata, int8_t type, const char* value) {
	asmln_t tmpln;
	tmpln.instrcopy = "data64";
	tmpln.nparams = 1;
	tmpln.commentcopy = tmpln.labelcopy = tmpln.errorcopy = NULL;
	void* buffer = NULL;

	if (type == ASMT_TOKENTYPE_NAME) {
		int32_t reg = asmgen_regnum(value);
		if (reg >= 0) {
			type = ASMT_TOKENTYPE_NUMBER;
			buffer = calloc(100, 1);
			if (buffer == NULL) {
				return false;
			}
			value = _ltoa(reg, buffer, 10);
		}
	}

	tmpln.paramtype[0] = type;
	tmpln.paramcopy[0] = value;

	bool result = asmdata_asmln(asmdata, &tmpln);
	if (buffer != NULL) {
		free(buffer);
	}
	return result;
}
bool asmgen_asmln_internal(asmdata_t* asmdata, asmln_t* asmln, int32_t opbase) {
	if (asmln->nparams < 0 || asmln->nparams > 3) {
		return false; // Bad parameter count
	}
	int32_t op = (opbase << 4) | 1 | (asmln->nparams << 2);
	if (((int32_t)((uint16_t)op)) != op) {
		return false; // Op doesn't fit in 16 bits
	}
	// TODO: Alignment?
	if (!asmdata_appendword(asmdata, op, ASMDATA_SIZE_16BIT)) {
		return false; // Failed to write instruction
	}
	int32_t i;
	for (i = 0; i < asmln->nparams; i++) {
		if (!asmgen_asmln_param(asmdata, asmln->paramtype[i], asmln->paramcopy[i])) {
			return false; // Failed to write operand
		}
	}
	return true;
}
bool asmgeneric_asmln(asmdata_t* asmdata, asmln_t* asmln) {
	if (asmln->errorcopy != NULL || asmln->instrcopy == NULL) {
		return false;
	}

	const char* n = asmln->instrcopy;
	int32_t i = 0;
	while (geninstrs[i] != NULL) {
		if (strcmp(n, geninstrs[i]) == 0) {
			return asmgen_asmln_internal(asmdata, asmln, i);
		}
		i++;
	}
	return false;
}