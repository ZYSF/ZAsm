#ifndef ASMDATA_H
#define ASMDATA_H

#include "asmln.h"

#define ASMDATA_MAXSECTIONS 1000
#define ASMDATA_MAXFILLED	(1024*1024*1024)
#define ASMDATA_MAXSYMBOLS	100000
#define ASMDATA_MAXREFERENCES 100000

typedef struct asmdata asmdata_t;
typedef struct asmdata_section asmdata_section_t;
typedef struct asmdata_symbol asmdata_symbol_t;
typedef struct asmdata_reference asmdata_reference_t;

struct asmdata_reference {
	int32_t symbolindex;
	int32_t sectionindex;
	int64_t sectionoffset;
	int8_t size;	// 0 for 8bit, 1 for 16bit, 2 for 32bit, 3 for 64bit
	int8_t baseflags;
	int16_t extflags;
	int32_t extdata;
};

#define ASMDATA_SYMBOL_DUMMY	(1<<8)
#define ASMDATA_SYMBOL_EXPR		(1<<9)
#define ASMDATA_SYMBOL_CONST	(1<<10)
#define ASMDATA_SYMBOL_OP		(1<<11)

struct asmdata_symbol {
	const char* namecopy;
	int32_t flags; // 0 if not defined
	int32_t sectionindex; // -1 if not defined
	int32_t firstreferenceindex; // -1 if not defined
	int64_t sectionoffset; // 0 if not defined, also reused for constant values in expressions (but with -1 for section)
	int32_t x_lhs;
	int32_t x_op;
	int32_t x_rhs;
	int32_t reserved;
};

struct asmdata_section {
	int32_t sectionnumber;
	bool bigendian;
	const char* namecopy;
	uint8_t* buffer;
	int32_t buffersize;
	int32_t bufferfilled;
	int64_t reservedsize;
	int64_t virtualoffset;
};

struct asmdata {
	bool finalised;
	asmdata_section_t* activesection;
	int32_t nsections;
	asmdata_section_t* sections[ASMDATA_MAXSECTIONS];
	int32_t nsymbols;
	asmdata_symbol_t symbols[ASMDATA_MAXSYMBOLS];
	int32_t nreferences;
	asmdata_reference_t references[ASMDATA_MAXREFERENCES];
};

asmdata_section_t* asmdata_section_new(const char* name);
bool asmdata_section_reserveextra(asmdata_section_t* section, int64_t nbytes, bool willfill);
void* asmdata_section_delete(asmdata_section_t* section);

ASMLN_INLINE bool asmdata_section_align(asmdata_section_t* section, int64_t alignment) {
	while (section->reservedsize % alignment != 0) {
		if (!asmdata_section_reserveextra(section, 1, false)) {
			return false;
		}
	}
	return true;
}

ASMLN_INLINE uint8_t* asmdata_section_fill(asmdata_section_t* section, int64_t nbytes) {
	int64_t offset = section->reservedsize;

	if (((int64_t)((int32_t)offset)) != offset || ((int64_t)((int32_t)nbytes)) != nbytes) {
		return NULL;
	}

	if (!asmdata_section_reserveextra(section, nbytes, true)) {
		return NULL;
	}

	return &(section->buffer[(int32_t)offset]);
}

