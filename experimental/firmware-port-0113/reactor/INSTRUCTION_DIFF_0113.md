# Reactor hook function full instruction-level comparison, 07 → 0113

Scope: the four reactor hook functions (`ReactorCallback::invoke`,
`SelectReactor::check_timers`, `PollReactor::dispatch_loop`,
`ReactorCompletion::wait`), compared in full between the old
(02.01.01.07, `19712b…`) and the documented port target (02.01.01.13,
`688b7bf8…`) — not just the 8-byte entry sequence covered in
[`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md).

## What this confirms, and what it does not

**Confirms:** after normalizing call-target addresses and immediate
operands (see "Method" below), the full instruction sequence of all four
functions is identical between the 07 and 0113 builds. This is a static
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

## Method

1. Disassembled each function's full address range in both binaries with
   `llvm-objdump -d` (same LLVM 23.1.5 build used in
   `FINGERPRINT_VERIFICATION_0113.md`).
2. Normalized call-target addresses and hex immediates to a placeholder
   (`ADDR`), so that a uniform address shift (e.g. the `+0x2530` pattern
   from `ADDRESS_MAPPING_0113.md`) does not register as a difference by
   itself, while the mnemonic, register operands, and referenced symbol
   names are preserved as-is. Example rule set:

   ```sh
   sed -E \
     -e 's/#-0x[0-9a-f]+/#-ADDR/g' \
     -e 's/#0x[0-9a-f]+/#ADDR/g' \
     -e 's/0x[0-9a-f]+ </ADDR </g' \
     -e 's/@ imm = #-0x[0-9a-f]+/@ imm = #-ADDR/g' \
     -e 's/@ imm = #0x[0-9a-f]+/@ imm = #ADDR/g' \
     -e 's/\+0x[0-9a-f]+>/+ADDR>/g'
   ```

   **Validated, not assumed correct:** this exact rule set was checked to
   reproduce, byte-for-byte, the pre-existing
   `old_check_timers.norm.asm`/`new_check_timers.norm.asm` files already
   present in this port's analysis output (produced independently,
   earlier) from their corresponding plain `.asm` disassembly — i.e. it
   matches a normalization convention already used in this port, rather
   than being a new, unchecked method.
3. Stripped the leading address and raw-opcode-hex columns (which trivially
   differ because of the address shift) from both normalized listings.
4. Diffed the two resulting texts per function.

## Result

| Function | Diff after normalization |
|---|---|
| `ReactorCallback::invoke` | Identical (only the trivial filename header and the function's own address label differ) |
| `SelectReactor::check_timers` | Identical (same) |
| `PollReactor::dispatch_loop` | Identical (same) |
| `ReactorCompletion::wait` | Identical (same) |

Every mnemonic, register operand, and referenced symbol name — including
every internal branch target's resolved symbol name — matches line for
line, for the full 308/820/1400/600-byte bodies, not only the leading 8
bytes.

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

Because the four functions' compiled bodies are byte-identical between
builds, any constant struct-offset encoded directly in *their own*
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
  functions, post call-target/immediate normalization).
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
