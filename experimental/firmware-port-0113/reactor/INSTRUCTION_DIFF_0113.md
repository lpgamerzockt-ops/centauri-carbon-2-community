# Reactor hook function full instruction-level comparison, 07 → 0113

Scope: the four reactor hook functions (`ReactorCallback::invoke`,
`SelectReactor::check_timers`, `PollReactor::dispatch_loop`,
`ReactorCompletion::wait`), compared in full between the old
(02.01.01.07, `19712b…`) and the documented port target (02.01.01.13,
`688b7bf8…`) — not just the 8-byte entry sequence covered in
[`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md).

## What this confirms, and what it does not

**Confirms:** after individually resolving every differing instruction
(see "Method" below — not a blanket normalization), all four functions are
instruction- and control-flow-identical between the 07 and 0113 builds,
with exactly one explained, behaviorally-inert content difference (an
embedded `__FILE__` build path in a diagnostic log call). This is a static
comparison of these four functions' own compiled bodies only.

**Does not confirm:**
- ABI compatibility of the separately-built `reactor_module.cpp` bridge
  (`cc2_callback`/`cc2_timers`/`cc2_poll`/`cc2_wait`) with the actual
  0113 vendor ABI. That bridge is compiled with a different, separately
  pinned cross-toolchain and injected via `LD_PRELOAD`; this document says
  nothing about it directly.
- `shared_ptr`-ABI, RB-tree layout, calling convention, or C++
  exception-boundary compatibility as general properties of the 0113
  build. See "What this suggests, cautiously" below for the one narrow,
  indirect inference this result does support — and its limits.
- Any hardware or runtime behavior.

## Function sizes (measured, not assumed)

Taken directly from each binary's own symbol table (`llvm-readelf -sW`),
not carried over from the existing `experimental/firmware-port/reactor/port-manifest.json`:

| Function | Size (both builds) |
|---|---|
| `ReactorCallback::invoke` | 308 bytes |
| `SelectReactor::check_timers` | 820 bytes |
| `PollReactor::dispatch_loop` | 1400 bytes |
| `ReactorCompletion::wait` | 600 bytes |

Both binaries report the same size for each function — itself a
precondition for the instruction-level diff below, measured against
`688b7bf8…` (the documented target) specifically, not assumed from the
07-only manifest.

## Method (corrected — no blanket hex normalization)

An earlier version of this document normalized *every* hex immediate in
both disassembly listings to a placeholder before diffing. That was too
coarse: it would have silently discarded a genuine difference in any
non-branch instruction's immediate operand (a struct offset, a constant,
or a data-pointer construction), not only the address-shift artifacts it
was meant to hide. **That normalization is retracted** and replaced with
the method below, which classifies every single differing line instead of
discarding it.

1. Disassembled each function's full address range in both binaries with
   `llvm-objdump -d` (same LLVM 23.1.5 build used in
   `FINGERPRINT_VERIFICATION_0113.md`), with no normalization applied.
2. Paired up the two binaries' instruction streams 1:1 by position (valid
   here because both report the same instruction count/size for every
   function — see "Function sizes" above).
3. For each instruction pair, compared the mnemonic and full operand text
   verbatim. A pair is left as a flagged difference unless it is a
   branch/call instruction (`bl`, `b`, `beq`, …) whose target resolves,
   via `llvm-objdump`'s own `<symbol>` annotation, to the **same symbol
   name** on both sides — that specific case is the one address-shift
   artifact this method accepts automatically, because the symbol
   identity (not the raw address) is what matters for whether the two
   bodies call the same thing.
4. Every other differing instruction (any non-branch instruction with a
   different hex literal) is resolved individually: for a `movw`/`movt`
   pair constructing an absolute address, the actual bytes at that address
   in each binary are read and compared directly (see "Resolved
   differences" below) — not inferred from the address alone.

## Result

| Function | Branch targets (symbol-identical) | Instructions identical as-is | Other differing instructions |
|---|---|---|---|
| `ReactorCallback::invoke` | 19 | 58 | 0 |
| `SelectReactor::check_timers` | 42 | 163 | 0 |
| `PollReactor::dispatch_loop` | 75 | 256 | 19 |
| `ReactorCompletion::wait` | 35 | 109 | 6 |

`invoke` and `check_timers` have **no** differing instructions beyond
symbol-identical branch targets — every other instruction, including all
register operands and non-branch immediates, is identical as-is (no
normalization needed or applied).

`dispatch_loop` and `wait` each contain one or more `movw`/`movt` pairs
that construct an absolute address of a string or data constant (used by
an `spdlog::source_loc` diagnostic-logging call); these account for all
19 (`dispatch_loop`, in 3 repeated call sites of 6 instructions each +
one PC-relative literal-pool load) and 6 (`wait`, one call site)
"other differing instructions". Each is resolved below, individually, by
reading the actual bytes the constructed address points to in both
binaries — not discarded as presumed address noise.

## Resolved differences (every one, not sampled)

| What the address points to | Old content | New content | Verdict |
|---|---|---|---|
| Function-name string (`dispatch_loop` call sites) | `"dispatch_loop"` | `"dispatch_loop"` | identical |
| `__FILE__` build path (`dispatch_loop` call sites) | `/home/elegoo/workspace/FD0304/elegoo_printer/elegoo/reactor.cpp` | `/home/muge/eeb001/elegoo/reactor.cpp` | **differs** — see below |
| Separator string (`dispatch_loop` call sites) | `"!!!!!!!!!!!!!!!!!!!"` (19 `!`) | `"!!!!!!!!!!!!!!!!!!!"` (19 `!`) | identical |
| Function-name string (`wait` call site) | `"wait"` | `"wait"` | identical |
| `__FILE__` build path (`wait` call site) | same as above | same as above | **differs**, same reason |
| Separator string (`wait` call site) | `"!!!!!!!!!!!!!!!!!!!!!"` (21 `!`) | `"!!!!!!!!!!!!!!!!!!!!!"` (21 `!`) | identical |
| PC-relative literal-pool double (`dispatch_loop`, one `vldr`) | `1000.0` | `1000.0` | identical (same function, shifted with it) |

**The one genuine content difference found:** the compiler-embedded
`__FILE__` path differs because the two binaries were built on different
build machines/paths (`elegoo`/`workspace/FD0304` vs. `muge`/`eeb001`).
This string is passed only to a diagnostic logging call
(`spdlog::source_loc`) — it is not used in any control-flow, pointer
arithmetic, or struct-layout decision in either function. The accompanying
line-number argument to the same call (`#0x309` = 777) is identical on
both sides, which is itself corroborating evidence this is the same
source line in both builds, just compiled from a tree checked out at a
different absolute path.

## Corrected conclusion

**Not "byte-identical."** All four functions are **instruction- and
control-flow-identical**, including every branch target (by symbol
identity) and every non-branch operand except the diagnostic-only
`__FILE__` string, whose difference is fully explained and has no bearing
on runtime behavior, struct layout, or ABI. This is a stronger, narrower,
and individually-justified claim than the earlier, retracted "identical
after normalization" statement.

## Correction: the existing manifest's `instructions_equal_except_call_targets: false` does not apply here

An earlier note in this directory (`ARM_TEST_PLAN.md`,
`FINGERPRINT_VERIFICATION_0113.md`) flagged that the pre-existing
`experimental/firmware-port/reactor/port-manifest.json` already marks
`PollReactor::dispatch_loop` and `ReactorCompletion::wait` as
`"instructions_equal_except_call_targets": false`, and treated that as a
reason for extra caution about these two functions specifically. That
reference conflated two different comparisons:

- The existing manifest's `old_va`/`new_va` pair for these entries (e.g.
  `dispatch_loop`: `old_va: 0x46d520`, `new_va: 0x26e3a8`) compares the
  **original stock-vendor implementation** against the **community
  wholesale-replacement implementation**, both **within the same
  02.01.01.07 build** — i.e. it answers "does the community's
  replacement function behave like the stock one it replaces", not
  "does the 02.01.01.07 build's function match the 02.01.01.13 build's
  function". Its `false` result for these two functions says the
  community replacement is not instruction-identical to the stock
  function it replaces (expected — it is a deliberate rewrite), and says
  nothing about cross-base (07 vs. 0113) consistency.
