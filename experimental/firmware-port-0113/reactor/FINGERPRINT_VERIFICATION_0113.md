# Reactor hook-site fingerprint check — methodology correction and result

## Correction to a prior, now-retracted claim

An earlier analysis pass reported all four reactor hook-site fingerprints
as mismatching between the old (02.01.01.07) and a new binary, and treated
that as a failed gate. **That claim is retracted.** Its extraction script
read `data[addr:addr + 8]` using `addr` as a raw file offset, but `addr`
was a virtual address (`p_vaddr`), not a file offset (`p_offset`) — ELF
distinguishes the two, and a virtual address must be translated through
its containing `PT_LOAD` segment before indexing into the file:

```text
file_offset = p_offset + (virtual_address - p_vaddr)
```

The bytes the earlier pass read were file-offset bytes that happened to
share the same numeric value as the virtual addresses — not the
instruction bytes actually at those virtual addresses. No conclusion from
that pass ("four fingerprints differ", "different compiler prologues",
"fingerprint gate FAIL") is supported by that extraction and none of it is
carried into [`ARM_TEST_PLAN.md`](ARM_TEST_PLAN.md) or
[`ADDRESS_MAPPING_0113.md`](ADDRESS_MAPPING_0113.md).

## Corrected extraction and result

Re-extracted with the virtual address correctly translated through the
matching executable `PT_LOAD` segment, cross-checked with `llvm-objdump`
disassembly at each address. Both binaries' hashes were re-verified
immediately before this extraction ran.

**Reproducibility record:**

- Date: 2026-11-09.
- Tooling: `pyelftools` 0.33; Homebrew LLVM 23.1.5
  (`llvm-objdump`/`llvm-readelf` at `/opt/homebrew/opt/llvm/bin/`).
- Extraction code (run as `python3 extract_fingerprint.py <binary>` against
  each of `old.elegoo_printer` and `new.elegoo_printer`):

  ```python
  from elftools.elf.elffile import ELFFile

  def va_to_offset(elf, va):
      for seg in elf.iter_segments():
          if seg['p_type'] != 'PT_LOAD':
              continue
          p_vaddr, p_filesz, p_offset = seg['p_vaddr'], seg['p_filesz'], seg['p_offset']
          if p_vaddr <= va < p_vaddr + p_filesz:
              return p_offset + (va - p_vaddr), seg
      return None, None

  # For each hook VA: open the binary, resolve its executable PT_LOAD
  # segment (the loop above only accepts a segment that actually contains
  # the VA within p_filesz, i.e. a file-backed, non-bss range), compute
  # file_offset, then read exactly 8 bytes: data[file_offset:file_offset+8].
  # A VA with no containing PT_LOAD segment (not executable/not file-backed)
  # is reported as unresolved rather than silently read from a guessed offset.
  ```

  This differs from the retracted script (above) specifically in resolving
  `file_offset` through the matching `PT_LOAD` segment first, and in only
  accepting a segment where the full 8-byte read falls inside `p_filesz`
  (i.e. is backed by file content, not a `.bss`-style zero-fill region)
  before reading any byte.

| Role | Binary | SHA-256 | Confirmed present at |
|---|---|---|---|
| Old reference | `elegoo_printer` | `19712b5943b10bb8cb6aab9a4ed498ee962b28538b23c91b7ed003eb9e590745` | `~/reactor-analysis/old.elegoo_printer` |
| Local comparison copy | `elegoo_printer` | `f0111e0b52c2ea39ff2aae70675c53b311f7340df8ed233e16e8e0cc9f883e14` | (separate local analysis path, see below) |
| **Documented port target** | `elegoo_printer` | `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` | `~/reactor-analysis/new.elegoo_printer` |

Result, run and recorded separately for each new binary against the old
reference, at the mapped addresses from `ADDRESS_MAPPING_0113.md`:

| Function | New VA | New file offset | Bytes (`f011…`) | Bytes (`688b…`, documented target) | Matches old (`fingerprints.h`) |
|---|---|---|---|---|---|
| `ReactorCallback::invoke` | `0x26d08c` | `0x25d08c` | `30482de90cb08de2` | `30482de90cb08de2` | yes (both) |
| `SelectReactor::check_timers` | `0x26f1dc` | `0x25f1dc` | `70482de910b08de2` | `70482de910b08de2` | yes (both) |
| `PollReactor::dispatch_loop` | `0x2708d8` | `0x2608d8` | `10482de908b08de2` | `10482de908b08de2` | yes (both) |
| `ReactorCompletion::wait` | `0x26cb70` | `0x25cb70` | `10482de908b08de2` | `10482de908b08de2` | yes (both) |

**`poll` PLT/relocation proof, repeated directly against `688b…` (not
transferred from the `f011…` run):**

