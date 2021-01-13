#ifndef ASMGEN1_H
#define ASMGEN1_H

#include "asmln.h"
#include "asmdata.h"

#define ASMGEN1_INSTR_INFO \
	"00", "xxx", "syscall",\
	"11", "abi", "addimm",\
	"A1", "abc", "add",\
	"A2", "abc", "sub",\
	"A3", "abc", "and",\
	"A4", "abc", "or",\
	"A5", "abc", "xor",\
	"A6", "abc", "shl",\
	"A7", "abc", "shrz",\
	"A8", "abc", "shrs",\
	"B1", "axx", "blink",\
	"B2", "xxc", "bto",\
	"B3", "axc", "be",\
	"B4", "xxx", "before",\
	"B8", "xxx", "bait",\
	"C3", "abi", "ctrlin64",\
	"CB", "bci", "ctrlout64",\
	"D2", "abi", "read32",\
	"DA", "bci", "write32",\
	"E2", "abi", "in32",\
	"EA", "bci", "out32",\
	"FA", "bci", "ifabove",\
	"FB", "bci", "ifbelows",\
	"FE", "bci", "ifequals",\
	NULL, NULL, NULL

#define ASMGEN1_CTRL_INFO \
	"$$CTRL_0", "$$CTRL_CPUID",\
	"$$CTRL_1", "$$CTRL_EXCN",\
	"$$CTRL_2", "$$CTRL_FLAGS",\
	"$$CTRL_3", "$$CTRL_MIRRORFLAGS",\
	"$$CTRL_4", "$$CTRL_XADDR",\
	"$$CTRL_5", "$$CTRL_MIRRORXADDR",\
	"$$CTRL_6", "$$CTRL_TIMER0",\
	"$$CTRL_7", "$$CTRL_TIMER1_RESERVED",\
	"$$CTRL_8", "$$CTRL_SYSTEM0_RESERVED",\
	"$$CTRL_9", "$$CTRL_SYSTEM1_RESERVED",\
	"$$CTRL_A", "",\
	"$$CTRL_B", "",\
	"$$CTRL_C", "",\
	"$$CTRL_D", "",\
	"$$CTRL_E", "",\
	"$$CTRL_F", "",\
	NULL, NULL

#define ASMGEN1_EXCN_INFO \
	"$$EXCN_0", "",\
	"$$EXCN_1", "$$EXCN_BADDOG",\
	"$$EXCN_2", "$$EXCN_INVALIDINSTR",\
	"$$EXCN_3", "$$EXCN_SYSMODEINSTR",\
	"$$EXCN_4", "$$EXCN_BUSERROR",\
	"$$EXCN_5", "",\
	"$$EXCN_6", "$$EXCN_REGISTERERROR",\
	"$$EXCN_7", "$$EXCN_ALUERROR",\
	"$$EXCN_8", "$$EXCN_RESERVED",\
	"$$EXCN_9", "$$EXCN_DINGDONG",\
	"$$EXCN_A", "$$EXCN_HARDWARE",\
	"$$EXCN_B", "",\
	"$$EXCN_C", "",\
	"$$EXCN_D", "",\
	"$$EXCN_E", "",\
	"$$EXCN_F", "",\
	NULL, NULL

#define ASMGEN1_REGISTER_INFO \
	"$r0","",\
	"$r1","",\
	"$r2","",\
	"$r3","",\
	"$r4","",\
	"$r5","",\
	"$r6","$rbase",\
	"$r7","$rstack",\
	"$r8","",\
	"$r9","",\
	"$rA","",\
	"$rB","",\
	"$rC","",\
	"$rD","",\
	"$rE","",\
	"$rF","",\
	NULL,NULL

bool asmgen1_asmln(asmdata_t* asmdata, asmln_t* asmln);

/* From ifndef at top of file: */
#endif