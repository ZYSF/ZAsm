# ZAsm
A simple assembler. Produces a custom linkable format (see ZLink for the linker). Can be used for different instruction sets but includes a layer targetting my "gen1cpu" project.

## Basic usage

    zasm --output my_program.obj my_program.asm
    
This assumes `my_program.asm` contains the assembler source code and (if it assembles successfully) a linkable binary object will be created in `my_program.obj`.
    
(See the [linker project](https://github.com/ZYSF/ZLink/) for how to create flat binaries from the linkable outputs.)

## Syntax & Design

### Basic Syntax

The basic format for an instruction line (the square braces indicating that these things are optional):

    [optional_label:] [optional_instr_name [optional_param0] [, optional_param1] [, optional_param2] ...] [; Optional comment after semicolon]

For example this is a valid instruction line showing both a label, an instruction (or more correctly a pseudo-instruction), a parameter and a comment:

    my_var: data32 100 ; This should reserve some space for a 32-bit value I can reference elsewhere

Whereas this is four valid instruction lines (including a blank just for demonstration):

    ; This should reserve some space for a 32-bit value I can reference elsewhere
    
    my_var:
    data32 100

### Labels & Operators

Now that we've declared a `my_var` label, we can use a pointer to this on another line:

    pointer_to_my_var: data32 my_var ; This space will hold a pointer to the start of my_var
    
That is, the syntax itself is similar to conventional assembly syntax, but *operators* and *labels* are both treated with particular care:

    pointer_after_my_var: data32 (my_var + 4) ; This space will hold a pointer to the space just after my_var

Since the assembler doesn't know exactly where `my_var` will end up in memory, it needs to pass information about the `+` operator and it's operands to the linker.

### Layering Of Complex Operations

The parser (which handles the syntax) and the data level (which handles data declarations and other basics) are neatly isolated, which allows other layers to fit in the middle.

For the [gen1cpu](https://github.com/ZYSF/gen1cpu/) instruction set, the specific instructions are internally just translated to `data32` lines. For example, the instruction `add $r0, $r5, $r1` is internally translated to something like:

    data32 ((((0xA1 << 24) | (0 << 16)) | (5 << 8)) | 1) ; (0xA1 being the "add" opcode)

Of course that example could be computed by the assembler if it really wanted to (and the above example ignores some details like masking), but this process is handled systematically and instructions which reference addresses within the program are also constructed the same way.

This means that the linker doesn't need to know about the instruction encoding, and also that special operators can be added e.g. to imply special checks the linker would have to do to ensure that there are no overflows. In fact you can just use arbitrary operators like:

    data32 (my_special_label SPECIAL_LINKER_LOOKUP 999)

This would reserve space for a 32-bit value, but it would expect your linker to make sense of the "SPECIAL_LINKER_LOOKUP" operator (whatever that means to some special linker).

This design allows the assembler to be incorporated into more diverse targets and workflows (e.g. you can strip it down to just the data layer if you like, or use it with the linker to link together data-only binary files).

An important thing to note is that each operation must follow the exact format `(left-hand-side operator right-hand-side)`, that is it must always include brackets and can never assume the left or right hand side is implied (so e.g. to pass a unary minus or `not` operation to the linker you'd have to use something like zero as a dummy left-hand side). This ensures that there are no ambiguities in the syntax. For example, if you just did:

    data32 +

Or:

    data32 SPECIAL_LINKER_LOOKUP

Then `+` or `SPECIAL_LINKER_LOOKUP` would still be scanned the same but wouldn't be treated as an operator (it'd just be a name, like `data32 plus`).

## Data Pseudo-Instructions

### section

The `section` pseudo-instruction switches to the binary section named by the first parameter (which can either be a name or a string).

By convention code and data are generally stored in separate sections (which is helpful for a number of practical reasons) but additional sections can be defined as necessary to hold special structures, debug information, or data with other semantics. A new section can be created automatically by switching to it (they don't have any special flags at this point, but sections with particular names such as `code` generally get special treatment during loading).

### symbol

The `symbol` pseudo-instruction is currently just ignored (along with any parameters), but this is reserved for marking symbols with particular attributes, for example:

    symbol main, global     ; Make sure we mark main as global so some future version doesn't treat it differently
    main:
        ; ... my main function here

### align

The `align` pseudo-instruction reserves however many bytes are necessary to align the current output to the number of bytes specified. Sections are assumed to be "perfectly aligned" already (that is, the `align` instruction aligns from the section base, so if you load that section to an unaligned address then the result might not necessarily be aligned).

The typical usage of this is ensuring that variables or functions are stored in reasonable places. For example:

    my_byte: data8 5 ; This space will hold my 1-byte value
    
    my_word: data32 12345 ; This space will hold my 32-bit value (which would be 4 bytes!)
 
Would typically result in the `my_word` space not being aligned to the size of a 32-bit word. In some cases this might be desirable (to pack data in efficiently), but it can lead to slowdowns, encoding issues or other problems when accessing such values. The solution is to align values:

    my_byte: data8 5 ; This space will hold my 1-byte value
    
    align 4 ; This will make sure `my_word` starts on a 4-byte boundary
    my_word: data32 12345 ; This space will hold my 32-bit value (which would be 4 bytes!)


### reserve

The `reserve` pseudo-instruction can be used to reserve a number of blank bytes without necessarily filling them all in with actual zero values. This can be helpful for defining zero-initialised values used by a program.

If you define some specific data after some reserved bytes, then they will generally be filled in with zero values anyway, but if the reserved bytes are at the end of the section then they can easily be optimised away.

### data8

We've already used some `data8` and `data32` pseudo-instructions, which obviously just define some data (or more precisely some space, filled in with some data).

These can be used for strings (text values) as well:

    my_string: data8 "This is my string data"

However to indicate the end of the text there are a couple of options. The C-style way is simple but can be slow to decode:

    my_cstring: data8 "This is a C-style (0-terminated) string", 0

Whereas the Pascal-style way of encoding a string's length is easier to decode (and generally considered "safer") but requires some more care:

    align 4
    my_pstring:
      data32 (my_pstring_end - (my_pstring + 4))
      data8 "This is a pascal-style string (with the size computed as a number at the start)"
    my_pstring_end:

### data16

Works like `data8` except with 16-bit words.

### data32

Works like `data8` except with 32-bit words.

Note that string format is still supported in 32-bit (and other) data values:

    my_wide_string: data32 "These characters would each be expanded to 32 bits", 0

In the future, this should make it easier to work with Unicode content, however in practice right now string syntax is still limited to the ASCII character set.

### data64

Works like `data8` except with 64-bit words.

## "Gen1" Instructions

The instructions for [gen1cpu](https://github.com/ZYSF/gen1cpu/) are only made available in their most basic forms for now (without any macro-instructions or convenience versions).

Only the lower 16 registers are supported for now (just for the sake of simplicity), which are named `$r0`, `$r1`, `$r2`, and so on up to `$rF`. Some additional aliases may be assigned to match calling conventions (e.g. stack pointer) but these aren't final yet. Special constants for control registers are also partly-added but the naming isn't standardised yet (that's not so important since you can just use numbers for them).

Instructions with reserved bits always have those bits set to zero (there are no options to set those bits specially but to support customised instructions you can always just encode your own special instruction with data32).

Examples:

    ; This would just execute a (likely bogus) system call:
    ; This will load "1234" into $r0
    xor $r3, $r3, $r3
    addimm $r3, 1234
    ; Since "syscall" bits are currently reserved, the assembler assumes them to be zero and doesn't take parameters for this:
    syscall

## TODO

 * Encoding of floating point data
 * Handling of symbol options
 * Allow changing endianness from within the assembler syntax (it's already supported internally)
 * Better memory management/internal structures (e.g. the `gen1` layer doesn't free all of it's temporary values - this doesn't matter so much for short/single runs but might become annoying for larger builds)
 * Maybe a macro system or similar (for defining custom instructions or repeated structures)
