# Reactor bridge ABI verification against the actual 0113 vendor binary

Scope: for every ABI assumption `reactor_module.cpp` makes about the
vendor binary — the 4 remaining open struct offsets, function
signatures, calling convention, and exception/unwind boundaries — this
records the assumption, the method used to check it, the result, and the
evidence. Per instruction, anything not conclusively resolved is marked
**BLOCKER**, not "probably compatible."

All checks below ran against the actual vendor binaries
(`old.elegoo_printer` = `19712b…`, `new.elegoo_printer` = `688b7bf8…`,
both hashes re-verified immediately before this session's checks).
Nothing was installed on any device, no hash constant or firmware pin
was changed, nothing was pushed.

## 1. The 4 remaining open struct offsets

### Offset `96` ("normal" callback vector) — **RESOLVED**

- **Assumption:** `at(owner, 96)` holds a `std::vector<Shared>` of
  "normal" (synchronous) registered callbacks, read by `take_callback`
  and `status_note`.
- **Method:** identified the vendor function that populates it —
  `SelectReactor::register_callback(...)` (old `0x26b9fc`, new
  `0x26df2c`, both size 200) — by searching the symbol table for a
  plausible registrar, then ran the same instruction-by-instruction
  comparison used in `INSTRUCTION_DIFF_0113.md`/`SUPPORT_FUNCTIONS_DIFF_0113.md`.
- **Result:** the function is instruction-identical between builds
  (only address-shift branch targets differ), **and** it contains
  `add r3, r3, #96` operating on the owner pointer — a genuine,
  confirmed object-field access, not a stack-frame offset.
- **Evidence:** `old@0x26ba54: add r3, r3, #96`, same instruction present
  unchanged in the new build's corresponding, address-shifted copy of
  the same function.

### Offset `108` ("async" callback vector) — **BLOCKER, not resolved**

- **Assumption:** `at(owner, 108)` holds a `std::vector<Shared>` of
  "async" registered callbacks, same role as offset 96 but for
  `register_async_callback`.
- **Method:** identified `SelectReactor::register_async_callback(...)`
  (old `0x26bbdc`, new `0x26e10c`, both size 388) as the most likely
  vendor function, by the same symbol-search method, and disassembled
  its full body in both builds.
- **Result:** the function **is** instruction-identical between builds
  (its 5 apparent differences were individually resolved below — none
  are real content differences), **but it does not contain a direct
  `#108` field write anywhere in its own body.** Instead, it constructs
  a `std::function<void()>` and pushes it onto a
  `std::queue<std::function<void()>>` (`_ZNSt5queueISt8functionIFvvEESt5dequeIS2_SaIS2_EEE4pushEOS2_`)
  for deferred execution — the actual write to offset `108` (if it
  happens at all under this name) would occur later, inside that
  queued lambda's own body (symbol
  `_ZZN13SelectReactor23register_async_callback...EUlvE_clEv`), which
  this check did not locate or compare.
- **Evidence the 5 apparent differences are benign** (resolved
  individually, not discarded): `"Error writing to pipe: "` (identical
  string, both builds); the pointer passed alongside it resolves, by
  exact symbol-table lookup, to `_ZSt4cerr@GLIBCXX_3.4` (`std::cerr`) in
  **both** binaries at their respective (expectedly different) data
  addresses; the third resolves to `_ZSt4endlIcSt11char_traitsIcEE...`
  (`std::endl`)'s PLT stub in **both** binaries. None of this is a
  content difference — it is ordinary `std::cerr << "..." << std::endl`
  error-logging code, and it tells us nothing about offset `108`.
- **Status: BLOCKER.** The function responsible for writing offset `108`
  has not been identified or compared. This offset's compatibility with
  the `0113` build is **not established**, one way or the other.

### Offset `20` (RB-tree node value) — **BLOCKER, not resolved**

