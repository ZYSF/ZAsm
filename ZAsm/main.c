#include "asmln.h"
#include "asmdata.h"
#include "asmgeneric_old.h"
#include "asmgen1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void usage(int argc, char** argv, int argi) {
	fprintf(stderr, "TODO: USAGE\n");
}

bool assemble_line(void* assembler, const char* line) {
	return true;
}

void dump_indent(FILE* output, const char* indentString, int indent) {
	while (indent-- > 0) {
		fprintf(output, "%s", indentString);
	}
}

void dump_x(FILE* output, int32_t type, asmlnx_t* x, const char* indentString, int indent) {
	if (type != ASMT_TOKENTYPE_OPENBR || x == NULL) {
		return;
	}

	dump_indent(output, indentString, indent);
	fprintf(output, "[LHS]\t[type %d]\t'%s'\n", x->lhstype, x->lhscopy);
	dump_x(output, x->lhstype, x->lhsx, indentString, indent + 1);

	dump_indent(output, indentString, indent);
	fprintf(output, "[OP]\t'%s'\n", x->opcopy);

	dump_indent(output, indentString, indent);
	fprintf(output, "[RHS]\t[type %d]\t'%s'\n", x->rhstype, x->rhscopy);
	dump_x(output, x->lhstype, x->rhsx, indentString, indent + 1);
}

void dump_asmln(FILE* output, asmln_t* asmln) {
	if (asmln == NULL) {
		fprintf(output, "ERROR: NULL asmln\n");
	}
	if (asmln->errorcopy != NULL) {
		fprintf(output, "ERROR: %s\n", asmln->errorcopy);
	}
	fprintf(output, "%s:\t%s\t[%d params]\t; %s\n",
		asmln->labelcopy == NULL ? "[no label]" : asmln->labelcopy,
		asmln->instrcopy == NULL ? "[no instr]" : asmln->instrcopy,
		asmln->nparams,
		asmln->commentcopy == NULL ? "[no comment]" : asmln->commentcopy);
	int32_t i;
	for (i = 0; i < asmln->nparams; i++) {
		fprintf(output, "\t[param %d]\t[type %d]\t'%s'\n", i, asmln->paramtype[i], asmln->paramcopy[i]);
		dump_x(output, asmln->paramtype[i], asmln->paramx[i], "\t", 2);
	}
}

#define MODE_DATA		0
#define MODE_GEN1		1
#define MODE_GEN1X		2
#define MODE_GENERIC	3


bool assemble_input(void* assembler, FILE* input, int32_t mode) {
	int32_t lnum = 1;
	int32_t buffermax = 1024 * 1024;
	char* buffer = calloc(buffermax, 1);
	if (buffer == NULL) {
		fprintf(stderr, "Out of memory?");
		return false;
	}

	const char* line;
	while ((line = fgets(buffer, buffermax, input)) != NULL) {
		/* First parse the line.*/
		asmln_t* asmln = asmln_new(line);
		/* Now check for parse errors. */
		if (asmln == NULL) {
			fprintf(stderr, "asmln_new failed entirely!");
			return false;
		}
		if (asmln->errorcopy != NULL) {
			dump_asmln(stdout, asmln);
			return false;
		}
		/* If it's a plain data line, just assemble it: */
		if (asmdata_isvalidasmln(assembler, asmln)) {
			bool tmp = asmdata_asmln(assembler, asmln);
			//printf("Done.\n");
			if (!tmp) {
				fprintf(stderr, "Failed to interpret plain data line:\n");
				fprintf(stderr, "Around line %d:\t", lnum);
				dump_asmln(stderr, asmln);
				asmln_delete(asmln);
				free(buffer);
				return false;
			}
		}
		else if (mode == MODE_GEN1 || mode == MODE_GEN1X) {
			bool tmp = asmgen1_asmln(assembler, asmln);
			//printf("Done.\n");
			if (!tmp) {
				fprintf(stderr, "Failed to interpret generic instruction line:\n");
				fprintf(stderr, "Around line %d:\t", lnum);
				dump_asmln(stderr, asmln);
				asmln_delete(asmln);
				free(buffer);
				return false;
			}
		}
		else if (mode == MODE_GENERIC) {
			bool tmp = asmgeneric_asmln(assembler, asmln);
			//printf("Done.\n");
			if (!tmp) {
				fprintf(stderr, "Failed to interpret generic instruction line:\n");
				fprintf(stderr, "Around line %d:\t", lnum);
				dump_asmln(stderr, asmln);
				asmln_delete(asmln);
				free(buffer);
				return false;
			}
		}
		else {
			fprintf(stderr, "This instruction can't be interpreted:\n");
			fprintf(stderr, "Around line %d:\t", lnum);
			dump_asmln(stderr, asmln);
			asmln_delete(asmln);
			free(buffer);
			return false;
		}

		asmln = asmln_delete(asmln);
		memset(buffer, 0, buffermax);
		lnum++;
	}

	free(buffer);
	return true;
}

