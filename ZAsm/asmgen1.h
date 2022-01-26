#ifndef ASMGEN1_H
#define ASMGEN1_H

#include "asmln.h"
#include "asmdata.h"

/* NOTE: This list may include some planned-but-not-implemented instructions (see the CPU docs for more information).
 * In particular (at time of writing) I've added "h" (as in high-half) versions of the memory and I/O operations before
 * adding them to the CPU implementation.
 
 * As for the syscall and trap instructions, these aren't really intended to be implemented, they're just reserved as
 * "invalid instructions" which intentionally trigger an exception. This is why the assembler doesn't specify an encoding
 * for the parameters; they might be meaningful to an operating system, might just be some reference data the debugger
 * has inserted, or might just be left blank anyway (in any case except "blank", the assembler doesn't know how you want
 * any parameters you might add encoded, but you can do it manually with data instructions anyway). For implementing
 * system calls, it would probably make more sense to pass the system call number as a normal argument (like in a function
 * call), since this would make for faster/simpler lookup in the kernel and more convenient access from C applications, so
 * the blank option is expected to suffice at least for basic usage.
 */

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
	"C7", "axi", "ctrlin",\
	"CF", "xci", "ctrlout",\
	"C3", "axi", "gen1ctrlin64",\
	"CB", "xci", "gen1ctrlout64",\
	"D2", "abi", "read32",\
	"D6", "abi", "read32x",\
	"D7", "abi", "read32h",\
	"DA", "bci", "write32",\
	"DE", "bci", "write32hz",\
	"DF", "bci", "write32hx",\
	"D0", "abi", "read8",\
	"D4", "abi", "read8x",\
	"D1", "abi", "read16",\
	"D5", "abi", "read16x",\
	"D8", "bci", "write8",\
	"D9", "bci", "write16",\
	"D6", "abi", "gen1read32h",\
	"DE", "bci", "gen1write32h",\
	"E2", "abi", "in32",\
	"EA", "bci", "out32",\
	"E6", "abi", "gen1in32h",\
	"EE", "bci", "gen1out32h",\
	"FA", "bca", "ifabove",\
	"FB", "bca", "ifbelows",\
	"FE", "bca", "ifequals",\
	NULL, NULL, NULL

/* NOTE: The CTRL_ and EXCN_ constants aren't supported yet. I'm still not sure if it really makes sense to have them
 * in the assembler (since, generally speaking, these would probably be reimplemented as constants in C code anyway,
 * and additional ones could be added by implementations), but it can't hurt to leave the definitions here as a reference.
 */

#define ASMGEN1_CTRL_INFO \
	"$$CTRL_0", "$$CTRL_CPUID",\
	"$$CTRL_1", "$$CTRL_EXCN",\
	"$$CTRL_2", "$$CTRL_FLAGS",\
	"$$CTRL_3", "$$CTRL_MIRRORFLAGS",\
	"$$CTRL_4", "$$CTRL_XADDR",\
	"$$CTRL_5", "$$CTRL_MIRRORXADDR",\
	"$$CTRL_6", "$$CTRL_TIMER0",\
	"$$CTRL_7", "$$CTRL_TIMER1_RESERVED",\
	"$$CTRL_8", "$$CTRL_SYSTEM0",\
	"$$CTRL_9", "$$CTRL_SYSTEM1",\
	"$$CTRL_A", "$$CTRL_GPIOA_PINS",\
	"$$CTRL_B", "$$CTRL_GPIOA_INPUTMASK__OR_GPIOB__RESERVED",\
	"$$CTRL_C", "$$CTRL_GPIOA_OUTPUTMASK__OR_GPIOC___RESERVED",\
	"$$CTRL_D", "$$CTRL_GPIOA_MASKEDPINS___OR_GPIOD__RESERVER",\
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