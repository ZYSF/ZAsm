#include "asmdata.h"
#include <stdlib.h>
//#include <stdio.h> //Just for debuging. TODO: Remove.

static int32_t asmdata_strlen_(const char* str) {
	if (str == NULL) {
		return 0;
	}

	int32_t l = 0;

	while (str[l] != 0) {
		l++;
	}

	return l;
}

static const char* asmdata_strndup_(const char* str, int maxlen) {
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

static const char* asmdata_strdup_(const char* str) {
	return asmdata_strndup_(str, asmdata_strlen_(str));
}

static bool asmdata_streq_(const char* a, const char* b) {
	if (a == b) {
		return true;
	}

	int32_t la = asmdata_strlen_(a);
	int32_t lb = asmdata_strlen_(b);

	if (la != lb) {
		return false;
	}

	int32_t i;
	for (i = 0; i < la; i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}

	return true;
}


asmdata_section_t* asmdata_section_new(const char* name) {
	asmdata_section_t* result = calloc(1, sizeof(asmdata_section_t));

	if (result != NULL) {
		result->namecopy = asmdata_strdup_(name);
	}

	return result;
}
bool asmdata_section_reserveextra(asmdata_section_t* section, int64_t nbytes, bool willfill) {
	int32_t granularity = 1024; // Resize the buffer about kilobyte or so at a time to prevent unnecessary realloc calls
	/* Firstly, the reserved total needs to be incremented whether filled or not. */
	section->reservedsize += nbytes;

	/* If we're not filling it, we can just leave it on the reserved total (not necessarily any need to allocate/use extra memory!). */
	if (!willfill) {
		return true;
	}

	/* Make sure it fits in 32 bits (so we don't have to worry whether size_t is 32- or 64- bits) and is below the maximum. */
	if (((int64_t)((int32_t)section->reservedsize)) != section->reservedsize || section->reservedsize > ASMDATA_MAXFILLED) {
		return false;
	}

	if (section->reservedsize > section->buffersize) {
		section->buffersize = (int32_t)section->reservedsize; // Already checked that it can fit in 32 bits if filled
		while (section->buffersize % granularity != 0) {
			section->buffersize++;
		}
		if (section->buffer == NULL) {
			section->buffer = calloc(1, section->buffersize);
		}
		else {
			section->buffer = realloc(section->buffer, section->buffersize);
		}
		if (section->buffer == NULL) {
			return false;
		}
	}

	section->bufferfilled = (int32_t)section->reservedsize;
	return true;
}

void* asmdata_section_delete(asmdata_section_t* section) {
	if (section != NULL) {
		if (section->namecopy != NULL) {
			free(section->namecopy);
			section->namecopy = NULL;
		}
		if (section->buffer != NULL) {
			free(section->buffer);
			section->buffer = NULL;
			section->bigendian = false;
			section->bufferfilled = 0;
			section->buffersize = 0;
			section->reservedsize = 0;
			section->virtualoffset = 0;
		}

		free(section);
	}
	return NULL;
}

asmdata_t* asmdata_new() {
	asmdata_t* result = calloc(1, sizeof(asmdata_t));

	if (result != NULL) {

	}

	return result;
}
void* asmdata_delete(asmdata_t* asmdata) {
	if (asmdata != NULL) {
		asmdata->activesection = NULL;
		int32_t i;
		for (i = 0; i < asmdata->nsections; i++) {
			asmdata->sections[i] = asmdata_section_delete(asmdata->sections[i]);
		}
		asmdata->nsections = 0;
		free(asmdata);
	}

	return NULL;
}

bool asmdata_finalise(asmdata_t* asmdata) {
	/* Finalisation will fail if there's any hint that the code has already been finalised (even if it was done manually by defining symbol
	 * table sections in assembly code).
	 */
	if (asmdata->finalised || asmdata_hassection(asmdata, "asmdata.strings") || asmdata_hassection(asmdata, "asmdata.symbols") || asmdata_hassection(asmdata, "asmdata.references")) {
		return false;
	}

	/* The three linkage sections are created in "optimised" order: You generally need to have read the symbols section to make sense of the
	 * references section, so symbols are written before references. Strings (and maybe later extra debug info) are written last because they
	 * need not actually be read in many cases.
	 */
	if (asmdata_selectsection(asmdata, "asmdata.symbols") == NULL) {
		return false;
	}
	if (asmdata_selectsection(asmdata, "asmdata.references") == NULL) {
		return false;
	}
	if (asmdata_selectsection(asmdata, "asmdata.strings") == NULL) {
		return false;
	}

	/* The first string will be at offset 0 (which might also imply NULL), so we can start by adding a NULL string there. An interpreter
	 * should generally handle NULL strings differently, but in case any are automatically read the result will be a marker: */
	asmdata_selectsection(asmdata, "asmdata.strings");
	const char* nullstr = "<ASMDATA-NULL-STRING>";
	asmdata_appendbytes(asmdata, (uint8_t*)nullstr, asmdata_strlen_(nullstr) + 1); // Be sure to include the terminating zero byte!

	int32_t i;
	for (i = 0; i < asmdata->nsymbols; i++) {
		asmdata_section_t* str = asmdata_selectsection(asmdata, "asmdata.strings");
		int64_t stroffset = str->reservedsize;
		int32_t strlen = asmdata_strlen_(asmdata->symbols[i].namecopy);
		asmdata_appendbytes(asmdata, (uint8_t*)asmdata->symbols[i].namecopy, strlen + 1); // Make sure we include terminating zero.
		asmdata_section_t* sym = asmdata_selectsection(asmdata, "asmdata.symbols");
		asmdata_appendword(asmdata, asmdata->symbols[i].flags, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->symbols[i].sectionindex, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->symbols[i].sectionoffset, ASMDATA_SIZE_64BIT);
		asmdata_appendword(asmdata, stroffset, ASMDATA_SIZE_64BIT);
		asmdata_appendword(asmdata, asmdata->symbols[i].x_lhs, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->symbols[i].x_op, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->symbols[i].x_rhs, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->symbols[i].reserved, ASMDATA_SIZE_32BIT);
	}
	asmdata_section_t* refs = asmdata_selectsection(asmdata, "asmdata.references");
	for (i = 0; i < asmdata->nreferences; i++) {
		asmdata_appendword(asmdata, asmdata->references[i].baseflags, ASMDATA_SIZE_8BIT);
		asmdata_appendword(asmdata, asmdata->references[i].size, ASMDATA_SIZE_8BIT);
		asmdata_appendword(asmdata, asmdata->references[i].extflags, ASMDATA_SIZE_16BIT);
		asmdata_appendword(asmdata, asmdata->references[i].sectionindex, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->references[i].sectionoffset, ASMDATA_SIZE_64BIT);
		asmdata_appendword(asmdata, asmdata->references[i].symbolindex, ASMDATA_SIZE_32BIT);
		asmdata_appendword(asmdata, asmdata->references[i].extdata, ASMDATA_SIZE_32BIT);
	}

	asmdata->finalised = true;

	return true;
}

bool asmdata_produceheader(asmdata_t* asmdata, int32_t pagesize, const char* hint1, const char* hint2, int32_t inthint) {
	/* Exclude any unreasonably small/large page sizes, but allow weird/unaligned ones just in case. */
	if (pagesize < 1 || pagesize >(1024 * 1024 * 1024)) {
		return false;
	}
	/* The header can only be produced once, otherwise it would get too confusing... */
	if (asmdata_hassection(asmdata, "asmdata.fileheader")) {
		return false;
	}
	/* If the strings section hasn't already been initialised, we should do the same initialisation that would happen in
	 * asmdata_finalise(), ensuring that the strings section is created before the file header.
	 */
	if (!asmdata_hassection(asmdata, "asmdata.strings")) {
		/* The first string will be at offset 0 (which might also imply NULL), so we can start by adding a NULL string there. An interpreter
		 * should generally handle NULL strings differently, but in case any are automatically read the result will be a marker: */
		if (asmdata_selectsection(asmdata, "asmdata.strings") == NULL) {
			return false;
		}
		const char* nullstr = "<ASMDATA-NULL-STRING>";
		asmdata_appendbytes(asmdata, (uint8_t*)nullstr, asmdata_strlen_(nullstr) + 1); // Be sure to include the terminating zero byte!
	}
	int32_t nstrsection = asmdata_selectsection(asmdata, "asmdata.strings")->sectionnumber;

	/* Now we can start with the file header itself: */
	if (asmdata_selectsection(asmdata, "asmdata.fileheader") == NULL) {
		return false;
	}
	int32_t nhdrsection = asmdata_selectsection(asmdata, "asmdata.fileheader")->sectionnumber;

	asmdata_appendbytes(asmdata, (uint8_t*)"ASMDATA1", 8);

	asmdata_appendword(asmdata, 1, ASMDATA_SIZE_32BIT); // Version number
	asmdata_appendword(asmdata, pagesize, ASMDATA_SIZE_32BIT);

	if (hint1 == NULL) {
		asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT);
	}
	else {
		int64_t stroffset = asmdata_selectsection(asmdata, "asmdata.strings")->reservedsize;
		asmdata_appendbytes(asmdata, (uint8_t*)hint1, asmdata_strlen_(hint1) + 1); // Include terminating zero byte
		asmdata_selectsection(asmdata, "asmdata.fileheader");
		asmdata_appendword(asmdata, stroffset, ASMDATA_SIZE_64BIT);
	}
	if (hint2 == NULL) {
		asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT);
	}
	else {
		int64_t stroffset = asmdata_selectsection(asmdata, "asmdata.strings")->reservedsize;
		asmdata_appendbytes(asmdata, (uint8_t*)hint2, asmdata_strlen_(hint2) + 1); // Include terminating zero byte
		asmdata_selectsection(asmdata, "asmdata.fileheader");
		asmdata_appendword(asmdata, stroffset, ASMDATA_SIZE_64BIT);
	}
	asmdata_appendword(asmdata, inthint, ASMDATA_SIZE_32BIT);

	asmdata_appendword(asmdata, nstrsection, ASMDATA_SIZE_32BIT);
	asmdata_appendword(asmdata, nhdrsection, ASMDATA_SIZE_32BIT);

	asmdata_appendword(asmdata, asmdata->nsections, ASMDATA_SIZE_32BIT);

	/* We need to add the section name strings before generating the section list, otherwise the newly-added strings
	 * won't all get added to the string section before it's totals are recorded in the header.
	 */
	int64_t nameoffsets[ASMDATA_MAXSECTIONS];
	int32_t i;
	for (i = 0; i < asmdata->nsections; i++) {
		nameoffsets[i] = asmdata_selectsection(asmdata, "asmdata.strings")->reservedsize;
		asmdata_appendbytes(asmdata, (uint8_t*)asmdata->sections[i]->namecopy, asmdata_strlen_(asmdata->sections[i]->namecopy) + 1); // Include terminating zero byte
	}

	/* TODO: Hash values. */
	int64_t hdrsize = asmdata_selectsection(asmdata, "asmdata.fileheader")->reservedsize;
	hdrsize += asmdata->nsections * 8 * 8;
	hdrsize += 16; // Final file size and checksum fields
	// NOTE: The header size is double-checked against the produced header at the end of this function

	int64_t sectionoffset = hdrsize;
	while (sectionoffset % pagesize != 0) {
		sectionoffset++;
	}
	for (i = 0; i < asmdata->nsections; i++) {
		asmdata_section_t* s = asmdata->sections[i];
		asmdata_appendword(asmdata, s->bigendian ? 1 : 0, ASMDATA_SIZE_32BIT); // Encoding flags (in future may also specify compression etc.)
		asmdata_appendword(asmdata, 0, ASMDATA_SIZE_32BIT); // Content flags (in future may specify mapped/unmapped/executable/writable/etc.)
		asmdata_appendword(asmdata, sectionoffset, ASMDATA_SIZE_64BIT); // Offset in file
		asmdata_appendword(asmdata, s->virtualoffset, ASMDATA_SIZE_64BIT);
		if (s == asmdata->activesection) { /* The size field needs to be hard-coded for the header since we're still creating it! */
			asmdata_appendword(asmdata, hdrsize, ASMDATA_SIZE_64BIT); // Size in file (not including page boundary padding)
			asmdata_appendword(asmdata, hdrsize, ASMDATA_SIZE_64BIT); // Size in memory (for header section, is the same as
			asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT); // Reserved
			asmdata_appendword(asmdata, nameoffsets[i], ASMDATA_SIZE_64BIT);
			asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT); // Hash (TODO)
			sectionoffset += hdrsize;
		}
		else {
			asmdata_appendword(asmdata, s->bufferfilled, ASMDATA_SIZE_64BIT); // Size in file (not including page boundary padding)
			asmdata_appendword(asmdata, s->reservedsize, ASMDATA_SIZE_64BIT); // Size in memory (padded with zero by the loader)
			asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT); // Reserved
			asmdata_appendword(asmdata, nameoffsets[i], ASMDATA_SIZE_64BIT);
			asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT); // Hash (TODO)
			sectionoffset += s->bufferfilled;
		}
		while (sectionoffset % pagesize != 0) {
			sectionoffset++;
		}
	}
	asmdata_appendword(asmdata, sectionoffset, ASMDATA_SIZE_64BIT); // Total file size
	asmdata_appendword(asmdata, 0, ASMDATA_SIZE_64BIT); // Checksum (TODO)

	/* Make sure the written header size exactly matches the size we calculated before writing. */
	if (asmdata->activesection->bufferfilled != hdrsize) {
		return false;
	}

	return true;
}

