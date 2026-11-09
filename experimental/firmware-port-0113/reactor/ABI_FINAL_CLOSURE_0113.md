# Closing the remaining ABI gates: offsets 20/248 and the exception boundary

Follow-up to `ABI_BLOCKER_RESOLUTION_2_0113.md`. That entry upgraded
offsets `20`/`248` to "indirectly well-supported via stable libstdc++
ABI" but explicitly stopped short of a direct disassembly confirmation,
and the exception/unwind boundary was only "narrowed" (same personality
routine used *somewhere* in both binaries). This entry closes both,
with direct disassembly evidence.

## Offsets `20` and `248`: now CONFIRMED by direct disassembly

**Key insight:** the actual RB-tree insertion/rebalancing logic
(`_Rb_tree_insert_and_rebalance`) is an **external, dynamically-linked
`libstdc++.so` function** (symbol table entry: `UND`, undefined) — it is
not compiled into `elegoo_printer` at all, so there is nothing
base-specific to compare for the rebalancing logic itself. What *is*
compiled into `elegoo_printer`, and what this session located and
compared, are three trivial libstdc++ accessor functions instantiated
for this exact map type, each small enough to show the raw offset
directly:

| Function | Old VA | New VA | Result |
|---|---|---|---|
| `_Rb_tree::_M_begin()` | `0x276a80` | `0x278fb0` | **byte-for-byte identical**, 10/10 instructions, no difference at all (not even an address-shift — it's a leaf function) |
| `_Rb_tree::_S_value(node)` | `0x27bd6c` | `0x27e29c` | identical except one address-shift branch target |
| `_Rb_tree_node::_M_valptr()` | `0x27db04` | `0x280034` | identical except one address-shift branch target |

**Disassembly evidence:**
```
_M_begin() (both binaries, byte-identical):
  str  r11, [sp, #-4]!
  ...
  ldr  r3, [r11, #-8]   ; r3 = this (the _Rb_tree, at owner+240)
  ldr  r3, [r3, #0x8]   ; <-- root pointer at (owner+240)+8 = owner+248
  mov  r0, r3
  bx   lr

_M_valptr() (both binaries, identical except the inlined call's address shift):
  ldr  r3, [r11, #-8]   ; r3 = this (the node)
  add  r3, r3, #16      ; <-- value/pair storage at node+16
  bl   __aligned_membuf::_M_ptr()
```

- **Offset `248`** (RB-tree root, relative to `owner`): `_M_begin()`
  directly dereferences `this + 8`, where `this` is the `_Rb_tree`
  object starting at `owner + 240` (confirmed in
  `ABI_BLOCKER_RESOLUTION_2_0113.md`) — so `owner + 240 + 8 = owner +
  248`. **CONFIRMED**, byte-identical between builds.
- **Offset `20`** (node-relative `Shared` value): `_M_valptr()` places
  the `pair<int, shared_ptr<ReactorFileHandler>>` at `node + 16`.
  Within that pair, the `int` key occupies bytes `[16, 20)` and the
  `shared_ptr` value — matching `reactor_module.cpp`'s own comment
  ("node key +16, shared value +20") — starts at `node + 20`.
  **CONFIRMED**, identical between builds (only the expected
  address-shift in the one nested call).

**All 11 of 11** hardcoded struct offsets `reactor_module.cpp` relies on
(`144, 32, 16, 264, 8, 24, 96, 108, 240, 248, 20`) are now confirmed by
direct disassembly against the actual `688b7bf8…` target, not assumed.

## Exception/unwind boundary: strengthened with a function-specific check

`ABI_VENDOR_INTERFACE_VERIFICATION_0113.md` showed *some* relocation in
each binary resolves to `__gxx_personality_v0`. This entry checked the
**specific function the bridge actually calls through**,
`SelectReactor::pause` (invoked from `cc2_wait` at `0x26e5f4`/new VA):

```
old.elegoo_printer --unwind, entry for SelectReactor::pause:
  FunctionAddress: 0x26C0C4
  ExceptionHandlingTable: .ARM.extab
  Model: Generic
  PersonalityRoutineAddress: 0xBFD3C  -> disassembles to <__gxx_personality_v0@plt>

new.elegoo_printer --unwind, entry for SelectReactor::pause:
  FunctionAddress: 0x26E5F4
  ExceptionHandlingTable: .ARM.extab
  Model: Generic
  PersonalityRoutineAddress: 0xBFC64  -> disassembles to <__gxx_personality_v0@plt>
```

Both builds give this specific function a valid, non-`CantUnwind`
unwind-index entry, using the `Generic` model (full `.ARM.extab` table,
not the abbreviated `Compact` form), and the personality routine it
points to resolves, by direct disassembly, to `__gxx_personality_v0` in
both. This is a function-specific confirmation, not just "the symbol
appears somewhere in the binary."

**Still not established, stated plainly:** the exact byte content of the
`.ARM.extab` LSDA entry for this function (its type_info/catch/cleanup
table encoding) was not compared byte-for-byte between builds, and no
C++ exception was ever actually thrown *through* a real vendor-compiled
frame under test (the isolated `qemu-arm-static` self-test only
exercises this session's own candidate-toolchain-compiled bridge code,
vendor entirely mocked). The mechanism-level risk (a mismatched or SJLJ
unwind scheme) is now ruled out with real, function-specific evidence;
the LSDA-byte-level and actual-live-throw risks are not eliminated by
static analysis alone and would need a real device/runtime test to
close completely.

## Updated status

| Gate | Status |
|---|---|
| All 11 struct offsets | **CONFIRMED** by direct disassembly |
| Exception/unwind mechanism for the actual called function | **CONFIRMED**: valid `Generic`/`.ARM.extab` entry, `__gxx_personality_v0`, in both builds, for `SelectReactor::pause` specifically |
| Exception/unwind LSDA byte-level encoding | Not compared — would require a dedicated `.ARM.extab` diff, not done here |
| Live exception through a real vendor frame | Not tested — requires real hardware or a non-mocked harness |

This is the strongest static ABI evidence this session has produced. It
does **not** constitute a hardware test, does not touch any device, and
does not authorize any hash-gate change or installation.

## Scope of this document

Local, read-only analysis only. No hash gate was loosened, no file was
installed or changed on any device, no hardware or print test was
started, and nothing was pushed.