bool assemble(void* assembler, const char* filename, int32_t mode) {

	FILE* input = fopen(filename, "r");

	if (input == NULL) {
		fprintf(stderr, "Bad filename?");
		return false;
	}

	if (!assemble_input(assembler, input, mode)) {
		fclose(input);
		return false;
	}

	fclose(input);

	return true;
}

bool produce_section(void* assembler, int sectionnum, FILE* output, bool readable) {
	asmdata_t* asmdata = assembler;
	asmdata_section_t* section = asmdata->sections[sectionnum];
	if (readable) {
		fprintf(output, "\t[base=0x%016llX reserved=0x%016llX filled=0x%08llX buffered=0x%08llX]\n", (long long)section->virtualoffset, (long long)section->reservedsize, (long long)section->bufferfilled, (long long)section->buffersize);
		int32_t i = 0;
		while (i < section->bufferfilled) {
			if (i > 0 && i % 16 == 0) {
				fprintf(output, "\n");
			}
			if (i % 16 == 0) {
				fprintf(output, "\tS%04d+0x%016x:\t", sectionnum, i);
			}
			fprintf(output, "%02x ", section->buffer[i]);
			i++;
		}
		fprintf(output, "\n\n");
	}
	return true;
}

bool produce_output(void* assembler, FILE* output, bool readable, bool header, int32_t pagesize) {
	asmdata_t* asmdata = assembler;
	int32_t sectionnum;
	int32_t offset = 0;
	if (header && readable) {
		fprintf(output, "SECTIONS (%d):\n", asmdata->nsections);
		for (sectionnum = 0; sectionnum < asmdata->nsections; sectionnum++) {
			fprintf(output, "\tSECTION %04d: '%s'\n", sectionnum, asmdata->sections[sectionnum]->namecopy);
		}
		for (sectionnum = 0; sectionnum < asmdata->nsections; sectionnum++) {
			fprintf(output, "[SECTION %04d: '%s']\n", sectionnum, asmdata->sections[sectionnum]->namecopy);
			if (!produce_section(assembler, sectionnum, output, readable)) {
				return false;
			}
		}
		fprintf(output, "\n");
	}
	else if (header) {
		asmdata_section_t* hdrsec = asmdata_findsection(asmdata, "asmdata.fileheader", false);
		if (hdrsec == NULL || hdrsec->buffer == NULL) {
			return false;
		}
		if (fwrite(hdrsec->buffer, 1, hdrsec->bufferfilled, output) != hdrsec->bufferfilled) {
			return false;
		}
		offset += hdrsec->bufferfilled;
		while ((offset % pagesize) != 0) {
			char c = 0;
			if (fwrite(&c, 1, 1, output) != 1) {
				return false;
			}
			offset++;
		}
		for (sectionnum = 0; sectionnum < asmdata->nsections; sectionnum++) {
			asmdata_section_t* sec = asmdata->sections[sectionnum];
			fprintf(stderr, "Writing section '%s' (#%d) at file offset %d\n", sec->namecopy, sectionnum, offset);
			if (sec->bufferfilled > 0 && fwrite(sec->buffer, 1, sec->bufferfilled, output) != sec->bufferfilled) {
				return false;
			}
			offset += sec->bufferfilled;
			while ((offset % pagesize) != 0) {
				char c = 0;
				if (fwrite(&c, 1, 1, output) != 1) {
					return false;
				}
				offset++;
			}
		}
		// We're done now
		return true;
	}
	if (readable) {
		fprintf(output, "REFERENCES (%d):\n", asmdata->nreferences);
		int32_t refnum;
		for (refnum = 0; refnum < asmdata->nreferences; refnum++) {
			fprintf(output, "\tREFERENCE %04d -> SYMBOL %04d\n", refnum, asmdata->references[refnum].symbolindex);
		}
		fprintf(output, "\n");
		fprintf(output, "SYMBOLS (%d):\n", asmdata->nsymbols);
		int32_t symnum;
		for (symnum = 0; symnum < asmdata->nsymbols; symnum++) {
			fprintf(output, "\tSYMBOL %04d -> '%s'\n", symnum, asmdata->symbols[symnum].namecopy);
		}
		fprintf(output, "\n");
	}
	return true;
}