```text
llvm-objdump -d --start-address=0xbf5fc --stop-address=0xbf65c new.elegoo_printer
  000bf628 <poll@plt>:
     bf628: e28fc60c  add  r12, pc, #12, #12
     bf62c: e28cca16  add  r12, r12, #90112
     bf630: e5bcf208  ldr  pc, [r12, #0x208]!
  (neighbor PLT entries, same order as the old binary:
   basic_string::rfind, zipCloseFileInZip, MQTTAsync_setCallbacks,
   allocator<char>::ctor, poll, send, hv_isdir, __throw_logic_error, basename)

llvm-readelf -rW new.elegoo_printer | grep poll
  00cd5838  000bb816 R_ARM_JUMP_SLOT  00000000  poll

llvm-readelf -SW new.elegoo_printer | grep -i got
  [22] .got  PROGBITS  00cd5388  cc5388  000944  04  WA  0  0  4
```

`poll@plt` resolves at `0xbf628` with an `R_ARM_JUMP_SLOT` relocation for
`poll` at GOT slot `0xcd5838` (`.got` base `0xcd5388`) — confirmed directly
against the documented target binary, independent of the earlier `f011…`
run (which found the identical GOT/`.got` values, for reference, but is no
longer the basis for this conclusion).

**What this shows:** for the documented port target (`688b…`), a correct
byte-level comparison at the four mapped hook sites found no difference in
the leading 8 bytes, and the `poll` PLT mapping is independently confirmed
at the new address. This closes the fingerprint-byte and PLT-target gates
in `ARM_TEST_PLAN.md` §2 **for the 8-byte entry sequence and the PLT target
only**. It is **not**:
- evidence that the full function bodies (beyond the first 8 bytes) are
  unchanged — the still-outstanding "instruction-level re-diff" item in
  `ARM_TEST_PLAN.md` §2 is not addressed by this;
- evidence of ABI compatibility (struct layout, calling convention,
  exception semantics) — those gates remain separately open;
- a hardware/runtime qualification result, an integrated port, or a basis
  for any hash-gate change.

## Evidence-boundary note: `f011…` vs. `688b…`

The earlier, separately-run comparison against `f011…` and this
comparison against `688b…` reached the same byte-level result (all four
fingerprints match, same PLT/GOT layout for `poll`). That is useful
corroborating evidence that both local binaries agree with each other and
with the old reference at these specific bytes — it does **not** establish
that `f011…` and `688b…` are otherwise identical, and no claim here relies
on treating them as interchangeable. Each result above is labeled by which
binary it was measured against.

The earlier draft of this document stated that `688b…` "was not found in
the paths searched so far." That statement is retracted: the file is
present at `~/reactor-analysis/new.elegoo_printer` on this machine
(hostname `MacBook-Pro-von-Manuel.local`), its hash was re-confirmed
immediately before this extraction ran, and the extraction above was run
directly against it.

## Status for `ARM_TEST_PLAN.md` §2 gates

- "Hook-site instruction fingerprints" and "PLT target proof": the 8-byte
  entry sequence and the `poll` PLT target are now confirmed, by direct
  measurement, against the documented port target `688b…`. Everything
  these two gates do not cover (full function bodies, ABI, calling
  convention, exceptions, toolchain, hardware) is unchanged.
- All other `ARM_TEST_PLAN.md` §2 gates (struct/ABI layout,
  calling-convention, exception-boundary re-verification, toolchain match,
  and the `dispatch_loop`/`wait` instruction-level re-diff) remain **NOT
  RUN**, exactly as before.

## Open items carried forward

1. ~~Full instruction-level body comparison~~ — done, see
   [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md): every differing
   instruction was individually resolved (no blanket normalization); all
   four hook functions are instruction- and control-flow-identical between
   the 07 and 0113 builds except one explained, behaviorally-inert
   embedded `__FILE__` build-path string. That document also corrects an
   earlier conflation of this with the existing manifest's unrelated
   `instructions_equal_except_call_targets: false` field.
2. Struct/ABI layout, calling-convention, and exception-boundary
   re-verification — none of this follows from matching entry bytes or
   from the full-body instruction match in `INSTRUCTION_DIFF_0113.md`.
3. Exact pinned toolchain (GCC 6.5.0 / glibc 2.23 /
   `arm-cortex_a15-linux-gnueabihf`) confirmed **not available** on this
   machine as of 2026-11-09 — see `INSTRUCTION_DIFF_0113.md`.
4. No ARM hardware/runtime qualification of the 02.01.01.13 reactor port
   was performed as part of this verification.

## Scope of this document

Local, read-only analysis and documentation only. No hash gate was
loosened, no file was installed or changed on any device, and no
hardware or print test was started.
