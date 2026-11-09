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
| Toolchain match | Compiler version, target, sysroot, and linked `libstdc++`/`libc` version match the pinned `arm-cortex_a15-linux-gnueabihf` GCC 6.5.0 / glibc 2.23 toolchain exactly — not "GCC 5 or 6" and not a substituted local toolchain path | **UPDATED 2026-11-09:** a self-built crosstool-NG 1.24.0 toolchain now exists and reports exactly GCC 6.5.0 / glibc 2.23 (see `HASH_LEDGER.md` §2). This is a **TEST CANDIDATE**, not a verified match to the originally-intended `/opt/cc2-cross/toolchain-out/...` toolchain, whose own provenance is still undocumented anywhere in this repository — the two have never been compared to each other. Treat as PASSED-FOR-THIS-CANDIDATE, not as closing the gate's original "match the pinned toolchain" wording |
| Mapping re-verification | `verify_mapping.py` against freshly regenerated `old.sym`/`new.sym` reports the same 21 symbol-matched entries as [`ADDRESS_MAPPING_0113.md`](ADDRESS_MAPPING_0113.md) | PASSED — rerun against freshly regenerated symbol tables: `OK: 21  CHECK: 0  MISSING: 1`, see [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md) |
| PLT target proof (closes the 1 expected-MISSING) | The `0xbf70c → 0xbf628` `poll` PLT mapping is confirmed by an independent method beyond the `-0xe4` delta pattern — e.g. disassembling the PLT stub at `0xbf628` in the new binary and confirming it resolves to `poll@plt`/the same GOT relocation role as `0xbf70c` in the old binary. **"Expected MISSING" is a label on an unresolved gate, not a pass condition** | CONFIRMED against the documented target `688b7bf8…` — see [`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md): `poll@plt` at `0xbf628`, `R_ARM_JUMP_SLOT` relocation for `poll`, matching GOT layout |
| Hook-site instruction fingerprints | The 8-byte fingerprint bytes `reactor_module.cpp` checks at each of the 4 patch addresses before patching are regenerated from the real 02.01.01.13 binary at the new addresses (not carried over from the 02.01.01.07 fingerprints table) | CONFIRMED (8-byte entry sequence only) against the documented target `688b7bf8…` — see [`FINGERPRINT_VERIFICATION_0113.md`](FINGERPRINT_VERIFICATION_0113.md). Does **not** cover full function bodies, ABI, calling convention, or exceptions — those stay on the gates below |
| Struct/ABI layout re-verification | The RB-tree layout offsets and the `shared_ptr`-ABI `Shared` helper are confirmed against the new build's actual compiled object layout, under the exact pinned toolchain from "Toolchain match" above — compiler/library ABI and specific build flags affect this, not just the GCC major version | **CONFIRMED (updated 2026-11-09):** all 11 of 11 hardcoded offsets (`144,32,16,264,8,24,96,108,240,248,20`) now confirmed by direct disassembly against the real `688b7bf8…` binary. See `ABI_AND_RUNTIME_VERIFICATION_0113.md`, `ABI_BLOCKER_RESOLUTION_0113.md`, `ABI_BLOCKER_RESOLUTION_2_0113.md`, `ABI_FINAL_CLOSURE_0113.md`. Still **not** run under the exact *originally-pinned* toolchain (see Toolchain match above) — this is confirmed against the vendor binary's own compiled layout directly, independent of which toolchain built the bridge |
| Calling-convention re-verification | Parameter passing for each hooked function (hard float vs. soft float, struct-by-value, the hidden result pointer in `r0` that `cc2_wait` relies on for the `nlohmann::json` return) is confirmed for the new build, not assumed unchanged | **CONFIRMED (updated 2026-11-09):** direct disassembly of `ReactorCompletion::wait` (the exact function `cc2_wait` patches, `0x26a640`→`0x26cb70`) shows the identical prologue in both builds, with `r0`=hidden-out, `r1`=`this`, `d0`=waketime, `r2`=fallback-ptr — an exact structural match to `cc2_wait`'s own `(out,self,waketime,fallback)` signature. See `ABI_FINAL_CLOSURE_2_0113.md`. (Function signatures for the other 21/22 `original<>()` targets remain confirmed via exact mangled-name match; ELF-level hard-float ABI flag `0x5000400` confirmed identical, per `ABI_VENDOR_INTERFACE_VERIFICATION_0113.md`.) |
| Exception-boundary re-verification | The C++ exception ABI across the hook boundary (every `cc2_*` entry point wraps vendor calls in `try/catch`) is confirmed compatible with the new build's compiler/library ABI | **CONFIRMED for static encoding (updated 2026-11-09), not a full PASS:** `SelectReactor::pause` (the specific function `cc2_wait` calls) has a valid `Generic`/`.ARM.extab` unwind entry in both builds, whose `PersonalityRoutineAddress` disassembles to `__gxx_personality_v0@plt` in both (`ABI_FINAL_CLOSURE_0113.md`). The raw `.ARM.extab` LSDA bytes were then compared directly: 15 of 16 words are byte-identical between builds, and the 1 differing word is fully explained by the (already-confirmed) personality-routine-address shift, decoded and checked exactly — see `ABI_FINAL_CLOSURE_2_0113.md`. An actual live exception thrown through a real vendor frame remains untested — that is a runtime property, not closable by static analysis; only a real device/runtime test can close it |
| `dispatch_loop` / `wait` instruction-level re-diff | Full-body instruction comparison (not just the 8-byte prologue) of all four hook functions between the 07 and 0113 builds, with every instruction that differs stated and explained | PASSED for the scope of comparing these four functions' own compiled bodies — see [`INSTRUCTION_DIFF_0113.md`](INSTRUCTION_DIFF_0113.md): every differing instruction individually resolved (no blanket normalization); all four are instruction- and control-flow-identical except one explained, behaviorally-inert `__FILE__` build-path string. Also corrects an earlier conflation with the existing manifest's unrelated `instructions_equal_except_call_targets: false` field (that field compares stock-vendor code to the community replacement *within* 02.01.01.07, not 07 against 0113). This does **not** cover the separately-built `reactor_module.cpp` bridge's own ABI — see the gates below, which remain NOT RUN |

## 3. Test preparation (before any device step)

- [x] Full backup of the current on-device state (`/opt/bin/elegoo_printer`,
  `/opt/lib/libcolib.so`, `/opt/usr/cc2-reactor-runtime-v2/`, relevant
  init scripts) taken and its own hash recorded.
  - **Status 2026-11-09: DONE.** Copied from the device to the
    operator's machine in two rounds and hashed there. First round:

    | File | SHA-256 of the local copy | Matches the hash read on the device? |
    |---|---|---|
    | `/opt/bin/elegoo_printer` | `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` | yes |
    | `/opt/lib/libcolib.so` | `14288a4a73c339b039dd89597618ebb05702f90d668b76f74019d5d15ce67403` | yes |
    | `/etc/init.d/cc2-reactor-launch` (before the `fallback()` fix) | `a8a736fdedba6efb5b3ddc1e636c33da33533083708374249493e3d97e9c0da8` | md5 `9e15e111…` matched the device |
    | `/opt/usr/cc2-reactor-runtime-v2/` without its `elegoo_printer` copy (tar archive) | `0aa007c4f4e2ff820de73ae5f363d95667ce0a5bc47b8fdb3c7ad1923cadb94f` | archive lists and extracts cleanly; contains `libcc2-reactor-v2.so` `8f78646d…` |

  - Second round: one archive of 13 files. All 13 local hashes equal
    the hashes read on the device at the same time:

    | File | SHA-256 |
    |---|---|
    | `/opt/bin/ec-eeb001-gui` | `668ab8cb490c74eb5bbd769b421f6e4ade4e91fba322e22257496b9645f9ebfe` |
    | `/opt/bin/ai_camera` | `90da7634c21b74b3bf943d68129da78668b47f9706620e158283697ab95553a7` |
    | `/opt/inst/daemon-000/daemon-000` | `83433b14e49d1e8aa9523282ead03618378573f4b66de1ad8f7f3c444b11aada` |
    | `/etc/init.d/printer` | `e7537298955e23b7a03cab4c8d7bd241c1dfcc070883cdf51c4889cd255d1bc4` |
    | `/etc/init.d/cc2-reactor-launch` (with the device-side `fallback()` fix) | `8183b1273c91d4a6a27920ce7fe46b52a3bfdbf04eaa92bb8b3e27816db03de9` |
    | `/opt/bin/run_printer.sh` | `05e4614779fc59b43c604eda7d85f1e8534b24d3cf284b16226c9c3bbc975118` |
    | `/opt/inst/cc2-reactor/` (7 files) | as listed in its own `SHA256SUMS` (`84f8fb5d…`); includes `libcc2-reactor-v2.so` `8f78646d…` |

  - The runtime directory's own `elegoo_printer` copy hashes to
    `688b7bf8…`, the same as `/opt/bin/elegoo_printer`, so the backup of
    the latter covers it.
  - Two earlier full-directory archive attempts were truncated in transfer
    and are not usable.
  - Not backed up: the vendor stock (unpatched) binaries. Returning the
    device to stock ELEGOO binaries would need the files from the
    official 02.01.01.13 package, which has not been unpacked for this.
- [x] Rollback path verified working *before* any new artifact is
  installed — not assumed from a prior, unrelated rollback test.
  - **Status 2026-11-09: VERIFIED for `/opt/bin/elegoo_printer`**, by a
    dress rehearsal that replaced it with its own byte-identical backup.
  - **Why the first attempt failed.** Writing the backup straight onto
    `/opt/bin/elegoo_printer` with `scp` failed with
    `dest open "/opt/bin/elegoo_printer": Failure`. Checked on the device:
    `/` (which holds `/opt/bin`) is a writable overlay, 385 MB free, owner
    `root`, session runs as `uid=0`. Opening the real file for append
    gave `ash: can't create elegoo_printer: Text file busy`, hash
    unchanged. Cause: Linux refuses to open an executable for writing
    while a process runs from it.
  - **Procedure proven on a probe file first.** A copy of BusyBox started
    as `sleep` in a scratch directory under `/opt/bin`: opening it for
    writing gave `Text file busy`; uploading a second copy under another
    name, checking its hash and `mv` over the running file succeeded
    (`mv rc=0`), and the running process stayed alive. Scratch directory
    removed afterwards.
  - **Dress rehearsal on `elegoo_printer`** (no print job active):
    1. `scp` the backup to `/opt/bin/elegoo_printer.rollback`.
    2. On the device: abort unless its hash is `688b7bf8…`; `chmod 755`;
       copy the current file to `elegoo_printer.prev` and abort unless
       that hash is `688b7bf8…`; then
       `mv elegoo_printer.rollback elegoo_printer`.
       Result: `mv rc=0`, both files `688b7bf8…`. The running process
       (PID 2411) kept running from the replaced file
       (`/proc/2411/exe -> /opt/bin/elegoo_printer (deleted)`).
    3. `run_printer.sh stop`, `run_printer.sh start`. Result: one process,
       PID 2658, `/proc/2658/exe -> /opt/bin/elegoo_printer` (no
       `(deleted)`), no `LD_PRELOAD` in its environment, file hash
       `688b7bf8…`.
  - **Scope.** Verified only for `elegoo_printer`, and only with
    identical content: it shows the replacement path works, not that a
    different binary would start. The same procedure is expected to work
    for `ec-eeb001-gui` and `ai_camera`, but was not run for them, and
    restarting them cleanly may need a full device reboot. A file under
    `/etc/init.d/` was replaced with plain `scp` the same day (no process
    runs from it).
  - Rule for any later binary change: upload under a new name, check the
    hash on the device, keep a `.prev` copy, `mv` into place, then restart
    the process. Never write onto a running binary.
- [ ] Start/stop procedure for the test build, verified against the actual
  launcher scripts in this repository (read directly, not assumed):
  - **Start (test-candidate path):** `experimental/firmware-port/launcher.sh`
    is the direct test-candidate launcher. Read verbatim, it: (1) refuses to
    run unless `$BASE/elegoo_printer` (`$BASE=/opt/usr/cc2-reactor-runtime-v2`)
    hashes to the pinned `EXPECTED` value, (2) refuses unless
    `$BASE/SHA256SUMS` verifies clean, (3) refuses if `LD_PRELOAD` is already
    set in the environment, (4) otherwise sets
    `CC2_REACTOR_RUNTIME_V2=1` and `LD_PRELOAD=$BASE/libcc2-reactor-v2.so`,
    then `exec`s `$BASE/elegoo_printer "$@"`. So the verified start command,
    once `$BASE` holds the test artifacts and a matching `SHA256SUMS`, is
    exactly `sh experimental/firmware-port/launcher.sh` run as the user/context
    that normally launches `elegoo_printer` — any of the hash/`LD_PRELOAD`
    guards failing exits `125` without starting anything.
  - **Full on-device path:** `builder/current/components/reactor/firmware-launcher.sh`
    is the installed entry point that calls the above. Read verbatim, it
    additionally: waits (up to 120 s) for `/opt/usr` to be mounted, checks a
    `cc2-reactor-disabled` marker, checks `LD_PRELOAD` is unset, checks the
    stock `/opt/bin/elegoo_printer` and `/opt/lib/libcolib.so` hashes against
    the pinned `EXPECTED`/`libcolib.so` values, verifies `/opt/inst/cc2-reactor`'s
    own `SHA256SUMS`, stages a fresh copy into `$BASE` only if the payload
    hash changed, runs that staged copy's own `preflight.sh`, and only then
    `exec`s `$BASE/launcher.sh "$@"` (which is the script above). Any guard
    failing calls `fallback()`, which logs via `logger -t cc2-reactor` and
    `exec`s the plain, unmodified `/opt/bin/elegoo_printer` instead — i.e. the
    documented failure mode is a silent fall back to the stock binary, not a
    crash or a stop.
  - **Stop:** no stop/respawn logic exists in either launcher script or
    anywhere else in this repository — both scripts end in `exec`, which
    replaces the shell process with `elegoo_printer` and never returns to run
    a stop path. Process supervision (what restarts or stops `elegoo_printer`
    on the device, and how) is owned by the device's own init system outside
    this repository's scope and is not documented here; inventing a stop
    command would be unverified. The one process-identity check this
    repository itself actually uses and verifies is `pidof elegoo_printer`,
    confirmed directly in `cc2-control/scripts/launch.sh` and
    `cc2-control/scripts/start-firmware.sh` — suitable for confirming the
    process is (still) running, not for stopping it.
  - This closes the "command not yet verified against existing launcher"
    gate for *start*; it does not invent or verify a *stop* command, since
    none exists in the sources checked.
  - **Defect found and fixed on real hardware (see
    [`INCIDENT_0113_ADDRESS_MISMATCH.md`](INCIDENT_0113_ADDRESS_MISMATCH.md)):**
    `fallback()` in both the on-device `/etc/init.d/cc2-reactor-launch` and
    this repository's own `firmware-launcher.sh` exec'd `elegoo_printer`
    without exporting `LD_LIBRARY_PATH=/opt/lib` first, unlike the normal
    path — a device reaching `fallback()` (e.g. via
    `/opt/usr/cc2-reactor-disabled`, or a hash-check failure) from a shell
    that had not sourced `/etc/profile` (such as a non-interactive SSH
    command) failed to start `elegoo_printer` (`error while loading shared
    libraries: libcolib.so: cannot open shared object file`). The normal
    boot path was not affected, because `/etc/init.d/printer` sources
    `/etc/profile`, which exports `LD_LIBRARY_PATH=/opt/lib`. **Fixed** directly
    on the affected device (the repository change `c0f9fc0b` broke
    dependent hash pins and was reverted, both within fork PR #1, so this repository's `firmware-launcher.sh` still has the
    defect); verified by a real `run_printer.sh stop`/`start` cycle over SSH through
    the actual `fallback()` path (new clean process instance, no
    `LD_PRELOAD`, no error). For any manual start/stop step in this plan,
    use a device that has this fix, or source `/etc/profile` first.
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

State on 2026-11-09: no stage of section 5 has been run; the reactor is
disabled on the test device. The hardware tests H1–H3 of
[`../TEST_MATRIX.md`](../TEST_MATRIX.md) (Z-offset menu during a print,
camera with remote access, OrcaSlicer upload/start/cancel) have not been
run either. The rollback prerequisite in section 3 is now met for
`elegoo_printer` (see above); H1–H3 install nothing, so they are no
longer blocked by it.

- H1 (Z-offset menu during a print): **PASS** on 2026-11-10; details and
  the limits of the evidence in [`../TEST_MATRIX.md`](../TEST_MATRIX.md),
  section "Ergebnisse auf echter Hardware". This is a test of the GUI
  patch, not of the reactor; section 5 stays NOT RUN.
- H2 (camera locally and remotely at the same time): **PASS** on
  2026-11-10.
- H5a (switch from LAN to non-LAN mode, the trigger of the original
  crash): **PASS** on 2026-11-10, no crash, same `elegoo_printer` PID.
  H5b (switch back to LAN) not run.
- H3 (OrcaSlicer upload/start/cancel): **PARTIAL PASS** on 2026-11-10 —
  upload and delete via the Community UI during a running print, plus
  start/pause/cancel/return-to-idle, all passed with the same
  `elegoo_printer` PID. The Port-80 upload path (where the patch's
  status guards sit) was not reached: ElegooSlicer refuses the upload
  itself while the printer is not idle, on stock firmware too. See
  [`../TEST_MATRIX.md`](../TEST_MATRIX.md).
- Not run: H4, H6, H7, H5b, and the Port-80 upload-during-print path.
  No reactor stage of section 5 was run; the reactor stays disabled.

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