asmdata_section_t* asmdata_findsection(asmdata_t* asmdata, const char* sectionname, bool autocreate) {
	int32_t i;
	for (i = 0; i < asmdata->nsections; i++) {
		if (asmdata_streq_(asmdata->sections[i]->namecopy, sectionname)) {
			return asmdata->sections[i];
		}
	}
	if (autocreate && asmdata->nsections < ASMDATA_MAXSECTIONS) {
		asmdata_section_t* result = asmdata_section_new(sectionname);
		if (result == NULL) {
			return NULL;
		}
		asmdata->sections[asmdata->nsections] = result;
		result->sectionnumber = asmdata->nsections;
		asmdata->nsections++;
		return result;
	}
	return NULL;
}



bool asmdata_beginfile(asmdata_t* asmdata, const char* name) {
	return true; // TODO init?
}

bool asmdata_endfile(asmdata_t* asmdata, const char* name) {
	return true; // TODO cleanup/checks?
}

static int32_t asmdata_dummysymbol(asmdata_t* asmdata, const char* dummyname) {
	int32_t i;

	if (asmdata->nsymbols >= ASMDATA_MAXSYMBOLS) {
		return -1;
	}
	i = asmdata->nsymbols;
	asmdata->symbols[i].flags |= ASMDATA_SYMBOL_DUMMY;
	asmdata->symbols[i].namecopy = asmdata_strdup_(dummyname);
	asmdata->symbols[i].sectionindex = -1;
	asmdata->nsymbols++;
	return i;
}

