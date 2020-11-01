#include "asmln.h"
#include <memory.h>
#include <stdio.h> // TODO: Remove this, it's only for debugging/testing..

static int32_t asmln_strlen_(const char* str) {
	if (str == NULL) {
		return 0;
	}

	int32_t l = 0;

	while (str[l] != 0) {
		l++;
	}

	return l;
}

static const char* asmln_strndup_(const char* str, int maxlen) {
	char* result = calloc(maxlen + 1, 1);

	if (result != NULL && str != NULL) {
		int32_t i;
		for (i = 0; i < maxlen; i++) {
			if (str[i] == 0) {
				return (const char*)result;
			}
			result[i] = str[i];
		}
	}

	return (const char*)result;
}

static const char* asmln_strdup_(const char* str) {
	return asmln_strndup_(str, asmln_strlen_(str));
}

static const char* asmln_tokendup_(asmt_t* asmt) {
	int32_t t = asmt_tokentype(asmt);
	int32_t len = asmt_tokenlength(asmt);
	if (len <= 0) {
		return NULL;
	}
	if (t == ASMT_TOKENTYPE_STRING) {
		return asmln_strndup_(asmt->input + (asmt->index + 1), len - 2); // Skip quotes
	}
	if (t == ASMT_TOKENTYPE_LABEL) {
		len--;
	}
	//printf("Duplicating token type %d length %d\n", t, len);

	const char* result = asmln_strndup_(asmt->input + asmt->index, len);

	//printf("Got '%s'\n", result);

	return result;
}

static bool asmlnx_parse_subexpression(asmln_t* asmln, asmt_t* asmt, int32_t* type_var, const char** copy_var, asmlnx_t** x_var, int32_t* incr_var) {
	bool mayhavemoreparams = true;
	int32_t tt = asmt_tokentype(asmt);
	int32_t subc = 0;
	fprintf(stderr, "Got token type #%d\n", tt);
	switch (tt)
	{
	case ASMT_TOKENTYPE_OPENBR:
		x_var[0] = calloc(1, sizeof(asmlnx_t));
		if (x_var[0] == NULL) {
			fprintf(stderr, "MEMORY FAILURE\n");
			return false; // TODO: Better error handling here?
		}
		type_var[0] = tt;
		copy_var[0] = asmln_strdup_("(...)"); // TODO: Copy the whole expression source for debugging? (Probably not worthwhile.)
		incr_var[0]++;
		asmt_skiptoken(asmt);
		if (asmlnx_parse_subexpression(asmln, asmt, &(x_var[0]->lhstype), &(x_var[0]->lhscopy), &(x_var[0]->lhsx), &subc)) {
			fprintf(stderr, "BAD LHS\n");
			return false;
		}
		if (asmt_tokentype(asmt) != ASMT_TOKENTYPE_NAME) {
			fprintf(stderr, "NOT A NAME\n");
			return false;
		}
		x_var[0]->opcopy = asmln_tokendup_(asmt);
		fprintf(stderr, "GOT OPERATOR '%s'\n", x_var[0]->opcopy);
		asmt_skiptoken(asmt);
		if (asmlnx_parse_subexpression(asmln, asmt, &(x_var[0]->rhstype), &(x_var[0]->rhscopy), &(x_var[0]->rhsx), &subc)) {
			fprintf(stderr, "BAD RHS\n");
			return false;
		}
		if (asmt_tokentype(asmt) != ASMT_TOKENTYPE_CLOSEBR) {
			fprintf(stderr, "MISSING CLOSE\n");
			return false;
		}
		asmt_skiptoken(asmt);
		if (asmt_tokentype(asmt) == ASMT_TOKENTYPE_COMMA) {
			//printf("I got a comma\n");
			asmt_skiptoken(asmt);
		}
		else {
			mayhavemoreparams = false;
		}
		break;
	case ASMT_TOKENTYPE_NAME:
	case ASMT_TOKENTYPE_NUMBER:
	case ASMT_TOKENTYPE_STRING:
		type_var[0] = tt;
		copy_var[0] = asmln_tokendup_(asmt);
		x_var[0] = NULL;
		incr_var[0]++;
		asmt_skiptoken(asmt);
		if (asmt_tokentype(asmt) == ASMT_TOKENTYPE_COMMA) {
			//printf("I got a comma\n");
			asmt_skiptoken(asmt);
		}
		else {
			mayhavemoreparams = false;
		}
		break;
	default:
		mayhavemoreparams = false;
		break;
	}

	return mayhavemoreparams;
}

