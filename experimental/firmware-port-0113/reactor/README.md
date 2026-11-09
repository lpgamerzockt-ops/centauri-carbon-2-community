# Reactor address mapping to 02.01.01.13 — experimental, not qualified

Scoped work-in-progress for Task A in
[`../REACTOR_CAMERA_PORT_TASKS.md`](../REACTOR_CAMERA_PORT_TASKS.md#task-a--reactor-port-to-020101113).
This directory carries only the **address-mapping evidence** produced so
far — symbol names, old/new virtual addresses, hashes, and the tooling used
to derive them. It does **not** contain:

- the stock vendor binaries (`elegoo_printer`, `libcolib.so`) — only their
  SHA-256 hashes, per [`../HANDOFF_PACKAGE_CHECKLIST.md`](../HANDOFF_PACKAGE_CHECKLIST.md#3-what-not-to-include);
- any compiled `.so` (`libcc2-reactor-v2.so` or similar) — a build artifact,
  not source, and not reproducible from what is checked in here;
- any change to `VENDOR_SHA256`/`EXPECTED` in
  `builder/current/core/reactor_component.py` or
  `experimental/firmware-port/launcher.sh` — those pins still gate on the
  qualified 02.01.01.07 Community V4.4.3 build and are **not** changed by
  this commit;
- raw disassembly listings or opcode bytes — the mapping below was derived
  from symbol-table name matching (`llvm-readelf -sW`), not from attaching
  binary disassembly, so only addresses/offsets/hashes/explanations are
  included.

## Status: experimental, unqualified — no print clearance

- The 22-address mapping (below) is a necessary input for the reactor port,
  **not a finished or verified port**. It only establishes that the same
  mangled symbol exists at a shifted address in the new binary; it does not
  prove the reactor's hooked callbacks are ABI-correct against the new
  build (struct-layout/`shared_ptr`-ABI re-verification from
  `REACTOR_CAMERA_PORT_TASKS.md` item 3 is still outstanding), and no real
  ARM execution test of `reactor_module.cpp` against the 02.01.01.13 binary
  has been run from this repository state.
- A host-side bridge/self-test binary was reportedly run and passed in a
  separate working session; that result has not been independently
  reproduced or verified here and does not substitute for ABI
  re-verification or on-device testing.
- Current on-device reactor load status for 02.01.01.13 is unresolved as of
  this commit — see [`../RUNTIME_FINDINGS_20261108.md`](../RUNTIME_FINDINGS_20261108.md)
  and [`../RUNTIME_FINDINGS_20261109.md`](../RUNTIME_FINDINGS_20261109.md),
  which both found the reactor hook in native fallback against `688b7bf8…`.
- Any statement about Canvas timing behavior or reactor-independent
  operation under this mapping is a **hypothesis**, not a measured result.
- **No "verified" tag applies to this mapping, and it does not clear the
  reactor for use on a printer with an active print job.**

## Contents

- [`ADDRESS_MAPPING_0113.md`](ADDRESS_MAPPING_0113.md) — the 22 old→new
  virtual-address pairs, the symbol name each was matched on, and the
  verification method and result (21 confirmed by symbol match, 1 by fixed
  PLT-stub offset).
- `apply_mapping.py`, `verify_mapping.py` — the two scripts used to derive
  and check the mapping, kept here as the reproducible method, not as a
  tool meant to be run against this repository's current
  `experimental/firmware-port/reactor/` files (those remain pinned to
  02.01.01.07 and unchanged by this commit).
- [`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md) —
  a methodology correction (virtual address vs. file offset) and the
  corrected 8-byte hook-entry fingerprint and `poll` PLT-target check,
  run directly against the documented port target.
- [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md) — the full-body
  instruction-level comparison of all four hook functions between the 07
  and 0113 builds, and a correction of an earlier conflation with an
  unrelated field in the existing `port-manifest.json`.
- [`SUPPORT_FUNCTIONS_DIFF_0113.md`](SUPPORT_FUNCTIONS_DIFF_0113.md) — the
  same comparison extended to the other 18 vendor addresses the bridge
  calls into (not just the 4 patch targets).
- [`ABI_AND_RUNTIME_VERIFICATION_0113.md`](ABI_AND_RUNTIME_VERIFICATION_0113.md) —
  a struct-offset cross-check against the already-verified vendor code,
  a vtable applicability check, and an isolated ARM32 (emulated)
  self-test of the bridge's own logic with the vendor mocked out.
- [`ABI_VENDOR_INTERFACE_VERIFICATION_0113.md`](ABI_VENDOR_INTERFACE_VERIFICATION_0113.md) —
  assumption/method/result/evidence for the remaining open struct
  offsets, function signatures, calling convention, and the
  exception/unwind boundary against the actual vendor binaries; unclear
  items are marked BLOCKER, not assumed compatible.
- [`ABI_BLOCKER_RESOLUTION_0113.md`](ABI_BLOCKER_RESOLUTION_0113.md) —
  follow-up resolving offset `108` (confirmed) and narrowing offsets
  `20`/`248` and the exception/unwind boundary; still-open items remain
  marked BLOCKER.
- [`ABI_BLOCKER_RESOLUTION_2_0113.md`](ABI_BLOCKER_RESOLUTION_2_0113.md) —
  further follow-up: offset `240` (map member base) confirmed by direct
  disassembly; offsets `20`/`248` upgraded to "indirectly well-supported
  via stable libstdc++ ABI," explicitly not claimed as directly
  confirmed.
- [`ABI_FINAL_CLOSURE_0113.md`](ABI_FINAL_CLOSURE_0113.md) — closes
  offsets `20`/`248` with direct disassembly evidence (all 11 struct
  offsets now confirmed), and strengthens the exception/unwind-boundary
  gate with a function-specific personality-routine check for
  `SelectReactor::pause`.
- [`ABI_FINAL_CLOSURE_2_0113.md`](ABI_FINAL_CLOSURE_2_0113.md) — closes
  the calling-convention gate for the function `cc2_wait` patches
  (hidden-result-pointer register layout confirmed by direct
  disassembly), and closes the `.ARM.extab` LSDA byte-level comparison
  for `SelectReactor::pause` (byte-identical except the one word fully
  explained by the already-confirmed personality-routine-address shift).
  The only remaining open ABI items are inherently runtime (a live
  exception through a real vendor frame) or require a reference artifact
  this repository does not have (the originally-intended toolchain).
- [`ARM_TEST_PLAN.md`](ARM_TEST_PLAN.md) — acceptance criteria and abort
  conditions for the still-outstanding hardware/ABI steps.
- [`INCIDENT_0113_ADDRESS_MISMATCH.md`](INCIDENT_0113_ADDRESS_MISMATCH.md) —
  a real hardware incident, now **resolved**: an unidentified,
  address-generation-mismatched `libcc2-reactor-v2.so` was found active via
  `LD_PRELOAD` on a real 0113 device. Confirms (not hypothesizes) the
  mismatch by direct byte search, explains why it did not visibly corrupt
  anything (fingerprint check held, fail-closed), and documents a
  `fallback()` bug (missing `LD_LIBRARY_PATH`) that broke printer startup
  whenever the launcher ran outside a shell that had sourced
  `/etc/profile` (the normal boot path was not affected) — fixed in both
  this repository (`c0f9fc0b`) and directly on the device, and verified by
  a real start/stop test through the actual `fallback()` path.

## Open items before any further step

Unchanged from `../REACTOR_CAMERA_PORT_TASKS.md`:

1. ABI re-verification of the `shared_ptr`/RB-tree layout assumptions
   against the actual 02.01.01.13 build (not assumed stable).
2. A real ARM execution test of `reactor_module.cpp` against the new
   binary — not yet done from this repository state.
3. Only after 1–2 pass: a new qualified `VENDOR_SHA256` pin — explicitly
   **not** part of this commit.
4. The pinned cross-toolchain (GCC 6.5.0 / glibc 2.23,
   `arm-cortex_a15-linux-gnueabihf`) is the one to build and verify
   against; loosening the GCC version check or toolchain path to work
   around a missing local toolchain is an environment problem to fix, not
   a check to relax, and is explicitly **not** carried into this
   repository by this commit.
