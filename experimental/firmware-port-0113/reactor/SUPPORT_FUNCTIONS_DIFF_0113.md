# Reactor bridge support-function instruction comparison, 07 → 0113

Scope: the 18 "support" vendor addresses `reactor_module.cpp` calls into
via `original<F>(address)(...)` — `shared_ptr`/`__shared_count`
ctors/dtors, `nlohmann::json` ctors/dtors/accessors, `_Function_base`,
`get_monotonic`, and the `poll` PLT stub — compared between the
02.01.01.07 (`19712b…`) and the documented 0113 target (`688b7bf8…`)
builds. This extends
[`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md), which covered only
the 4 patch-target functions (`invoke`/`check_timers`/`dispatch_loop`/`wait`).

## What this confirms, and what it does not

**Confirms:** every one of these 18 support addresses' code is
instruction- and control-flow-identical between the two builds (modulo
the address-shift and one explained exception each, below) — the same
functions the bridge's `Shared`/`shared_ptr` helper and JSON handling
directly depend on. Combined with `INSTRUCTION_DIFF_0113.md`'s result for
the 4 patch targets, **all 22 addresses `reactor_module.cpp` hardcodes are
now individually checked**, not just the ones it patches.

**Does not confirm:** ABI compatibility of `reactor_module.cpp`'s own
compiled assumptions (struct layouts, calling convention) with whatever
toolchain built the 0113 vendor binary — this remains a property of the
*vendor's* build, which this comparison does not control for. It also
does not confirm runtime behavior.

## Method

Same as `INSTRUCTION_DIFF_0113.md`: full disassembly over each function's
actual size (from each binary's own symbol table, or — where no sized
`FUNC` symbol exists — a window bounded by the next real symbol, confirmed
by inspection, not assumed), paired instruction-by-instruction, with every
difference individually resolved rather than blanket-normalized. A
classifier script accepts only a branch/call target whose disassembled
`<symbol>` annotation matches the same name on both sides as an
address-shift artifact; everything else is listed and explained below.

## Result

| Old VA | New VA | Symbol | Result |
|---|---|---|---|
| `0x109e4c` | `0x10c37c` | `...basic_json...is_null` | identical except branch-target address-shift |
| `0x1d62b4` | `0x1d87e4` | `std::function<void(double)>::operator()` | identical except branch-target address-shift |
| `0x26a4ec` | `0x26ca1c` | `ReactorCompletion::complete` | identical except branch-target address-shift |
| `0x26b8b8` | `0x26dde8` | `SelectReactor::unregister_timer` | identical except branch-target address-shift |
| `0x26c0c4` | `0x26e5f4` | `SelectReactor::pause` | identical except branch-target address-shift and one explained diagnostic-string difference (see below) |
| `0x26d3a0` | `0x26f8d0` | `SelectReactor::end_greenlet` | identical except branch-target address-shift |
| `0x26dddc` | `0x27030c` | `SelectReactor::getcurrent` | identical except branch-target address-shift |
| `0x271c90` | `0x2741c0` | `shared_ptr<ReactorGreenlet>::operator=` | identical except branch-target address-shift |
| `0x2720d4` | `0x274604` | `vector<shared_ptr<ReactorGreenlet>>::push_back` | identical except branch-target address-shift |
| `0x272350` | `0x274880` | `std::function<json(...)>::operator()` | identical except branch-target address-shift |
| `0x2738bc` | `0x275dec` | `std::function<double(double)>::operator()` | identical except branch-target address-shift |
| `0x35ee3c` | `0x35cbd0` | `get_monotonic` | **one extra `nop` in the new build — explained below, not a semantic difference** |
| `0xbf70c` | `0xbf628` | `poll@plt` | PLT stub: GOT displacement differs as expected for a different PLT table position — already independently confirmed via relocation/GOT inspection in `FINGERPRINT_VERIFICATION_0113.md`, not a raw-byte comparison here |
| `0xc3ee8` | `0xc6418` | `_Function_base::_M_empty` | **byte-identical**, no difference at all |
| `0xcfd10` | `0xd2240` | `shared_ptr<ReactorTimer>::~shared_ptr` | identical except branch-target address-shift |
| `0xd0460` | `0xd2990` | `shared_ptr<ReactorTimer>::shared_ptr(const&)` | identical except branch-target address-shift |
| `0xd1194` | `0xd36c4` | `...basic_json...::~data` | identical except branch-target address-shift |
| `0xdb05c` | `0xdd58c` | `basic_json::basic_json(const&)` | identical — see "false-positive" note below |

## `get_monotonic`: one explained, benign difference

The new build has one extra `nop` instruction (at `0x35cc24`) immediately
before the function's literal pool, which simply shifts the pool's
`[pc, #N]` displacement by 4 (`#36`→`#40` for the `vldr d6` load) and
shifts the backward/forward direction of the `bl report_errno` call
(same callee by symbol name in both). The literal-pool double constant
itself (the nanosecond-to-second conversion factor) is unaffected. This
is ordinary compiler alignment padding (likely for 8-byte alignment of
the double literal), not a logic change — confirmed by reading both full
disassemblies side by side, not inferred.

## `basic_json::basic_json(const&)`: a false positive in this session's own tooling, not a binary difference

The classifier script initially reported 8 "mnemonic differs" lines in
this function. Inspection of the raw disassembly showed the actual
content is a `cmp`/`ldrls pc,[pc,r3,lsl #2]` **jump table** (the ARM
idiom for a `switch` over the JSON value-type tag), followed by six
`.word` entries holding **absolute addresses of jump targets inside this
same function**. The script's line parser expected one 8-hex-digit
opcode field per instruction line and mis-split the four
space-separated raw bytes of each `.word` line (e.g. `ec b0 0d 00`),
treating a fragment of the byte dump as if it were a mnemonic. This is a
defect in this session's own comparison script, not a finding about the
binaries. Read correctly, the jump-table target addresses shift by
exactly the same uniform `+0x2530` offset as every other address in this
region (e.g. `0x000db0ec` → `0x000dd61c`), consistent with everything
else in this document.

## `SelectReactor::pause`: resolved diagnostic-string differences

Like `dispatch_loop`/`wait` in `INSTRUCTION_DIFF_0113.md`, this function's
`movw`/`movt` pairs construct pointers for `spdlog::source_loc`
diagnostic-logging calls. Every one was individually resolved by reading
the actual bytes at the constructed address in both binaries:

| Constant | Old | New | Verdict |
|---|---|---|---|
| Function-name string (×2 occurrences) | `"pause"` | `"pause"` | identical |
| `__FILE__` build path | (same path string already documented in `INSTRUCTION_DIFF_0113.md`) | (same) | **differs**, same already-explained reason (different build machine/path, diagnostic-only, no behavioral role) |
| Assertion/separator string | `"g == nullptr"` | `"g == nullptr"` | identical |
| Format string | `"__func__:{},waketime:{}"` | `"__func__:{},waketime:{}"` | identical |

## Updated scope statement

Between this document and `INSTRUCTION_DIFF_0113.md`, all 22 addresses
`experimental/firmware-port/reactor/reactor_module.cpp` hardcodes
(4 patch targets + 18 support calls) have now been individually
instruction-compared between the 02.01.01.07 and 0113 (`688b7bf8…`)
builds. The only genuine content differences found across all 22 are the
diagnostic-only `__FILE__` build-path strings (zero behavioral role) and
one benign alignment `nop`. This remains, as stated in
`INSTRUCTION_DIFF_0113.md`, a narrow, indirect data point about these
specific functions' own compiled layout — not a general ABI confirmation,
not a runtime check, and not a basis for any hash-gate change.

## Scope of this document

Local, read-only analysis and documentation only. No hash gate was
loosened, no file was installed or changed on any device, and no
hardware or print test was started.
