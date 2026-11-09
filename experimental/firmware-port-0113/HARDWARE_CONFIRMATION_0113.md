# Hardware confirmation: GUI, camera, printer and Dual Trust patches match the real 0113 device

Factual record from direct hash comparison against a real, running
02.01.01.13 printer. This does **not** set `ported_patch_set` to `true` in
`builder/current/base_profiles/02.01.01.13.json`, and does **not** change
`QUALIFIED_BASE` in `builder/current/core/firmware_builder.py` — that
decision needs its own separate design pass, because `QUALIFIED_BASE` and
the `EXPECTED` pin dictionary it feeds are currently single-base
(`02.01.01.07`) by construction, not because this evidence is
insufficient. See "What this does and does not establish" below.

## What was compared

This repository already holds a statically-produced, 0113-ported set of
patched binaries under `experimental/firmware-port-0113/WORK/corrected_v2/`
(not committed — gitignored, local analysis output), with the following
recorded in `WORK/corrected_v2/report.json`:

```
elegoo_printer.patched_sha256: 688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd
ec-eeb001-gui.patched_sha256:  668ab8cb490c74eb5bbd769b421f6e4ade4e91fba322e22257496b9645f9ebfe
ai_camera.patched_sha256:      90da7634c21b74b3bf943d68129da78668b47f9706620e158283697ab95553a7
checks: 42 PASS, 0 FAIL — "Adress-/Branch-Konsistenz OK (Struktur); Emulation + Hardware weiterhin noetig."
```

These three hashes were re-computed independently in this session
(`sha256sum` on the actual files), confirming the report's numbers are
not stale or copied wrong.

The corresponding `daemon-000` output (Dual Trust) is not a file in
`WORK/`, but its expected hash is pinned in `firmware_builder.py` as
`EXPECTED['daemon_dual'] = '83433b14e49d1e8aa9523282ead03618378573f4b66de1ad8f7f3c444b11aada'`.

## Direct comparison against the real device

Read directly from the real, running 02.01.01.13 printer (SSH, read-only
commands only, no file written):

| Binary | Expected (static, `corrected_v2` / `EXPECTED`) | Found live on device | Match? |
|---|---|---|---|
| `/opt/bin/elegoo_printer` | `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` | `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` | **yes** |
| `/opt/bin/ec-eeb001-gui` | `668ab8cb490c74eb5bbd769b421f6e4ade4e91fba322e22257496b9645f9ebfe` | `668ab8cb490c74eb5bbd769b421f6e4ade4e91fba322e22257496b9645f9ebfe` | **yes** |
| `/opt/bin/ai_camera` | `90da7634c21b74b3bf943d68129da78668b47f9706620e158283697ab95553a7` | `90da7634c21b74b3bf943d68129da78668b47f9706620e158283697ab95553a7` | **yes** |
| `/opt/inst/daemon-000/daemon-000` | `83433b14e49d1e8aa9523282ead03618378573f4b66de1ad8f7f3c444b11aada` | `83433b14e49d1e8aa9523282ead03618378573f4b66de1ad8f7f3c444b11aada` | **yes** |

All four match exactly. This device is, right now, running the exact
statically-produced 0113 patch output for all four of the patches listed
as required in `base_profiles/02.01.01.13.json`'s `ported_patch_set`
comment (Z-offset/GUI, HTTP/network — combined into `elegoo_printer` —
and camera), plus the separately-tracked Dual Trust patch to `daemon-000`.

## Relation to the open tracking entries