#define ASMDATA_SIZE_8BIT	((int8_t)0)
#define ASMDATA_SIZE_16BIT	((int8_t)1)
#define ASMDATA_SIZE_32BIT	((int8_t)2)
#define ASMDATA_SIZE_64BIT	((int8_t)3)
ASMLN_INLINE bool asmdata_section_appendword(asmdata_section_t* section, int64_t word, int8_t size) {
	uint8_t* target = asmdata_section_fill(section, 1LL << size);

	if (target == NULL) {
		return false;
	}

	switch (size) {
	case ASMDATA_SIZE_8BIT:
		target[0] = (uint8_t)word;
		return true;
	case ASMDATA_SIZE_16BIT:
		if (section->bigendian) {
			target[0] = (uint8_t)(word >> 8);
			target[1] = (uint8_t)(word);
		}
		else {
			target[0] = (uint8_t)(word);
			target[1] = (uint8_t)(word >> 8);
		}
		return true;
	case ASMDATA_SIZE_32BIT:
		if (section->bigendian) {
			target[0] = (uint8_t)(word >> 24);
			target[1] = (uint8_t)(word >> 16);
			target[2] = (uint8_t)(word >> 8);
			target[3] = (uint8_t)(word);
		}
		else {
			target[0] = (uint8_t)(word);
			target[1] = (uint8_t)(word >> 8);
			target[2] = (uint8_t)(word >> 16);
			target[3] = (uint8_t)(word >> 24);
		}
		return true;
	case ASMDATA_SIZE_64BIT:
		if (section->bigendian) {
			target[0] = (uint8_t)(word >> 56);
			target[1] = (uint8_t)(word >> 48);
			target[2] = (uint8_t)(word >> 40);
			target[3] = (uint8_t)(word >> 32);
			target[4] = (uint8_t)(word >> 24);
			target[5] = (uint8_t)(word >> 16);
			target[6] = (uint8_t)(word >> 8);
			target[7] = (uint8_t)(word);
		}
		else {
			target[0] = (uint8_t)(word);
			target[1] = (uint8_t)(word >> 8);
			target[2] = (uint8_t)(word >> 16);
			target[3] = (uint8_t)(word >> 24);
			target[4] = (uint8_t)(word >> 32);
			target[5] = (uint8_t)(word >> 40);
			target[6] = (uint8_t)(word >> 48);
			target[7] = (uint8_t)(word >> 58);
		}
		return true;
	default:
		return false;
	}
}

ASMLN_INLINE bool asmdata_section_appendbytes(asmdata_section_t* section, uint8_t* source, int32_t nbytes) {
	uint8_t* target = asmdata_section_fill(section, nbytes);

	if (target == NULL) {
		return false;
	}

	int32_t i;
	for (i = 0; i < nbytes; i++) {
		target[i] = source[i];
	}

	return true;
}

asmdata_t* asmdata_new();
void* asmdata_delete(asmdata_t* asmdata);

/* This function should be called (exactly) once before extracting data, assuming you want linkage information retained.
 * It will assemble the symbol and references list (and possibly any additional metadata) into their own special sections.
 * You generally shouldn't assemble anything else after finalising the asmdata structure (the API will still let you though,
 * in case you want to add e.g. a special checksum or signature section based on the finalised contents of the other sections,
 * but defining or using any symbols after this point is an error).
 */
bool asmdata_finalise(asmdata_t* asmdata);

/* Produces a simple file header. This is added as the last section (typically, but not necessarily, after finalisation). A
 * full binary file can then be produced by writing - firstly - the header section, and then each section (including the header
 * again) and padding to the given page boundary after every section (including the first and last copies of the header section).
 * Two "hint" strings (references into the strings section) can be provided as a simple means to classify file types within
 * higher-level environments (e.g. a system might use a conventions like hint1="program" hint2="generic-dynamic-64bit", and might
 * use different hints like hint1="library" vs hint1="program" to distinguish components, but no convention is mandated specifically).
 * The hint strings are only intended to confirm type information and may be empty or ignored. An additional "inthint" field is
 * also added in case high level systems need to be able to quickly identify things like architecture flags without loading strings.
 * No additional metadata (e.g. filename, architecture, build time) is added in the file header. This is by design (it can easily
 * be added in another section if necessary). A simple checksum of each section is added with it's record in the header, while
 * the checksum of the file header itself is calculated with it's own checksum field set to zero (before it has been set!). The
 * second copy of the file header is added primarily for integrity (i.e. if a section may be corrupted, how do we know the header
 * or checksum itself is not corrupted? With a second copy of course), however it's secondary purpose is to verify the end of file
 * for a loader. The file header will also contain a version number (currently 1), which should be taken as the version of the
 * file header/format only (not necessarily related to the version of the assembler/compiler/architecture or even necessarily of
 * the symbols/references sections which may not even exist in the output, but the strings section must be compatible for the
 * hints to be used).
 *
 * Note on duplicating header: Having the header in it's own section may also be convenient for special cases e.g. having to
 * inspect the header itself in "readable" assembly output or, in the future, including alternative headers for different
 * environments or files with multiple sub-files/file-headers for the purposes of optimisation. In these cases, a definitive
 * file header is still given at the start of the file, but an interpreter may use that header's section list to find the most
 * suitable alternative file header for it's environment.
 *
 * Note on page sizes: For compact binaries, a page size of 1 will leave no padding between sections, but other considerations:
 * Larger page sizes are primarily useful for optimising specific cases and specifically for sharing (at page granularity)
 * these pages between multiple program instances in a modern (multitasking) operating system. A page size of up to around
 * 64kb might make sense in some cases but larger page sizes (e.g. 2MB) typically lead to far-oversized program files. In
 * any case, a loader can still determine the same program contents regardless of the page size, so a smaller size (either 1
 * or a value such as 4, 8, or 16 to ensure basic alignment of fields) is generally a better default than a larger one. An
 * interpreter may expect a specific page size matching it's own environmental considerations (e.g. if it's too large a
 * smaller machine might run out of memory, and if it's too small it might have to copy all the data for alignment).
 */
