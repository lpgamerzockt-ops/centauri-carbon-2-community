# Reactor 02.01.01.13 ARM test plan — DRAFT, NOT EXECUTED, UNQUALIFIED

## Status and scope

**DRAFT. NOT EXECUTED. UNQUALIFIED.** This document defines acceptance
criteria and abort conditions for testing the reactor address mapping in
[`ADDRESS_MAPPING_0113.md`](ADDRESS_MAPPING_0113.md) on real ARM hardware.
No step in this plan has been run. Writing this plan does not install
anything, run anything on a printer, or change any hash gate.

- This commit adds **only this plan**. No installation, no script
  execution on a device, and no gate change is bundled with it.
- Any command below that would touch a real device and has not been
  checked against the existing launcher/installer scripts in this
  repository (`experimental/firmware-port/launcher.sh`,
  `builder/current/components/reactor/firmware-launcher.sh`,
  `cc2-control/installer/...`) is marked **BLOCKED — command not yet
  verified against existing launcher** rather than filled with a
  placeholder. Replacing a `BLOCKED` entry with a real command requires
  that review first, as its own follow-up.
- Passing every stage in this plan does **not** by itself authorize
  updating `VENDOR_SHA256`/`EXPECTED`. That update, if proposed, is its
  own separate, explicitly reviewed commit — never automatic on a
  successful smoke test (see "After this plan" below).
- No stage in this plan authorizes printing. A print test, if it is ever
  reached, requires its own explicit go-ahead beyond this document.

## 1. Artifact identity

Every test run in this plan must record, together, before any PASS is
recorded for any stage:

| Field | Value / how to obtain |
|---|---|
| Vendor `elegoo_printer` SHA-256 | `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` (confirm against the actual file under test, not assumed) |
| `libcolib.so` SHA-256 | `14288a4a73c339b039dd89597618ebb05702f90d668b76f74019d5d15ce67403` (confirm against the actual file under test) |
| `libcc2-reactor-v2.so` SHA-256 | A 0113-addressed candidate now exists: `de333cd9a0989de4c903fea369a503d2e587bd317e79a9e0c2bfddd0bb6fe424` (built from the repository's own, now-updated `reactor_module.cpp`/`fingerprints.h`, with the §2-toolchain equivalent — see `HASH_LEDGER.md` §2/§3b). This is this session's own build, not an externally qualified reference; re-confirm against the actual file under test before any step below. |
| Source commit | The exact commit of `experimental/firmware-port/reactor/{port-manifest.json,reactor_module.cpp}` the `.so` was built from |
| Toolchain fingerprint | Compiler version string (`<prefix>-g++ -dumpversion` and `--version`), target triple, sysroot path, the actual linked `libstdc++`/`libc` version (not just major version), and the full build-flag set (`-mcpu`, `-mfpu`, `-mfloat-abi`, exception/RTTI flags) — **all of it**, not "GCC 6 or similar" |
| Build command | The exact invocation used, verbatim, including environment (`PREFIX`, `PATH`) |
| Device firmware version | `otaversion` plus the three on-device binary hashes it was paired with at test time |

A hash by itself identifies an artifact for comparison; it is not a
signature and is not treated as one. No signing step is in scope here
(private keys are explicitly out of scope per
[`../HANDOFF_PACKAGE_CHECKLIST.md`](../HANDOFF_PACKAGE_CHECKLIST.md#3-what-not-to-include)).

An artifact-identity record on its own is traceability, not a pass. A
stage below is PASS only when its own stated acceptance criteria are also
met and recorded under "Result log" — never from artifact identity alone.

## 2. Static gates (must all PASS before any device step)

| Gate | Acceptance criterion | Status |
|---|---|---|
| Toolchain match | Compiler version, target, sysroot, and linked `libstdc++`/`libc` version match the pinned `arm-cortex_a15-linux-gnueabihf` GCC 6.5.0 / glibc 2.23 toolchain exactly — not "GCC 5 or 6" and not a substituted local toolchain path | NOT AVAILABLE / BLOCKED — no `/opt/cc2-cross/toolchain-out/bin/`, no `arm-linux-gnueabihf-gcc`/`g++`, no matching Homebrew cross toolchain, and only a bare `ubuntu:16.04` Docker image with no running container or installed build tools on this machine, as of 2026-11-09. See [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md) |
| Mapping re-verification | `verify_mapping.py` against freshly regenerated `old.sym`/`new.sym` reports the same 21 symbol-matched entries as [`ADDRESS_MAPPING_0113.md`](ADDRESS_MAPPING_0113.md) | PASSED — rerun against freshly regenerated symbol tables: `OK: 21  CHECK: 0  MISSING: 1`, see [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md) |
| PLT target proof (closes the 1 expected-MISSING) | The `0xbf70c → 0xbf628` `poll` PLT mapping is confirmed by an independent method beyond the `-0xe4` delta pattern — e.g. disassembling the PLT stub at `0xbf628` in the new binary and confirming it resolves to `poll@plt`/the same GOT relocation role as `0xbf70c` in the old binary. **"Expected MISSING" is a label on an unresolved gate, not a pass condition** | CONFIRMED against the documented target `688b7bf8…` — see [`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md): `poll@plt` at `0xbf628`, `R_ARM_JUMP_SLOT` relocation for `poll`, matching GOT layout |
| Hook-site instruction fingerprints | The 8-byte fingerprint bytes `reactor_module.cpp` checks at each of the 4 patch addresses before patching are regenerated from the real 02.01.01.13 binary at the new addresses (not carried over from the 02.01.01.07 fingerprints table) | CONFIRMED (8-byte entry sequence only) against the documented target `688b7bf8…` — see [`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md). Does **not** cover full function bodies, ABI, calling convention, or exceptions — those stay on the gates below |
| Struct/ABI layout re-verification | The RB-tree layout offsets and the `shared_ptr`-ABI `Shared` helper are confirmed against the new build's actual compiled object layout, under the exact pinned toolchain from "Toolchain match" above — compiler/library ABI and specific build flags affect this, not just the GCC major version | NOT RUN |
| Calling-convention re-verification | Parameter passing for each hooked function (hard float vs. soft float, struct-by-value, the hidden result pointer in `r0` that `cc2_wait` relies on for the `nlohmann::json` return) is confirmed for the new build, not assumed unchanged | NOT RUN |
| Exception-boundary re-verification | The C++ exception ABI across the hook boundary (every `cc2_*` entry point wraps vendor calls in `try/catch`) is confirmed compatible with the new build's compiler/library ABI | NOT RUN |
| `dispatch_loop` / `wait` instruction-level re-diff | Full-body instruction comparison (not just the 8-byte prologue) of all four hook functions between the 07 and 0113 builds, with every instruction that differs stated and explained | PASSED for the scope of comparing these four functions' own compiled bodies — see [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md): every differing instruction individually resolved (no blanket normalization); all four are instruction- and control-flow-identical except one explained, behaviorally-inert `__FILE__` build-path string. Also corrects an earlier conflation with the existing manifest's unrelated `instructions_equal_except_call_targets: false` field (that field compares stock-vendor code to the community replacement *within* 02.01.01.07, not 07 against 0113). This does **not** cover the separately-built `reactor_module.cpp` bridge's own ABI — see the gates below, which remain NOT RUN |

## 3. Test preparation (before any device step)

- [ ] Full backup of the current on-device state (`/opt/bin/elegoo_printer`,
  `/opt/lib/libcolib.so`, `/opt/usr/cc2-reactor-runtime-v2/`, relevant
  init scripts) taken and its own hash recorded.
- [ ] Rollback path verified working *before* any new artifact is
  installed — not assumed from a prior, unrelated rollback test.
- [ ] Start/stop procedure for the test build documented here before use:
  **BLOCKED — command not yet verified against existing launcher.**
- [ ] Confirmed exactly one printer instance is used for this entire plan
  (no mixing results from more than one device or from a device that was
  reflashed mid-plan).
- [ ] No print job active or queued for the duration of static gates,
  activation proof, and idle/function/motion/heating stages.

## 4. Activation proof (first device step, only after section 2 is all PASS)

Activation proof is necessary but not sufficient — it shows the hook
*attached*, not that the hooked functions execute correctly.

- [ ] Fresh `activation.marker` for *this* test run: delete/rename any
  pre-existing marker first, so a stale marker from an earlier, possibly
  replaced `.so` cannot be mistaken for this run's result.
- [ ] Marker's recorded PID matches the actual `elegoo_printer` PID at
  observation time (not a PID from a previous or since-restarted process).
- [ ] Marker's function pointers are checked against `/proc/<pid>/maps`
  for that PID: `libcc2-reactor-v2.so` is mapped, its on-disk file matches
  the SHA-256 recorded in section 1, and the function-pointer values fall
  inside that mapped range **accounting for the actual load bias** — do
  not assume a fixed base address unless the mapping explicitly uses one
  (e.g. a fixed `LD_PRELOAD`/`mmap` address); resolve pointer-to-mapping
  correspondence from the real `/proc/<pid>/maps` ranges for this run.
- [ ] Code-path execution evidence beyond the marker: the marker only
  proves `activate()` ran. For `cc2_poll`/`cc2_timers`/`cc2_wait`, add a
  bounded diagnostic counter (incremented on entry, read back via a
  separate, clearly-labeled diagnostic path) in a **separately tagged test
  build** — not the build that would ever be considered for the
  `VENDOR_SHA256` pin — and confirm each counter advances during the idle
  stage below. An unchanging counter after the idle stage means the hook
  attached but the code path did not run as expected, and this gate is
  FAIL regardless of the marker being present.

## 5. Test stages (each stage separate; do not skip ahead on a PASS)

| Stage | What it exercises | Pass condition |
|---|---|---|
| Emulation (if feasible) | Host-side or QEMU-based exercise of `reactor_module.cpp`'s bridge logic against the new addresses, reusing the existing `emulate_e*.py` harness pattern from `../` | Matches expected behavior with no crash, no exception propagating out of a `cc2_*` entry point |
| ARM idle | Device running, no print, no deliberate load, reactor hook active per section 4 | No crash, no core dump, diagnostic counters advancing, device remains responsive over a sustained idle period |
| Function | Deliberately exercise timers/callbacks/polling (the same kind of trigger used in the original crash reproduction, e.g. a LAN/WAN mode change) without motion or heating | No crash; each exercised code path's diagnostic counter advances; behavior matches the vendor's pre-port behavior for the same trigger |
| Motion | Controlled motion commands, no heating, no filament | No crash, no missed/garbled motion event attributable to the reactor hook |
| Heating | Controlled heating test, no motion, no print | No crash, no missed/garbled thermal event attributable to the reactor hook |
| Print | A real print job | **Not in scope until every stage above is PASS and reviewed** — no print test is authorized by this plan alone |

## 6. Result log

Record for every stage, as it is actually run (do not pre-fill):

| Stage | Expectation | Observation | Evidence (file/log/hash) | Result |
|---|---|---|---|---|
| (one row per stage above) | | | | PASS / FAIL / BLOCKED / NOT RUN |

Every row starts `NOT RUN`. `BLOCKED` is used when a prerequisite gate or
section-3 item is not satisfied yet — never silently skipped.

## 7. Abort and rollback

- **Abort immediately** on: any crash/core dump, any `SIGSEGV`/`pc=0`
  pattern (as previously seen in the original payload crash), the reactor
  diagnostic counters failing to advance when they should, any
  `/proc/<pid>/maps` mismatch against section 1's recorded identity, or
  any kernel message indicating instability (e.g. a `preempt_count`
  warning or a "reboot is needed" message) — note that such a message
  from an *earlier, unrelated run* does not count as evidence for or
  against *this* run; only a message observed during *this* run's own
  stages does.
- On abort: preserve the device's current state for diagnosis (core dump,
  relevant logs, `/proc/<pid>/maps` if still available) before taking any
  recovery action.
- Recovery: restore from the section-3 backup, confirm the restored
  binaries' hashes match the pre-test backup record, and confirm the
  device returns to its last known-good, qualified state
  (`VENDOR_SHA256`-matching `elegoo_printer`, reactor hook active via the
  existing, unchanged 02.01.01.07 qualification) before ending the test
  session.

## After this plan

If every stage above is PASS and reviewed, updating the
`VENDOR_SHA256`/`EXPECTED` pins to the 02.01.01.13 hash may be **proposed**
as its own, separately reviewed commit. It does not happen automatically
as a consequence of a passing smoke test, and this plan does not pre-approve
it.