- **Assumption:** for a node in the vendor's `std::map`/RB-tree-like
  structure reached via offset `248`, the mapped value (a `Shared`) sits
  at `node + 20` (per the comment in `reactor_module.cpp`: "node key +16,
  shared value +20").
- **Method:** attempted to find the vendor function that *inserts* into
  this tree (the counterpart to `handler()`'s read-only traversal), by
  searching the symbol table for plausible names
  (`SelectReactor`/`register`/`fd`/`descriptor`-related). No candidate
  insertion function was conclusively identified within this session's
  scope.
- **Result: not checked.** No instruction-level comparison was performed
  because the responsible vendor function was not identified.
- **Status: BLOCKER.**

### Offset `248` (`240+8`, RB-tree root pointer) — **BLOCKER, not resolved**

- **Assumption:** `at(owner, 248)` holds the root-node pointer of the
  same RB-tree-like `std::map` `handler()` traverses for fd→handler
  lookup.
- **Method:** same as offset `20` — searched for the insertion/mutation
  function; none conclusively identified.
- **Result: not checked.**
- **Status: BLOCKER.**

**Honest summary for §1:** 7 of 11 hardcoded offsets are now confirmed
(adding `96` to the 6 already confirmed in
`ABI_AND_RUNTIME_VERIFICATION_0113.md`: `144, 32, 16, 264, 8, 24, 96`).
3 remain open BLOCKERs (`108, 20, 248`) — not assumed compatible, not
downgraded to "probably fine." (This document also found and corrected,
for `108`, an unconfirmed guess from the prior entry's companion
analysis about *which* function to check — the guess was reasonable, but
checking it did not close the gate, and that is reported as such rather
than silently dropped.)

## 2. Function signatures of the vendor interfaces `reactor_module.cpp` calls

- **Assumption:** every `original<F>(address)` call's function-pointer
  type `F` in `reactor_module.cpp` matches the real vendor function's
  signature at that address, in both the old and the 0113 build.
- **Method:** C++ Itanium mangled names encode the complete function
  signature (argument types, return type via covariant-return rules,
  `const`/ref-qualifiers). `ADDRESS_MAPPING_0113.md` already matched 21
  of the 22 addresses (all but the `poll` PLT stub, which has no C++
  mangled name) by exact mangled-name equality between the old and 0113
  symbol tables — not merely by address.
- **Result:** since an exact mangled-name match is only possible when
  the full signature is identical, this *is* the signature check, not a
  separate one — already done, not assumed. No case was found where the
  mapped address's symbol name differs in its signature portion between
  builds.
- **Evidence:** the per-address table in `ADDRESS_MAPPING_0113.md`.

## 3. Calling convention

- **Assumption:** both the old and the 0113 vendor binaries use the ARM
  hard-float EABI calling convention (`-mfloat-abi=hard`), matching what
  `reactor_module.cpp` and its build flags assume.
- **Method:** read the ELF header flags directly from both actual vendor
  binaries with `llvm-readelf -h` (not inferred from build flags or from
  this session's own toolchain output).
- **Result:** **identical**, both binaries: `Flags: 0x5000400` = Version5
  EABI, hard-float ABI.
- **Evidence:**
  ```
  old.elegoo_printer: Flags: 0x5000400
  new.elegoo_printer: Flags: 0x5000400
  ```
  This confirms the *global* ABI flag matches. It does not, by itself,
  prove every individual function's parameter-passing matches in detail
  (e.g. struct-by-value splitting, the hidden result-pointer convention
  `cc2_wait` relies on for `nlohmann::json`) beyond what is already
  established in §2 via exact signature (mangled-name) matching.

## 4. Exception and unwind boundaries

- **Assumption:** a C++ exception thrown inside a vendor-compiled
  function (e.g. `SelectReactor::pause`, called by `cc2_wait` at
  `0x26c0c4`) unwinds correctly through stack frames that mix
  vendor-compiled code and this session's test-candidate-toolchain-compiled
  bridge code.
- **Method tried:** `ABI_AND_RUNTIME_VERIFICATION_0113.md`'s isolated
  `qemu-arm-static` self-test exercises exception propagation, but
  **entirely within code compiled by this session's own candidate
  toolchain** — the vendor side of every call is mocked out in
  `bridge_test.cpp`. This check cannot, by construction, say anything
  about unwinding across a real vendor/bridge compiler boundary, because
  no real vendor-compiled frame is ever on the stack during that test.
- **Result: not established, either way.**
- **Status: BLOCKER.** Whether the vendor binary was built with a
  compiler/unwind-table format compatible with what this session's
  candidate toolchain emits is unknown. The vendor's own build
  toolchain is undocumented anywhere in this repository (see
  `ARM_TEST_PLAN.md`'s "Toolchain match" gate, also still open). A
  compatible ARM EABI unwind-table *format* is likely (both are
  `.ARM.exidx`-based per EABI convention, confirmed present in the
  vendor binaries by the fingerprint/PLT checks already done), but
  "likely" is explicitly not treated as a pass here.

## Updated status

| Area | Status |
|---|---|
| Offset 96 | **CONFIRMED** |
| Offset 108 | **BLOCKER** — responsible function not identified |
| Offset 20 | **BLOCKER** — responsible function not identified |
| Offset 248 | **BLOCKER** — responsible function not identified |
| Function signatures | **CONFIRMED** (via exact mangled-name match, already established) |
| Calling convention (ELF-level ABI flag) | **CONFIRMED** on both real vendor binaries |
| Exception/unwind boundary (vendor ↔ bridge) | **BLOCKER** — not established either way |

This does not change any status in `ARM_TEST_PLAN.md`'s gate table other
than what is stated here. No hardware test is proposed or authorized by
this document; a minimal, reversible hardware-test proposal remains a
separate, not-yet-executed proposal.

## Scope of this document

Local, read-only analysis only. No hash gate was loosened, no file was
installed or changed on any device, no hardware or print test was
started, and nothing was pushed.
