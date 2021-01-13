# ZAsm
A simple assembler. Produces a custom linkable format (see ZLink for the linker). Can be used for different instruction sets but includes a layer targetting my "gen1cpu" project.

## Basic usage

    zasm --output my_program.obj my_program.asm
    
    This assumes `my_program.asm` contains the assembler source code and (if it assembles successfully) a linkable binary object will be created in `my_program.obj`.
    
    (See the linker project for how to create flat binaries from the linkable outputs.)

## Syntax & Design

The basic format for an instruction line:

    [optional_label:] [optional_instr_name [optional_param0] [, optional_param1] [, optional_param2] ...] [; Optional comment after semicolon]

For example this is a valid instruction line showing both a label, an instruction (or more correctly a pseudo-instruction), a parameter and a comment:

    my_var: data32 100 ; This should reserve some space for a 32-bit value I can reference elsewhere

Whereas this is four valid instruction lines (including a blank just for demonstration):

    ; This should reserve some space for a 32-bit value I can reference elsewhere
    
    my_var:
    data32 100
    
That is, the syntax itself is similar to conventional assembly syntax, with the exception of *operators*:

    pointer_after_my_var: data32 (my_var + 4) ; This space will hold a pointer to the space just after my_var

Since the assembler doesn't know exactly where my_var will end up in memory, it needs to pass information about the "+" operator (and it's operands) to the linker.

Complex operations can also be constructed from this, for example the instruction `add $r0, $r5, $r1` is internally translated to something like:

    data32 ((((0xA1 << 24) || (0 << 16)) || (5 << 8)) || 1)

Of course that example could be computed by the assembler if it really wanted to, but instructions which reference addresses within the program are also constructed the same way.

This means that the linker doesn't need to know about the instruction encoding, and also that special operators can be added e.g. to imply special checks the linker would have to do to ensure that there are no overflows. In fact you can just use arbitrary operators like:

    data32 (0 SPECIAL_LINKER_LOOKUP 999)

This would reserve space for a 32-bit value, but it would expect your linker to make sence of the "SPECIAL_LINKER_LOOKUP" operator (whatever that means).

This design allows more diverse targets and workflows to be developed from the assembler (e.g. you can strip it down to just the data layer if you like, or use it with the linker to link together data-only binary files).

## TODO

 * Better memory management/internal structures (e.g. the `gen1` layer doesn't free all of it's temporary values - but this should only be an issue if you need to compile very large programs or to use it as a library)
 * Maybe a macro system or similar (for defining custom instructions)
