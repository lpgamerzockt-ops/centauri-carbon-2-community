# Closing the remaining two ABI gaps: calling convention and LSDA bytes

Follow-up to `ABI_FINAL_CLOSURE_0113.md`, which left two items explicitly
open: the hidden-result-pointer convention for the function `cc2_wait`
replaces, and the byte-level content of the `.ARM.extab` LSDA for
`SelectReactor::pause`. This entry closes both with direct disassembly
and a decoded byte comparison, run with the real ARM cross-toolchain's
own `readelf`/`objdump` (`arm-cortex_a15-linux-gnueabihf-*`, from
`ct-ng-builder:with-toolchain-qemu`) against the actual `688b7bf8…` and
`19712b…`-generation vendor binaries. No device, no installation, no
hash-gate change.

## 1. Calling convention for the patched function: now CONFIRMED

`reactor_module.cpp` patches address `0x26cb70` (0113) /
`0x26a640` (07) with `bridge::cc2_wait`, whose own declared signature is:

```cpp
extern "C" void cc2_wait(void *out, void *self, double waketime, const void *fallback);
```

The symbol table identifies the patched target precisely:

```
_ZN17ReactorCompletion4waitEdRKN8nlohmann16json_abi_v3_12_010basic_json...EE
  = ReactorCompletion::wait(double, nlohmann::json const&)
```

A return type is not part of Itanium C++ mangling for an ordinary member
function, so the symbol name alone cannot show that this function returns
`nlohmann::json` **by value** (too large for registers, so ARM EABI
requires a hidden result pointer in `r0`, with every other parameter
shifted one slot right: `r0`=hidden-out, `r1`=`this`, remaining args
after). The actual compiled prologue, read directly, shows exactly that
shift — and is otherwise byte-identical between builds:

```
old 0x26a640 / new 0x26cb70 (identical opcodes, only bl targets shift):
  push {r4, fp, lr}
  add  fp, sp, #8
  sub  sp, sp, #92
  str  r0, [fp, #-64]   ; r0 -> saved as "out"      (hidden result ptr)
  str  r1, [fp, #-68]   ; r1 -> saved as "self"      (this, ReactorCompletion*)
  vstr d0, [fp, #-76]   ; d0 -> saved as "waketime"  (double, VFP reg, no GPR consumed)
  str  r2, [fp, #-80]   ; r2 -> saved as "fallback"  (const json*)
  ldr  r3, [fp, #-68]
  add  r3, r3, #8
  mov  r0, r3
  bl   <...is_null()>  ; (address-shifts, symbol-identical both builds)
  ...
```

This is an exact structural match to `cc2_wait`'s own four parameters
`(out, self, waketime, fallback)`, in the same registers ARM hard-float
EABI would assign to a hidden-pointer-returning member function with that
exact parameter list. **This closes the "Calling-convention
re-verification" gate for the specific function in question** — not by
assumption, but by direct disassembly of both the 07 and 0113 vendor
binaries, instruction-for-instruction identical except the expected
address-shifted `bl` targets (same pattern documented everywhere else in
this port).

Note: the `SelectReactor::pause` call inside `cc2_wait`'s body
(`original<double(*)(void*,double)>(0x26e5f4)(owner,waketime)`) is a
separate, simpler case — `this` in `r0`, `waketime` in `d0`, `double`
return in `d0` — no hidden pointer involved, and not the subject of this
gate.

## 2. `.ARM.extab` LSDA byte-level encoding for `SelectReactor::pause`: now CONFIRMED

`ABI_FINAL_CLOSURE_0113.md` confirmed the *personality routine* resolves
to `__gxx_personality_v0` in both builds for this function, but explicitly
left the LSDA table's own byte content unexamined. Dumping the raw
`.ARM.extab` bytes at the two entry addresses (`old @0xc22998`,
`new @0xc0fae8`, both 64 bytes / 16 words):

```
old: a4d3497f c9439b01 b0818480 ffff012f 24040000 9801c002 d40900d0
     049802c4 0900a807 04b40900 d0070498 0900e807 04880900 dc0804c4
     0900e009 04000000
new: 7c014b7f c9439b01 b0818480 ffff012f 24040000 9801c002 d40900d0
     049802c4 0900a807 04b40900 d0070498 0900e807 04880900 dc0804c4
     0900e009 04000000
```

**Only the first of 16 words differs.** That first word is the EHABI
"generic model" entry header — a 31-bit place-relative offset (word's own
address, offset added directly, sign-extended) to the personality
routine. Decoding it for both builds:

| Build | Entry address | Encoded word | Decoded target | Matches confirmed `PersonalityRoutineAddress`? |
|---|---|---|---|---|
| old | `0xc22998` | `0x7f49d3a4` | `0xbfd3c` | Yes — exactly, confirmed in `ABI_FINAL_CLOSURE_0113.md` |
| new | `0xc0fae8` | `0x7f4b017c` | `0xbfc64` | Yes — exactly, confirmed in `ABI_FINAL_CLOSURE_0113.md` |

So the one differing word is **fully and exactly explained** by the two
builds' different extab-entry/personality-routine addresses — the same
kind of mechanical address-shift documented throughout this port, not a
content difference. **The remaining 15 words (60 of the 64 bytes) — the
actual LSDA: call-site table and type/catch/cleanup encoding — are
byte-for-byte identical between the 07 and 0113 builds.**

This closes the "LSDA byte-level encoding" item that
`ABI_FINAL_CLOSURE_0113.md` explicitly left open.

## Updated status

| Gate | Status |
|---|---|
| Calling convention for the function `cc2_wait` replaces (`ReactorCompletion::wait`) | **CONFIRMED** by direct disassembly: hidden-result-pointer register layout matches `cc2_wait`'s own signature exactly, identical prologue in both builds |
| `.ARM.extab` LSDA byte-level encoding for `SelectReactor::pause` | **CONFIRMED**: 15 of 16 words byte-identical; the 1 differing word is fully explained by the (already-confirmed) personality-routine address shift |
| Live exception thrown through a real, non-mocked vendor frame | Still **not tested** — this is inherently a runtime property; static analysis cannot close it. Requires a real device or a non-mocked execution harness |
| Toolchain match against the originally-intended toolchain | Still **not closed** — that toolchain's own provenance is undocumented anywhere in this repository; this session's toolchain remains a test candidate, never cross-checked against it |

## What this does and does not establish

This closes every ABI item in `ARM_TEST_PLAN.md` §2 that is closable by
static disassembly of the two already-available vendor binaries. It does
**not** run any code, does not touch any device, does not change any hash
constant or firmware pin, and does not authorize installation or printing.
The two gates that remain open (live exception propagation, and matching
the originally-intended toolchain) are not static-analysis gaps — they
require either a real execution environment or a reference artifact this
repository does not have, and are stated as such rather than downgraded
to "probably fine."

## Scope of this document

Local, read-only analysis only, using `ct-ng-builder:with-toolchain-qemu`'s
own `arm-cortex_a15-linux-gnueabihf-readelf`/`objdump` against the two
already-present vendor binary copies. No file was installed or changed on
any device, no hardware or print test was started, and nothing was
pushed.
