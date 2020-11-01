#ifndef ASMGEN_H
#define ASMGEN_H

/* Definitions of "generic" instruction set as simple NULL-terminated lists of strings.*/

#define ASMGEN_REGNAMES \
	"r0",\
	"r1",\
	"r2",\
	"r3",\
	"r4",\
	"r5",\
	"r6",\
	"r7",\
	"r8",\
	"r0x8",\
	"r0x16",\
	"r0x32",\
	"r0x64",\
	"r1x8",\
	"r1x16",\
	"r1x32",\
	"r1x64",\
	"f0",\
	"f1",\
	"f2",\
	"f3",\
	NULL

#define ASMGEN_INSTRNAMES \
	"setrr",\
	"setrrm",\
	"setrc",\
	"setrcm",\
	"setrcpc",\
	"setrcpcm",\
	"setrrpc",\
	"setrrpcm",\
	"setrpcmr",\
	"setrpcmcx8",\
	"setrpcmcx16",\
	"setrpcmcx32",\
	"setrpcmf",\
	"setrpcmfx32",\
	"setrmf",\
	"setfrm",\
	"setfrpcm",\
	"setfrpcmx32",\
	"setrflageq",\
	"setrflagne",\
	"setrflaglt",\
	"setrflaggt",\
	"setcmr",\
	"addrr",\
	"addrc",\
	"subrr",\
	"subrc",\
	"mulrr",\
	"mulrc",\
	"divrr",\
	"divrc",\
	"signx64x32",\
	"pushr",\
	"popr",\
	"return",\
	"notr",\
	"comprr",\
	"comprc",\
	"compff",\
	"jumpcifne",\
	"jumpcifeq",\
	"intf",\
	"floati",\
	"floatr",\
	"floatrx32",\
	"floatsfd",\
	"movff",\
	"setfdfs",\
	"setfcm",\
	"callr",\
	"callc",\
	"jumpr",\
	"jumpc",\
	NULL

/* From ifndef at top of file: */
#endif