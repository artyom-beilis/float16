# Half Float IEEE Library for C and Z80

This is a library that implements IEEE 16 bit half float in

- C - general purpose code
- Z80 - assembly as useful high performance floating point library for 8bit systems

It supports:

- Basic operations "+", "-", "\*", "/"
- Conversion to/from integer
- Comparison operations

It uses IEEE 754 format. It supports full range of values including +/- inf, nan and subnormal values.
All operations work as expected with the respect of inf/nan values

Only simplest truncation rounding policy implemented, no round to nearest ties to evens provided.

What is expected in future:

- math functions: log/exp, trigonometry etc,
- formatting/parsing functions

## Z80 notes

- All functions are reentrant and relay on alternative set of registers AF', HL', DE', BC', so if your interrupt routines use shadow registers for fast context switching you can't use this library.
- Some functions is IX for frame pointer, IY is not modified.
- Each functions come in two variants the z88dk C calling format and using registers, for example `_f16_add` with parameters passed over stack and `_f6_add_hl_de` that the parameters passed via hl and de, and result returned in hl