int32_t asmdata_findsymbol(asmdata_t* asmdata, const char* name, bool autocreate) {
	int32_t i;
	for (i = 0; i < asmdata->nsymbols; i++) {
		if (asmdata_streq_(asmdata->symbols[i].namecopy, name)) {
			return i;
		}
	}
	if (!autocreate || asmdata->nsymbols >= ASMDATA_MAXSYMBOLS) {
		return -1;
	}
	i = asmdata->nsymbols;
	asmdata->symbols[i].namecopy = asmdata_strdup_(name);
	asmdata->symbols[i].sectionindex = -1;
	asmdata->nsymbols++;
	return i;
}

int32_t asmdata_symbolhere(asmdata_t* asmdata, const char* name) {
	int32_t idx = asmdata_findsymbol(asmdata, name, true);
	if (idx < 0) {
		return idx;
	}
	asmdata_symbol_t* symbol = &(asmdata->symbols[idx]);
	if (symbol->sectionindex != -1) {
		return -1; // It's already defined?
	}
	symbol->sectionindex = asmdata_activesection(asmdata)->sectionnumber;
	symbol->sectionoffset = asmdata_activesection(asmdata)->reservedsize;
	// TODO: Clear flags etc.?
	return idx;
}

int32_t asmdata_appendreferenceword(asmdata_t* asmdata, const char* name, int8_t size) {
	int64_t startoffset = asmdata_activesection(asmdata)->reservedsize;
	if (!asmdata_appendword(asmdata, 0, size)) {
		return -1;
	}
	if (asmdata->nreferences >= ASMDATA_MAXREFERENCES) {
		return -1;
	}
	int32_t idx = asmdata->nreferences;
	asmdata_reference_t* reference = &(asmdata->references[idx]);
	reference->symbolindex = asmdata_findsymbol(asmdata, name, true);
	if (reference->symbolindex < 0) {
		return -1;
	}
	reference->sectionindex = asmdata_activesection(asmdata)->sectionnumber;
	//printf("Note: Reference '%s' at offset %d\n", name, startoffset);
	reference->sectionoffset = startoffset;
	reference->size = size;
	// TODO: Clear flags etc.?
	asmdata->nreferences++;
	return idx;
}

