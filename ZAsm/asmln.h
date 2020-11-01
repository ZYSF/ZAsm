#ifndef ASMLN_H
#define ASMLN_H
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* This can be defined explicitly to change how the functions are defined (may be needed on some compilers or targets). */
#ifndef ASMLN_INLINE
#define ASMLN_INLINE static inline
#endif

/* Character functions: */

ASMLN_INLINE bool asmc_isalpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

ASMLN_INLINE int32_t asmc_digitval(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	else if (c >= 'a' && c <= 'f') {
		return c - 'a';
	}
	else if (c >= 'A' && c <= 'F') {
		return c - 'A';
	}
	else {
		return -1;
	}
}

ASMLN_INLINE bool asmc_isdigit(char c, int base) {
	int v = asmc_digitval(c);
	if (v < 0) {
		return false;
	}
	else if (v < base) {
		return true;
	}
	else {
		return false;
	}
}

ASMLN_INLINE bool asmc_isdec(char c) {
	return asmc_isdigit(c, 10);
}

ASMLN_INLINE bool asmc_ishex(char c) {
	return asmc_isdigit(c, 16);
}

ASMLN_INLINE bool asmc_isvalidlabelhead(char c) {
	return asmc_isalpha(c) || c == '_' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '|' || c == '&' || c == '$' || c == '.';
}

ASMLN_INLINE bool asmc_isvalidlabeltail(char c) {
	return asmc_isvalidlabelhead(c) || asmc_isdec(c);
}