int main(int argc, char** argv) {
	//argc = 4;
	//argv = (char* []){ "test", "--stdout", "--readable", "--stdin"/*"--ast-only","--input","C:\\Users\\Zak\\source\\repos\\ZCC\\Debug\\test2.c"*/ };
	int argi = 1;
	FILE* output = NULL;
	void* assembler = asmdata_new();
	bool somethingtodo = false;
	bool readable = false;
	bool finalise = true;
	bool produceheader = true;

	int32_t pagesize = 1024;
	const char* hint1 = "gen1";
	const char* hint2 = NULL;
	int32_t inthint = 0;
	int32_t mode = MODE_GEN1;

	while (argi < argc) {
		if (!strcmp(argv[argi], "--usage")) {
			usage(argc, argv, argi);
			return 0;
		}
		else if (!strcmp(argv[argi], "--output")) {
			if (output != NULL || argi + 1 >= argc) {
				usage(argc, argv, argi);
				return -1;
			}
			argi++;
			output = fopen(argv[argi], "wb");
			if (output == NULL) {
				fprintf(stderr, "ERROR: Failed to open output file '%s'\n", argv[argi]);
				return -1;
			}
		}
		else if (!strcmp(argv[argi], "--stdin")) {
			fprintf(stderr, "NOTE: Assembling from standard input\n");
			if (!assemble_input(assembler, stdin, mode)) {
				fprintf(stderr, "ERROR: Failed to assemble from standard input\n");
				return -1;
			}
			somethingtodo = true;
		}
		else if (!strcmp(argv[argi], "--stdout")) {
			fprintf(stderr, "NOTE: Dumping to standard output (use with --readable to easily inspect output)\n");
			output = stdout;
		}
		else if (!strcmp(argv[argi], "--readable")) {
			fprintf(stderr, "NOTE: Will attempt to produce 'readable' output\n");
			readable = true;
		}
		else if (!strcmp(argv[argi], "--nofinalise")) {
			fprintf(stderr, "NOTE: Output will not be finalised\n");
			finalise = false;
		}
		else if (!strcmp(argv[argi], "--noheader")) {
			fprintf(stderr, "NOTE: Output will not include file header\n");
			produceheader = false;
		}
		else {
			if (!assemble(assembler, argv[argi], mode)) {
				fprintf(stderr, "ERROR: Failed to assemble '%s'\n", argv[argi]);
				return -1;
			}
			somethingtodo = true;
		}
		argi++;
	}

	if (!somethingtodo || output == NULL) {
		fprintf(stderr, "ERROR: Nothing to do? Please define an input and an output!\n");
		usage(argc, argv, 0);
		return -1;
	}

	if (finalise && !asmdata_finalise(assembler)) {
		fprintf(stderr, "ERROR: Finalisation failed.\n");
		return -1;
	}

	if (produceheader && !asmdata_produceheader(assembler, pagesize, hint1, hint2, inthint)) {
		fprintf(stderr, "ERROR: Failed to produce header\n");
	}

	if (!produce_output(assembler, output, readable, produceheader, pagesize)) {
		fprintf(stderr, "ERROR: Failed to produce output\n");
		return -1;
	}

	if (output != stdout) {
		fclose(output);
	}
	output = NULL;

	fprintf(stderr, "FINISHED.\n");

	return 0;
}