bool asmdata_produceheader(asmdata_t* asmdata, int32_t pagesize, const char* hint1, const char* hint2, int32_t inthint);

asmdata_section_t* asmdata_findsection(asmdata_t* asmdata, const char* sectionname, bool autocreate);

ASMLN_INLINE bool asmdata_hassection(asmdata_t* asmdata, const char* sectionname) {
	return asmdata_findsection(asmdata, sectionname, false) != NULL;
}

ASMLN_INLINE asmdata_section_t* asmdata_selectsection(asmdata_t* asmdata, const char* sectionname) {
	asmdata_section_t* result = asmdata_findsection(asmdata, sectionname, true);

	if (result != NULL) {
		asmdata->activesection = result;
	}

	return result;
}

ASMLN_INLINE asmdata_section_t* asmdata_activesection(asmdata_t* asmdata) {
	if (asmdata->activesection == NULL) {
		return asmdata_selectsection(asmdata, "data");
	}
	return asmdata->activesection;
}

ASMLN_INLINE bool asmdata_appendword(asmdata_t* asmdata, int64_t word, int8_t size) {
	return asmdata_section_appendword(asmdata_activesection(asmdata), word, size);
}

ASMLN_INLINE bool asmdata_appendbytes(asmdata_t* asmdata, uint8_t* source, int32_t nbytes) {
	return asmdata_section_appendbytes(asmdata_activesection(asmdata), source, nbytes);
}

bool asmdata_beginfile(asmdata_t* asmdata, const char* name);
bool asmdata_endfile(asmdata_t* asmdata, const char* name);

int32_t asmdata_findsymbol(asmdata_t* asmdata, const char* name, bool autocreate);
int32_t asmdata_symbolhere(asmdata_t* asmdata, const char* name);
int32_t asmdata_appendreferenceword(asmdata_t* asmdata, const char* name, int8_t size);

bool asmdata_isvalidasmln(asmdata_t* asmdata, asmln_t* asmln);
bool asmdata_asmln(asmdata_t* asmdata, asmln_t* asmln);

ASMLN_INLINE bool asmdata_isvalidln(asmdata_t* asmdata, const char* ln) {
	asmln_t* asmln = asmln_new(ln);
	if (asmln == NULL) {
		return false;
	}
	bool result = asmdata_isvalidasmln(asmdata, asmln);
	asmln_delete(asmln);
	return result;
}

ASMLN_INLINE bool asmdata_ln(asmdata_t* asmdata, const char* ln) {
	asmln_t* asmln = asmln_new(ln);
	if (asmln == NULL) {
		return false;
	}
	bool result = asmdata_isvalidasmln(asmdata, asmln);
	asmln_delete(asmln);
	return result;
}

/* From ifndef at top of file: */
#endif