ASMLN_INLINE bool asmc_isspace(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* Tokeniser functions, these should be useful whether the structure is set up for a line of input or for a whole input file at a time. */

typedef struct asmt asmt_t;

struct asmt {
	const char* input;
	int32_t length;
	int32_t index;
};

ASMLN_INLINE bool asmt_isend(asmt_t* asmt) {
	return asmt->index < 0 || asmt->index >= asmt->length;
}

ASMLN_INLINE void asmt_skipspaces(asmt_t* asmt) {
	while (!asmt_isend(asmt) && asmc_isspace(asmt->input[asmt->index])) {
		asmt->index++;
	}
}

ASMLN_INLINE bool asmt_isnameorlabelstart(asmt_t* asmt) {
	return !asmt_isend(asmt) && asmc_isvalidlabelhead(asmt->input[asmt->index]);
}

ASMLN_INLINE bool asmt_isnumberstart(asmt_t* asmt) {
	return !asmt_isend(asmt) && (asmc_isdec(asmt->input[asmt->index]) || asmt->input[asmt->index] == '-' || asmt->input[asmt->index] == '+');
}

ASMLN_INLINE bool asmt_isstringstart(asmt_t* asmt) {
	return !asmt_isend(asmt) && asmt->input[asmt->index] == '\"';
}

ASMLN_INLINE bool asmt_iscommentstart(asmt_t* asmt) {
	return !asmt_isend(asmt) && asmt->input[asmt->index] == ';';
}

ASMLN_INLINE int32_t asmt_numberlength(asmt_t* asmt) {
	if (asmt_isnumberstart(asmt)) {
		int32_t len = 1;
		while (!asmt_isend(asmt) && asmt->index + len < asmt->length && asmc_isdec(asmt->input[asmt->index + len])) {
			len++;
		}
		return len;
	}
	else {
		return -1;
	}
}

ASMLN_INLINE int32_t asmt_namelength(asmt_t* asmt) {
	if (asmt_isnameorlabelstart(asmt)) {
		int32_t len = 1;
		while (!asmt_isend(asmt) && asmt->index + len < asmt->length && asmc_isvalidlabeltail(asmt->input[asmt->index + len])) {
			len++;
		}
		return len;
	}
	else {
		return -1;
	}
}

ASMLN_INLINE int32_t asmt_stringlength(asmt_t* asmt) {
	if (asmt_isstringstart(asmt)) {
		int32_t len = 1;
		while (!asmt_isend(asmt) && asmt->index + len < asmt->length && asmt->input[asmt->index + len] != '\"') {
			len++;
		}
		if (asmt_isend(asmt) || asmt->index + len >= asmt->length) {
			return -1;
		}
		return len + 1;
	}
	else {
		return -1;
	}
}

ASMLN_INLINE int32_t asmt_commentlength(asmt_t* asmt) {
	if (asmt_iscommentstart(asmt)) {
		int32_t len = 1;
		while (!asmt_isend(asmt) && asmt->index + len < asmt->length && asmt->input[asmt->index + len] != '\r' && asmt->input[asmt->index + len] != '\n') {
			len++;
		}
		return len;
	}
	else {
		return -1;
	}
}

ASMLN_INLINE bool asmt_islabelstart(asmt_t* asmt) {
	int32_t namelen = asmt_namelength(asmt);
	if (namelen > 0 && asmt->index + namelen < asmt->length && asmt->input[asmt->index + namelen] == ':') {
		return true;
	}
	else {
		return false;
	}
}

ASMLN_INLINE bool asmt_isnamestart(asmt_t* asmt) {
	return asmt_isnameorlabelstart(asmt) && !asmt_islabelstart(asmt);
}

#define ASMT_TOKENTYPE_ERROR	-1
#define ASMT_TOKENTYPE_END		0
#define ASMT_TOKENTYPE_LABEL	1
#define ASMT_TOKENTYPE_NAME		2
#define ASMT_TOKENTYPE_NUMBER	3
#define ASMT_TOKENTYPE_STRING	4
#define ASMT_TOKENTYPE_COMMA	5
#define ASMT_TOKENTYPE_COMMENT	6
#define ASMT_TOKENTYPE_OPENBR	7
#define ASMT_TOKENTYPE_CLOSEBR	8

ASMLN_INLINE int32_t asmt_tokentype(asmt_t* asmt) {
	asmt_skipspaces(asmt);
	if (asmt_isend(asmt)) {
		return ASMT_TOKENTYPE_END;
	}
	else if (asmt_isnamestart(asmt)) {
		return ASMT_TOKENTYPE_NAME;
	}
	else if (asmt_islabelstart(asmt)) {
		return ASMT_TOKENTYPE_LABEL;
	}
	else if (asmt_isnumberstart(asmt)) {
		return ASMT_TOKENTYPE_NUMBER;
	}
	else if (asmt_isstringstart(asmt)) {
		return ASMT_TOKENTYPE_STRING;
	}
	else if (asmt_iscommentstart(asmt)) {
		return ASMT_TOKENTYPE_COMMENT;
	}
	else if (asmt->input[asmt->index] == ',') {
		return ASMT_TOKENTYPE_COMMA;
	}
	else if (asmt->input[asmt->index] == '(') {
		return ASMT_TOKENTYPE_OPENBR;
	}
	else if (asmt->input[asmt->index] == ')') {
		return ASMT_TOKENTYPE_CLOSEBR;
	}
	else {
		return ASMT_TOKENTYPE_ERROR;
	}
}

ASMLN_INLINE int32_t asmt_tokenlength(asmt_t* asmt) {
	switch (asmt_tokentype(asmt)) {
	case ASMT_TOKENTYPE_NAME:
		return asmt_namelength(asmt);
	case ASMT_TOKENTYPE_LABEL:
		return asmt_namelength(asmt) + 1;
	case ASMT_TOKENTYPE_NUMBER:
		return asmt_numberlength(asmt);
	case ASMT_TOKENTYPE_STRING:
		return asmt_stringlength(asmt);
	case ASMT_TOKENTYPE_COMMENT:
		return asmt_commentlength(asmt);
	case ASMT_TOKENTYPE_COMMA:
	case ASMT_TOKENTYPE_OPENBR:
	case ASMT_TOKENTYPE_CLOSEBR:
		return 1;
	case ASMT_TOKENTYPE_ERROR:
	case ASMT_TOKENTYPE_END:
	default:
		return -1;
	}
}

ASMLN_INLINE bool asmt_skiptoken(asmt_t* asmt) {
	asmt_skipspaces(asmt); // Just in case of edge cases...
	if (asmt_isend(asmt)) {
		return false;
	}
	else {
		int32_t l = asmt_tokenlength(asmt);
		if (l <= 0) {
			return false;
		}
		else {
			asmt->index += l;
			return true;
		}
	}
}

ASMLN_INLINE void asmt_skipcomments(asmt_t* asmt) {
	while (asmt_iscommentstart(asmt)) {
		asmt_skiptoken(asmt);
	}
}



/* Main API: Deals with one line of assembly code at a time, in format like [labelname:] [instrname param1 , param2 , ... , paramN] [; comment] */

/* Set a reasonably high maximum, assuming data strings can go on for a while... */
#define ASMLN_MAXPARAMS		1000

typedef struct asmln asmln_t;
typedef struct asmlnx asmlnx_t;

struct asmln {
	const char* labelcopy;
	const char* instrcopy;
	int32_t nparams;
	int32_t paramtype[ASMLN_MAXPARAMS];
	const char* paramcopy[ASMLN_MAXPARAMS];
	asmlnx_t* paramx[ASMLN_MAXPARAMS];
	const char* commentcopy;
	const char* errorcopy;
};

/* Subexpressions, e.g. (1 + (2 * 3)) need to be split into a tree structure. Expressions are given the type ASMT_TOKENTYPE_OPENBR (like their first token).
 * These expressions could be optimised/reduced by the assembler in the future, but otherwise are encoded directly in the output, allowing the linker/loader to
 * resolve complex expressions involving linker symbols.
 */
struct asmlnx {
	int32_t lhstype;
	const char* lhscopy;
	asmlnx_t* lhsx;
	const char* opcopy;
	int32_t rhstype;
	const char* rhscopy;
	asmlnx_t* rhsx;
};

asmln_t* asmln_new(const char* sourceline);

void* asmln_delete(asmln_t* asmln);

/*
ASMLN_INLINE int32_t asmln_nextnonspace(const char* l, int32_t n, int32_t i) {
	while (i < n) {
		if (!asmc_isspace(l[i])) {
			return i;
		}
		i++;
	}
	return -1;
}

ASMLN_INLINE int32_t asmln_nextinstance(const char* l, int32_t n, int32_t i, char c) {
	while (i < n) {
		if (l[i] == c) {
			return i;
		}
		i++;
	}
	return -1;
}*/

//ASMLN_INLINE int32_t asmln_lab

/* Convenience functions: */
/*
ASMLN_INLINE int32_t asml_len(const char* l) {
	if (l == NULL) {
		return 0;
	}
	int32_t i = 0;
	while (l[i] != 0) {
		i++;
	}
	return i;
}*/

/* From ifndef at top of file: */
#endif