static void asmln_parse_inner(asmln_t* asmln, const char* sourceline) {
	if (asmln->errorcopy != NULL || asmln->instrcopy != NULL || asmln->labelcopy != NULL || asmln->commentcopy != NULL || asmln->nparams != 0) {
		asmln->errorcopy = asmln_strdup_("Assembler structure reused improperly");
		return;
	}
	if (sourceline == NULL) {
		asmln->errorcopy = asmln_strdup_("Source line is NULL");
		return;
	}
	asmt_t asmt;
	asmt.input = sourceline;
	asmt.length = asmln_strlen_(sourceline);
	asmt.index = 0;

	if (asmt_tokentype(&asmt) == ASMT_TOKENTYPE_LABEL) {
		asmln->labelcopy = asmln_tokendup_(&asmt);
		asmt_skiptoken(&asmt);
	}
	else {
		asmln->labelcopy = NULL;
	}
	asmln->nparams = 0;
	if (asmt_tokentype(&asmt) == ASMT_TOKENTYPE_NAME) {
		asmln->instrcopy = asmln_tokendup_(&asmt);
		asmt_skiptoken(&asmt);

		bool mayhavemoreparams = true;
		int32_t tt;
		while (mayhavemoreparams) {
			if (asmln->nparams >= ASMLN_MAXPARAMS) {
				asmln->errorcopy = asmln_strdup_("Too many parameters");
				return;
			}
			mayhavemoreparams = asmlnx_parse_subexpression(asmln, &asmt, &asmln->paramtype[asmln->nparams], &asmln->paramcopy[asmln->nparams], &asmln->paramx[asmln->nparams], &asmln->nparams);
			/*switch (tt = asmt_tokentype(&asmt))
			{
			case ASMT_TOKENTYPE_OPENBR:

			case ASMT_TOKENTYPE_NAME:
			case ASMT_TOKENTYPE_NUMBER:
			case ASMT_TOKENTYPE_STRING:
				if (asmln->nparams >= ASMLN_MAXPARAMS) {
					asmln->errorcopy = asmln_strdup_("Too many parameters");
					return;
				}
				asmln->paramtype[asmln->nparams] = tt;
				asmln->paramcopy[asmln->nparams] = asmln_tokendup_(&asmt);
				asmln->nparams++;
				asmt_skiptoken(&asmt);
				if (asmt_tokentype(&asmt) == ASMT_TOKENTYPE_COMMA) {
					//printf("I got a comma\n");
					asmt_skiptoken(&asmt);
				} else {
					mayhavemoreparams = false;
				}
				break;
			default:
				mayhavemoreparams = false;
				break;
			}*/
		}
	}
	else {
		asmln->instrcopy = NULL;
	}
	if (asmt_tokentype(&asmt) == ASMT_TOKENTYPE_COMMENT) {
		asmln->commentcopy = asmln_tokendup_(&asmt);
		asmt_skiptoken(&asmt);
	}
	else {
		asmln->commentcopy = NULL;
	}
	if (asmt_tokentype(&asmt) != ASMT_TOKENTYPE_END) {
		asmln->errorcopy = asmln_strdup_("Unexpected token (the source doesn't seem to be a line of valid assembler)");
		return;
	}
}

asmln_t* asmln_new(const char* sourceline) {
	asmln_t* result = calloc(1, sizeof(asmln_t));

	if (result != NULL) {
		asmln_parse_inner(result, sourceline);
	}

	return result;
}

void* asmlnx_delete(asmlnx_t* asmlnx) {
	if (asmlnx->lhscopy != NULL) {
		free(asmlnx->lhscopy);
	}
	if (asmlnx->lhsx != NULL) {
		asmlnx_delete(asmlnx->lhsx);
	}

	if (asmlnx->opcopy != NULL) {
		free(asmlnx->opcopy);
	}

	if (asmlnx->rhscopy != NULL) {
		free(asmlnx->rhscopy);
	}
	if (asmlnx->rhsx != NULL) {
		asmlnx_delete(asmlnx->rhsx);
	}

	free(asmlnx);

	return NULL;
}

void* asmln_delete(asmln_t* asmln) {
	if (asmln != NULL) {
		if (asmln->labelcopy != NULL) {
			free(asmln->labelcopy);
			asmln->labelcopy = NULL;
		}
		if (asmln->instrcopy != NULL) {
			free(asmln->instrcopy);
			asmln->instrcopy = NULL;
		}

		if (asmln->errorcopy != NULL) {
			free(asmln->errorcopy);
			asmln->errorcopy = NULL;
		}
		if (asmln->commentcopy != NULL) {
			free(asmln->commentcopy);
			asmln->commentcopy = NULL;
		}
		int32_t i;
		for (i = 0; i < asmln->nparams; i++) {
			asmln->paramtype[i] = ASMT_TOKENTYPE_END;
			if (asmln->paramcopy[i] != NULL) {
				free(asmln->paramcopy[i]);
				asmln->paramcopy[i] = NULL;
			}
			if (asmln->paramx[i] != NULL) {
				asmlnx_delete(asmln->paramx[i]);
				asmln->paramx[i] = NULL;
			}
		}
		asmln->nparams = 0;

		free(asmln);
	}
	return NULL;
}