- Upstream issue #334
  (<https://github.com/Neme77/centauri-carbon-2-community/issues/334>)
  lists the same GUI (`668ab8cb…`) and camera (`90da7634…`) output
  hashes as found on the device. Its printer hash (`adb85293…`) is an
  earlier build; the draft pull request #336
  (<https://github.com/Neme77/centauri-carbon-2-community/pull/336>)
  states that this earlier build had a payload defect that its tooling
  fixes. The device runs `688b7bf8…`, the output recorded in
  `WORK/corrected_v2/report.json`.
- Both entries describe the port as static- and emulation-verified and
  not hardware-tested. This document adds hash evidence from one device;
  it does not change that status.

## Runtime evidence, precisely stated

Two uptime measurements exist from the same session, separated by a full
device reboot:

- **Earlier measurement (before the reboot):** a `dmesg | tail` read on
  the device showed its most recent kernel message at
  `[17505.103333]`, i.e. the system had then been up for **at least
  17505 s (~4 h 52 min)**. `elegoo_printer` was PID 2064 at that time.
  The patched `elegoo_printer` hash (`688b7bf8…`) was already confirmed
  on the device during that boot. The operator reports that the GUI and
  camera were also checked during that earlier boot; that reading is not
  part of the command output recorded for this document, so it is noted
  here as operator-reported rather than as a recorded hash comparison.
  The recorded evidence for the longer uptime therefore covers the
  printer binary; for GUI and camera it rests on the operator's report.
- **Later measurement (after the reboot):** `/proc/uptime` read
  **2759.93 s (~46 minutes)**. The GUI, camera and `daemon-000` hashes in
  the table above were read during this second boot.
- That a full reboot (not only a process restart) happened in between
  follows from the uptime dropping and from `ec-eeb001-gui`/`ai_camera`
  having start times ~10.8 s after boot. What triggered the reboot was
  not recorded.

Details of the later measurement:
- `ec-eeb001-gui` (PID 1901) and `ai_camera` (PID 1902) both have a
  process start time of **1084 clock ticks (~10.8 s) after boot** — i.e.
  both started automatically as part of the normal boot sequence, not
  manually, and have been running continuously since.
- This means both the GUI and camera patches have been running without
  a crash or restart for the device's **entire current uptime since its
  last boot (~46 minutes)**, integrated cleanly into the standard boot
  sequence. It does **not** constitute a multi-hour or multi-day soak
  test for these two binaries — the device had rebooted ~46 minutes
  before this measurement (see above).
- `elegoo_printer` has been separately confirmed stable (see
  `experimental/firmware-port-0113/reactor/INCIDENT_0113_ADDRESS_MISMATCH.md`
  §7) through an intentional stop/start cycle, with normal startup
  through the full vendor sequence (MCU/toolhead/bed-sensor
  configuration, bed mesh, input shaper, HTTP server, MQTT, Canvas
  detection, filament sensor, reaching `IDLE`).

## What this does and does not establish

**Establishes:**
- The 0113-ported GUI, camera, and printer (network+HTTP) patches, as
  statically produced in `WORK/corrected_v2/`, are not merely
  theoretical — they are the exact bytes already running on a real
  0113 device, for the device's current uptime, without a crash.
- The Dual Trust patch's expected output for 0113 matches what the same
  device already runs, consistent with `daemon_stock`/`daemon_dual`
  being pinned as identical to 02.01.01.07 in the base profile.

**Does not establish:**
- That a 02.01.01.13 Community build is complete. The build path also
  needs the native display, the Connections component (vendor file pins
  and kernel modules), the camera runtime and the CC2 Control
  compatibility list, none of which is covered by these four hashes; see
  `PIPELINE_MULTI_BASE_DESIGN.md` section 2.
- A formal, documented emulation test for GUI or camera (per
  `WORK/corrected_v2/report.json`'s own words: "Emulation + Hardware
  weiterhin noetig" was still true as of that report; this document is
  new hardware evidence, not a replacement for the emulation step the
  project's own test matrix calls for).
- Long-duration stability (only ~46 minutes of continuous uptime
  measured here).
- That `ported_patch_set` should be set to `true`, or that
  `QUALIFIED_BASE` in `firmware_builder.py` should change. That decision
  is deliberately **not** made by this document — see the next section.

## Why `ported_patch_set` and `QUALIFIED_BASE` are not touched here

`firmware_builder.py`'s `QUALIFIED_BASE` is a single hardcoded version
string, and the module-level `EXPECTED` pin dictionary is populated once,
at import time, from the one profile `QUALIFIED_BASE` names. Several
`EXPECTED` entries are themselves base-specific outputs, not vendor
inputs — for example `EXPECTED['patched_gui'] = '5f312285…'`, which is
the **02.01.01.07** GUI patch output, not the 0113 one (`668ab8cb…`)
confirmed above. Flipping `ported_patch_set` in the 0113 profile alone
would have no effect, since `require_buildable_profile()` also requires
`profile['version']==QUALIFIED_BASE`, and `QUALIFIED_BASE` is still
`'02.01.01.07'`.

Supporting both bases as buildable (the agreed direction) means making
`EXPECTED` (and everything that reads it) base-aware, not just changing
one constant. That is a deliberate, security-relevant change to the core
build pipeline's hash-pinning logic and deserves its own focused design
and review, separate from this hardware-confirmation record.

## Scope of this document

Local, read-only comparison against a real device (hash reads only) plus
independent recomputation of locally-held, gitignored static-analysis
output. No file was installed, changed, or removed on the device; no
repository file outside this new document was changed; no hash gate,
pin, or qualification flag was modified.