static int32_t asmdata_appendsubref_(asmdata_t* asmdata, int32_t symbol, int8_t size) {
	int64_t startoffset = asmdata_activesection(asmdata)->reservedsize;
	if (!asmdata_appendword(asmdata, 0, size)) {
		return -1;
	}
	if (asmdata->nreferences >= ASMDATA_MAXREFERENCES) {
		return -1;
	}
	int32_t idx = asmdata->nreferences;
	asmdata_reference_t* reference = &(asmdata->references[idx]);
	reference->symbolindex = symbol;
	if (reference->symbolindex < 0) {
		return -1;
	}
	reference->sectionindex = asmdata_activesection(asmdata)->sectionnumber;
	//printf("Note: Reference '%s' at offset %d\n", name, startoffset);
	reference->sectionoffset = startoffset;
	reference->size = size;
	// TODO: Clear flags etc.?
	asmdata->nreferences++;
	return idx;
}

static int32_t asmdata_subx_(asmdata_t* asmdata, int32_t tt, const char* tokenstr, asmlnx_t* expr) {
	if (tt == ASMT_TOKENTYPE_NAME) { // Shortcut if this part of the expression is just a simple symbol name
		return asmdata_findsymbol(asmdata, tokenstr, true);
	}

	int32_t result = asmdata_dummysymbol(asmdata, tokenstr);

	if (result >= 0) {
		asmdata_symbol_t* sym = &asmdata->symbols[result];
		switch (tt) {
		case ASMT_TOKENTYPE_NUMBER:
			sym->flags |= ASMDATA_SYMBOL_CONST;
			sym->sectionoffset = atoll(tokenstr);
			break;
		case ASMT_TOKENTYPE_OPENBR:
			sym->x_lhs = asmdata_subx_(asmdata, expr->lhstype, expr->lhscopy, expr->lhsx);
			if (sym->x_lhs < 0) {
				return -1;
			}
			sym->x_op = asmdata_dummysymbol(asmdata, expr->opcopy);
			if (sym->x_op < 0) {
				return -1;
			}
			asmdata->symbols[sym->x_op].flags |= ASMDATA_SYMBOL_OP;
			sym->x_rhs = asmdata_subx_(asmdata, expr->rhstype, expr->rhscopy, expr->rhsx);
			if (sym->x_rhs < 0) {
				return -1;
			}
			sym->flags |= ASMDATA_SYMBOL_CONST;
			break;
		default:
			return -1;
		}
	}

	return result;
}