- This document's comparison is 07-community-function vs.
  0113-community-function — a different pair entirely, and the one
  actually relevant to this port. It is **not** contradicted by the
  manifest's `false` value; that value simply measures something else.

This correction applies to this document only; it does not retract any
measured result elsewhere in this directory.

## What this suggests, cautiously

Because the four functions' compiled bodies are instruction-identical
between builds (every operand individually resolved, not normalized
away), any constant struct-offset encoded directly in *their own*
instructions (e.g. a `vldr`/`ldr` with a fixed field offset) is also
identical between builds — which is a narrow, indirect data point
suggesting the specific struct layouts *these four functions themselves
read* have not shifted. This is **not** a general ABI confirmation: it
says nothing about the `reactor_module.cpp` bridge's own compiled
assumptions (`Shared`/`shared_ptr` helper, RB-tree traversal, the hidden
result pointer `cc2_wait` relies on for its `nlohmann::json` return),
which are compiled separately, with a different toolchain, and not
exercised by this comparison at all.

## Updated gate status

- **Mapping re-verification:** PASSED — `verify_mapping.py` against
  freshly regenerated symbol tables reports `OK: 21  CHECK: 0  MISSING: 1`,
  matching `ADDRESS_MAPPING_0113.md` exactly.
- **Instruction-level re-diff (`dispatch_loop`/`wait` and the other two):**
  PASSED for the scope defined above (full-body comparison of these four
  functions, every differing instruction individually resolved — one
  explained, behaviorally-inert content difference found, no
  normalization used).
- **Toolchain match:** still **NOT AVAILABLE / BLOCKED** — no matching
  cross-compiler, no installed pinned toolchain, and no running container
  with the required build tools on this machine.
- **ABI / calling-convention / exception-boundary re-verification:** still
  **NOT RUN**. This document does not change that status. These gates
  close only once the pinned toolchain is available, the `reactor_module.cpp`
  bridge is built against it, and that build is checked against the actual
  0113 vendor binary's structures — not from this static comparison alone.
- **Hardware / runtime:** still **NOT RUN**.

## Scope of this document

Local, read-only analysis and documentation only. No hash gate was
loosened, no file was installed or changed on any device, and no
hardware or print test was started.
