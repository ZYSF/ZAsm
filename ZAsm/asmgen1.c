#include "asmgen1.h"
#include <string.h>
#include <stdlib.h>

//#include <stdio.h>	 
#define strdup _strdup
#define ltoa _ltoa

static const char* gen1instrs[] = {
	ASMGEN1_INSTR_INFO
};
static const char* gen1regs[] = {
	ASMGEN1_REGISTER_INFO
};
static const char* gen1ctrls[] = {
	ASMGEN1_CTRL_INFO
};
static const char* gen1excns[] = {
	ASMGEN1_EXCN_INFO
};

int32_t asmgen1_regnum(const char* name) {
	if (name == NULL) {
		return -1;
	}
	int32_t i = 0;
	while (gen1regs[i * 2] != NULL) {
		if (strcmp(name, gen1regs[i * 2]) == 0) {
			return i;
		}
		if (strcmp(name, gen1regs[i * 2 + 1]) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

int32_t asmgen1_constnum(const char* name) {
	if (name == NULL) {
		return -1;
	}
	int32_t i = 0;
	while (gen1ctrls[i * 2] != NULL) {
		if (strcmp(name, gen1ctrls[i * 2]) == 0) {
			return i;
		}
		if (strcmp(name, gen1ctrls[i * 2 + 1]) == 0) {
			return i;
		}
		i++;
	}
	i = 0;
	while (gen1excns[i * 2] != NULL) {
		if (strcmp(name, gen1excns[i * 2]) == 0) {
			return i;
		}
		if (strcmp(name, gen1excns[i * 2 + 1]) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}
bool asmgen1_asmln_param(asmdata_t* asmdata, int8_t type, const char* value) {
	asmln_t tmpln;
	tmpln.instrcopy = "data64";
	tmpln.nparams = 1;
	tmpln.commentcopy = tmpln.labelcopy = tmpln.errorcopy = NULL;
	void* buffer = NULL;

	if (type == ASMT_TOKENTYPE_NAME) {
		int32_t reg = asmgen1_regnum(value);
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

bool asmgen1_asmln_internal_xxx(asmdata_t* asmdata, asmln_t* asmln, int32_t instrbase) {
	/* For instructions with no defined parameters, we don't accept any. You can still
	 * construct these instructions manually with your own "data32" line, but the assembler
	 * won't assume a format and will just expect e.g. "syscall" instructions to not use
	 * the unspecified bits. (This will probably change as such conventions are decided.)
	 */
	if (asmln->nparams != 0) {
		return false;
	}

	/* So for these instructions we really just have to pass in a "data32" line with out instruction base. */
	asmln_t tmpln;
	tmpln.instrcopy = "data32";
	tmpln.nparams = 1;
	tmpln.commentcopy = tmpln.labelcopy = tmpln.errorcopy = NULL;
	void* buffer = NULL;

	tmpln.paramtype[0] = ASMT_TOKENTYPE_NUMBER;
	buffer = calloc(100, 1);
	if (buffer == NULL) {
		return false;
	}
	tmpln.paramcopy[0] = _ltoa(instrbase, buffer, 10);

	/* Pass the line through to the backend. */
	bool result = asmdata_asmln(asmdata, &tmpln);

	if (buffer != NULL) {
		free(buffer);
	}

	return result;
}

asmlnx_t* asmgen1_copyx(asmlnx_t* x) {
	if (x == NULL) {
		return NULL;
	}
	asmlnx_t* result = calloc(sizeof(asmlnx_t), 1);
	if (result == NULL) {
		return NULL;
	}

	result->lhstype = x->lhstype;
	result->lhscopy = strdup(x->lhscopy);
	result->lhsx = asmgen1_copyx(x->lhsx);
	result->opcopy = strdup(x->opcopy);
	result->rhstype = x->rhstype;
	result->rhscopy = strdup(x->rhscopy);
	result->rhsx = asmgen1_copyx(x->rhsx);

	return result;
}

/* This will produce an asmlnx structure (which represents an operation) based on an existing left-hand-side, operator and right-hand-side. Only the operator
 * is copied.
 */
asmlnx_t* asmgen1_copyopx(int32_t lhstype, const char* lhsstr, asmlnx_t* lhsx, const char* opconstant, int32_t rhstype, const char* rhsstr, asmlnx_t* rhsx) {
	asmlnx_t* result = calloc(sizeof(asmlnx_t), 1);
	if (result == NULL) {
		return NULL;
	}

	result->lhstype = lhstype;
	result->lhscopy = lhsstr;
	result->lhsx = lhsx;
	result->opcopy = strdup(opconstant);
	result->rhstype = rhstype;
	result->rhscopy = rhsstr;
	result->rhsx = rhsx;

	return result;
}
/*
asmlnx_t* asmgen1_orx(int32_t lhstype, const char* lhsstr, asmlnx_t* lhsx, int32_t rhstype, const char* rhsstr, asmlnx_t* rhsx) {
	return asmgen1_copyopx(lhstype, lhsstr, lhsx, "|", rhstype, rhsstr, rhsx);
}
*/

asmlnx_t* asmgen1_innerparam(int32_t lhstype, const char* lhsstr, asmlnx_t* lhsx, int32_t rhstype, const char* rhsstr, asmlnx_t* rhsx, bool negshift, const char* shiftstr, const char* maskstr) {
	asmlnx_t* innerMask = asmgen1_copyopx(rhstype, rhsstr, rhsx, "&", ASMT_TOKENTYPE_NUMBER, maskstr, NULL);
	if (innerMask == NULL) {
		return NULL;
	}
	asmlnx_t* innerShift = asmgen1_copyopx(ASMT_TOKENTYPE_OPENBR, strdup("(...)"), innerMask, negshift ? ">>" : "<<", ASMT_TOKENTYPE_NUMBER, shiftstr, NULL);
	
	return asmgen1_copyopx(lhstype, lhsstr, lhsx, "|", ASMT_TOKENTYPE_OPENBR, strdup("(...)"), innerShift);
}

bool asmgen1_xparam(asmln_t* asmln, int32_t type, const char* str, asmlnx_t* x, bool expectRegister, uint32_t shift, uint32_t mask) {
	void* shiftbuffer = calloc(100, 1);
	if (shiftbuffer == NULL) {
		return false;
	}
	bool negshift = shift < 0;
	const char* shiftstr = ltoa(negshift ? 0 - shift : shift, shiftbuffer, 10);
	void* maskbuffer = calloc(100, 1);
	if (maskbuffer == NULL) {
		return false;
	}
	const char* maskstr = ltoa(mask, maskbuffer, 10);

	if (expectRegister) {
		//printf("Expecting register for '%s'\n", str);
		if (type != ASMT_TOKENTYPE_NAME) {
			return false;
		}
		int n = asmgen1_regnum(str);
		if (n < 0) {
			//printf("Bad register\n");
			return false;
		}
		void* regbuffer = calloc(100, 1);
		if (regbuffer == NULL) {
			//printf("Internal error\n");
			return false;
		}
		const char* regstr = ltoa(n, regbuffer, 10);
		//printf("Building inner...\n");
		asmln->paramx[0] = asmgen1_innerparam(asmln->paramtype[0], asmln->paramcopy[0], asmln->paramx[0], ASMT_TOKENTYPE_NUMBER, regstr, NULL, negshift, shiftstr, maskstr);
		asmln->paramcopy[0] = strdup("(...)");
		asmln->paramtype[0] = ASMT_TOKENTYPE_OPENBR;
		if (asmln->paramx[0] != NULL) {
			return true;
		}
	} else {
		//printf("Building inner (non-register)...\n");
		asmln->paramx[0] = asmgen1_innerparam(asmln->paramtype[0], asmln->paramcopy[0], asmln->paramx[0], type, strdup(str), asmgen1_copyx(x), negshift, shiftstr, maskstr);
		asmln->paramcopy[0] = strdup("(...)");
		asmln->paramtype[0] = ASMT_TOKENTYPE_OPENBR;
		if (asmln->paramx[0] != NULL) {
			return true;
		}
	}
}

bool asmgen1_asmln_internal_abc(asmdata_t* asmdata, asmln_t* asmln, int32_t instrbase, bool hasA, bool hasB, bool hasC) {
	/* Check number of parameters first, otherwise trying to decode them will probably cause problems. */
	int nparams = (hasA ? 1 : 0) + (hasB ? 1 : 0) + (hasC ? 1 : 0);
	if (asmln->nparams != nparams) {
		//printf("Bad params to ABC\n");
		return false;
	}

	/* Begin constructing an asmdata instruction. This instruction will be a "data32" instruction with a
	 * single parameter (an expression constructing the instruction).
	 */
	asmln_t tmpln;
	tmpln.instrcopy = "data32";
	tmpln.nparams = 1;
	tmpln.commentcopy = tmpln.labelcopy = tmpln.errorcopy = NULL;
	void* buffer = NULL;

	/* Begin by setting the type to number*/
	tmpln.paramtype[0] = ASMT_TOKENTYPE_NUMBER;
	buffer = calloc(100, 1);
	if (buffer == NULL) {
		return false;
	}
	tmpln.paramcopy[0] = _ltoa(instrbase, buffer, 10);

	//printf("Doing ABC...\n");

	int pnum = 0;

	if (hasA) {
		if (!asmgen1_xparam(&tmpln, asmln->paramtype[pnum], asmln->paramcopy[pnum], asmln->paramx[pnum], true, 16, 0xFF)) {
			//printf("Failed at A\n");
			free(buffer);
			return false;
		}
		pnum++;
	}

	if (hasB) {
		if (!asmgen1_xparam(&tmpln, asmln->paramtype[pnum], asmln->paramcopy[pnum], asmln->paramx[pnum], true, 8, 0xFF)) {
			//printf("Failed at B\n");
			free(buffer);
			return false;
		}
		pnum++;
	}

	if (hasC) {
		if (!asmgen1_xparam(&tmpln, asmln->paramtype[pnum], asmln->paramcopy[pnum], asmln->paramx[pnum], true, 0, 0xFF)) {
			//printf("Failed at C\n");
			free(buffer);
			return false;
		}
		pnum++; // We're at the end anyway, but just for good luck...
	}

	bool result = asmdata_asmln(asmdata, &tmpln);

	if (buffer != NULL) {
		free(buffer);
	}

	return result;
}

bool asmgen1_asmln_internal_abi(asmdata_t* asmdata, asmln_t* asmln, int32_t instrbase) {
	/* Check number of parameters first, otherwise trying to decode them will probably cause problems. */
	if (asmln->nparams != 3) {
		//printf("Bad params to ABI\n");
		return false;
	}

	/* Begin constructing an asmdata instruction. This instruction will be a "data32" instruction with a
	 * single parameter (an expression constructing the instruction).
	 */
	asmln_t tmpln;
	tmpln.instrcopy = "data32";
	tmpln.nparams = 1;
	tmpln.commentcopy = tmpln.labelcopy = tmpln.errorcopy = NULL;
	void* buffer = NULL;

	/* Begin by setting the type to number*/
	tmpln.paramtype[0] = ASMT_TOKENTYPE_NUMBER;
	buffer = calloc(100, 1);
	if (buffer == NULL) {
		return false;
	}
	tmpln.paramcopy[0] = _ltoa(instrbase, buffer, 10);

	//printf("Doing ABI...\n");

	if (!asmgen1_xparam(&tmpln, asmln->paramtype[0], asmln->paramcopy[0], asmln->paramx[0], true, 20, 0xF)) {
		//printf("Failed at A\n");
		free(buffer);
		return false;
	}

	if (!asmgen1_xparam(&tmpln, asmln->paramtype[1], asmln->paramcopy[1], asmln->paramx[1], true, 16, 0xF)) {
		//printf("Failed at B\n");
		free(buffer);
		return false;
	}

	if (!asmgen1_xparam(&tmpln, asmln->paramtype[2], asmln->paramcopy[2], asmln->paramx[2], false, 0, 0xFFFF)) {
		//printf("Failed at I\n");
		free(buffer);
		return false;
	}

	bool result = asmdata_asmln(asmdata, &tmpln);

	if (buffer != NULL) {
		free(buffer);
	}

	return result;
}

bool asmgen1_asmln_internal_bci(asmdata_t* asmdata, asmln_t* asmln, int32_t instrbase) {
	return asmgen1_asmln_internal_abi(asmdata, asmln, instrbase); // ABI and BCI are formatted the same (the registers just have different semantic meanings)
}

/* "bca" encoding is just the specialised form of abi/bci encoding, where the parameter (a code address) is encoded shifted two bits to the right. */
bool asmgen1_asmln_internal_bca(asmdata_t* asmdata, asmln_t* asmln, int32_t instrbase) {
	/* Check number of parameters first, otherwise trying to decode them will probably cause problems. */
	if (asmln->nparams != 3) {
		//printf("Bad params to ABI\n");
		return false;
	}

	/* Begin constructing an asmdata instruction. This instruction will be a "data32" instruction with a
	 * single parameter (an expression constructing the instruction).
	 */
	asmln_t tmpln;
	tmpln.instrcopy = "data32";
	tmpln.nparams = 1;
	tmpln.commentcopy = tmpln.labelcopy = tmpln.errorcopy = NULL;
	void* buffer = NULL;

	/* Begin by setting the type to number*/
	tmpln.paramtype[0] = ASMT_TOKENTYPE_NUMBER;
	buffer = calloc(100, 1);
	if (buffer == NULL) {
		return false;
	}
	tmpln.paramcopy[0] = _ltoa(instrbase, buffer, 10);

	//printf("Doing ABI...\n");

	if (!asmgen1_xparam(&tmpln, asmln->paramtype[0], asmln->paramcopy[0], asmln->paramx[0], true, 20, 0xF)) {
		//printf("Failed at A\n");
		free(buffer);
		return false;
	}

	if (!asmgen1_xparam(&tmpln, asmln->paramtype[1], asmln->paramcopy[1], asmln->paramx[1], true, 16, 0xF)) {
		//printf("Failed at B\n");
		free(buffer);
		return false;
	}

	/* Encoding the address parameter just uses a negative shift value (which gets translated into a right shift in case the linker is fussy). */
	if (!asmgen1_xparam(&tmpln, asmln->paramtype[2], asmln->paramcopy[2], asmln->paramx[2], false, -2, 0xFFFF)) {
		//printf("Failed at I\n");
		free(buffer);
		return false;
	}

	bool result = asmdata_asmln(asmdata, &tmpln);

	if (buffer != NULL) {
		free(buffer);
	}

	return result;
}

bool asmgen1_asmln_internal(asmdata_t* asmdata, asmln_t* asmln, int32_t opbase) {
	const char* codestr = gen1instrs[opbase * 3];
	uint32_t code = strtol(codestr, NULL, 16) << 24;
	const char* encstr = gen1instrs[opbase * 3 + 1];

	//printf("GOT CODESTR '%s' (0x%x) ENCSTR '%s'...\n", codestr, code, encstr);

	if (strcmp(encstr, "abi") == 0) {
		return asmgen1_asmln_internal_abi(asmdata, asmln, code);
	} else if (strcmp(encstr, "bci") == 0) {
		return asmgen1_asmln_internal_bci(asmdata, asmln, code);
	} else if (strcmp(encstr, "bca") == 0) {
		return asmgen1_asmln_internal_bca(asmdata, asmln, code);
	}
	else if (strcmp(encstr, "abc") == 0) {
		return asmgen1_asmln_internal_abc(asmdata, asmln, code, true, true, true);
	}
	else if (strcmp(encstr, "axx") == 0) {
		return asmgen1_asmln_internal_abc(asmdata, asmln, code, true, false, false);
	}
	else if (strcmp(encstr, "xxc") == 0) {
		return asmgen1_asmln_internal_abc(asmdata, asmln, code, false, false, true);
	}
	else if (strcmp(encstr, "axc") == 0) {
		return asmgen1_asmln_internal_abc(asmdata, asmln, code, true, false, true);
	}
	else if(strcmp(encstr, "xxx") == 0) {
		// NOTE: This could really just be replaced with _abc() with each parameter option as false
		return asmgen1_asmln_internal_xxx(asmdata, asmln, code);
	} else {
		//printf("NO MATCH?\n");
		return false;
	}
}
bool asmgen1_asmln(asmdata_t* asmdata, asmln_t* asmln) {
	//printf("A at '%s'\n", asmln->instrcopy);
	if (asmln->errorcopy != NULL || asmln->instrcopy == NULL) {
		return false;
	}
	//printf("B\n");

	const char* n = asmln->instrcopy;
	int32_t i = 0;
	while (gen1instrs[i*3] != NULL) {
		//printf("Checking '%s'...\n", gen1instrs[i * 3 + 2]);
		if (strcmp(n, gen1instrs[i*3 + 2]) == 0) {
			//printf("C at %d\n", i);
			bool x = asmgen1_asmln_internal(asmdata, asmln, i);
			if (!x) {
				//printf("Failed internally\n");
			}
			return x;
		}
		i++;
	}
	//printf("D\n");
	return false;
}