static bool asmdata_appendvalue_(asmdata_t* asmdata, int32_t tokentype, const char* tokenstr, asmlnx_t* expr, int8_t primsize) {
	if (tokentype == ASMT_TOKENTYPE_NUMBER) {
		long long x = atoll(tokenstr);
		//printf("Got number %lld\n", x);
		return asmdata_appendword(asmdata, (int64_t)x, primsize);
	}
	else if (tokentype == ASMT_TOKENTYPE_STRING) {
		int32_t l = asmdata_strlen_(tokenstr);
		int32_t i;
		for (i = 0; i < l; i++) {
			if (!asmdata_appendword(asmdata, (int64_t)((int8_t)tokenstr[i]), primsize)) {
				return false;
			}
		}
		return true;
	}
	else if (tokentype == ASMT_TOKENTYPE_NAME) {
		int32_t refidx = asmdata_appendreferenceword(asmdata, tokenstr, primsize);
		if (refidx < 0) {
			return false;
		}
		else {
			return true;
		}
	}
	else if (tokentype == ASMT_TOKENTYPE_OPENBR) {
		int32_t exprsym = asmdata_subx_(asmdata, tokentype, tokenstr, expr);
		if (exprsym < 0) {
			return false;
		}
		int32_t ref = asmdata_appendsubref_(asmdata, exprsym, primsize);
		if (ref < 0) {
			return false;
		}
		else {
			return true;
		}
	}
	else {
		return false;
	}
}

bool asmdata_isvalidasmln(asmdata_t* asmdata, asmln_t* asmln) {
	if (asmln == NULL || asmln->errorcopy != NULL) {
		return false; // If there's a major error it's obviously not a valid data line
	}

	if (asmln->instrcopy == NULL) {
		return true; // If there's no "instruction" part then it's either a plain label or an empty/comment line, which is perfectly valid
	}

	/* For normal lines (with an "instruction" part) then we recognise anything that is data or simple section/symbol/linkage instruction */
	return asmdata_streq_(asmln->instrcopy, "data8") || asmdata_streq_(asmln->instrcopy, "data16")
		|| asmdata_streq_(asmln->instrcopy, "data32") || asmdata_streq_(asmln->instrcopy, "data64")
		|| asmdata_streq_(asmln->instrcopy, "section") || asmdata_streq_(asmln->instrcopy, "symbol")
		|| asmdata_streq_(asmln->instrcopy, "align") || asmdata_streq_(asmln->instrcopy, "reserve");
}

bool asmdata_asmln(asmdata_t* asmdata, asmln_t* asmln) {
	if (!asmdata_isvalidasmln(asmdata, asmln)) {
		if (asmln != NULL && asmln->errorcopy == NULL) {
			asmln->errorcopy = asmdata_strdup_("Not a valid data instruction");
		}
		return false;
	}

	if (asmln->labelcopy != NULL) {
		int32_t idx = asmdata_symbolhere(asmdata, asmln->labelcopy);
		if (idx < 0) {
			asmln->errorcopy = asmdata_strdup_("Failed to create symbol (label already defined?)");
			return false;
		}
	}

	if (asmln->instrcopy == NULL && asmln->nparams == 0) {
		/* No instruction. We're done! (Parameters were also checked to be 0, just to be pedantic.) */
		return true;
	}

	if (asmdata_streq_(asmln->instrcopy, "section")) {
		if (asmln->nparams == 0) {
			asmdata->activesection = NULL; // Reset the section
			return true;
		}
		else if (asmln->nparams == 1) {
			if (asmln->paramtype[0] != ASMT_TOKENTYPE_NAME && asmln->paramtype[0] != ASMT_TOKENTYPE_STRING) {
				asmln->errorcopy = asmdata_strdup_("section instruction expects section identified by a name or string (but got a different parameter)");
				return false;
			}
			asmdata_selectsection(asmdata, asmln->paramcopy[0]);
			return true;
		}
		else {
			asmln->errorcopy = asmdata_strdup_("TODO: Additional section parameters");
			return false;
		}
	}
	else if (asmdata_streq_(asmln->instrcopy, "symbol")) {
		// TODO: Add symbol flags that can be changed with this instruction
		return true; // Ignore for now.
	}
	else if (asmdata_streq_(asmln->instrcopy, "align")) {
		if (asmln->nparams != 1 || asmln->paramtype[0] != ASMT_TOKENTYPE_NUMBER) {
			asmln->errorcopy = asmdata_strdup_("align special instruction requires a simple integer");
			return false;
		}
		long x = atoll(asmln->paramcopy[0]);
		while ((asmdata_activesection(asmdata)->reservedsize % x) != 0) {
			asmdata_activesection(asmdata)->reservedsize++;
		}
		return true;
	}
	else if (asmdata_streq_(asmln->instrcopy, "reserve")) {
		if (asmln->nparams != 1 || asmln->paramtype[0] != ASMT_TOKENTYPE_NUMBER) {
			asmln->errorcopy = asmdata_strdup_("reserve special instruction requires a simple integer");
			return false;
		}
		long x = atoll(asmln->paramcopy[0]);
		while (x > 0) {
			asmdata_activesection(asmdata)->reservedsize++;
			x--;
		}
		return true;
	}
	else if (asmdata_streq_(asmln->instrcopy, "data8")) {
		if (asmln->nparams < 1) {
			asmln->errorcopy = asmdata_strdup_("data instructions generally expect at least one value, otherwise they are probably erroneous");
			return false;
		}
		int32_t i;
		for (i = 0; i < asmln->nparams; i++) {
			//printf("DOING ARG %d\n", i);
			if (!asmdata_appendvalue_(asmdata, asmln->paramtype[i], asmln->paramcopy[i], asmln->paramx[i], ASMDATA_SIZE_8BIT)) {
				asmln->errorcopy = asmdata_strdup_("invalid data value");
				return false;
			}
		}
		return true;
	}
	else if (asmdata_streq_(asmln->instrcopy, "data16")) {
		if (asmln->nparams < 1) {
			asmln->errorcopy = asmdata_strdup_("data instructions generally expect at least one value, otherwise they are probably erroneous");
			return false;
		}
		int32_t i;
		for (i = 0; i < asmln->nparams; i++) {
			if (!asmdata_appendvalue_(asmdata, asmln->paramtype[i], asmln->paramcopy[i], asmln->paramx[i], ASMDATA_SIZE_16BIT)) {
				asmln->errorcopy = asmdata_strdup_("invalid data value");
				return false;
			}
		}
		return true;
	}
	else if (asmdata_streq_(asmln->instrcopy, "data32")) {
		if (asmln->nparams < 1) {
			asmln->errorcopy = asmdata_strdup_("data instructions generally expect at least one value, otherwise they are probably erroneous");
			return false;
		}
		int32_t i;
		for (i = 0; i < asmln->nparams; i++) {
			if (!asmdata_appendvalue_(asmdata, asmln->paramtype[i], asmln->paramcopy[i], asmln->paramx[i], ASMDATA_SIZE_32BIT)) {
				asmln->errorcopy = asmdata_strdup_("invalid data value");
				return false;
			}
		}
		return true;
	}
	else if (asmdata_streq_(asmln->instrcopy, "data64")) {
		if (asmln->nparams < 1) {
			asmln->errorcopy = asmdata_strdup_("data instructions generally expect at least one value, otherwise they are probably erroneous");
			return false;
		}
		int32_t i;
		for (i = 0; i < asmln->nparams; i++) {
			if (!asmdata_appendvalue_(asmdata, asmln->paramtype[i], asmln->paramcopy[i], asmln->paramx[i], ASMDATA_SIZE_64BIT)) {
				asmln->errorcopy = asmdata_strdup_("invalid data value");
				return false;
			}
		}
		return true;
	}
	else {
		asmln->errorcopy = asmdata_strdup_("internal error, asmdata recognised input but didn't translate it properly");
		return false;
	}
}