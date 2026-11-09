# Incident: address-generation-mismatched reactor library loaded on a real 0113 device

Factual record of a real hardware incident discovered during work toward
making the hardware test plan ([`ARM_TEST_PLAN.md`](ARM_TEST_PLAN.md))
stably testable. This happened on a real printer (IP withheld from this
document, SN known to the operator), not in emulation. Documented here as
an incident, not as a qualification of the 0113 port — it changes nothing
about the "experimental, unqualified" status in [`README.md`](README.md).

## 1. What was found running

A `libcc2-reactor-v2.so` was already active via `LD_PRELOAD` on the
device (confirmed live: `cat /proc/<pid>/environ` showed
`LD_PRELOAD=/opt/usr/cc2-reactor-runtime-v2/libcc2-reactor-v2.so`, library
listed in the process's maps). The file at that exact path was captured
into a local backup and hashed directly:

```
sha256sum .../libcc2-reactor-v2.so
8f78646d284ae1a4f76c52d6e33f51f6929c1f3d0dfbf26fd90d05465735b3ea
```

This hash matches neither this session's own 0113-port build candidate
recorded in `HASH_LEDGER.md` (`de333cd9…`) nor any other hash documented
anywhere in this repository. Its provenance (which build, which source,
when installed) is **not established** and is not claimed here.

The device's `/opt/bin/elegoo_printer` hashes to
`688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` —
confirmed identical to this session's own local copy of the 0113 binary
used to derive [`ADDRESS_MAPPING_0113.md`](ADDRESS_MAPPING_0113.md)
(`sha256sum ~/reactor-analysis/new.elegoo_printer` gives the same hash).
So the 0113 addresses documented in this repository are, concretely, the
correct ones for the exact binary that was running on this device.

## 2. Address-generation mismatch: confirmed, not a hypothesis

The captured `.so` was searched byte-for-byte (as literal 4-byte
little-endian words, since `patch()` loads each target address from a
literal pool) for both the four 02.01.01.07-era patch addresses and the
four 0113 patch addresses from `ADDRESS_MAPPING_0113.md`:

| Function | 07-era address | Found in the captured `.so`? | 0113 address | Found in the captured `.so`? |
|---|---|---|---|---|
| `ReactorCallback::invoke` | `0x26ab5c` | **yes**, at file offset `0x623c` | `0x26d08c` | no |
| `SelectReactor::check_timers` | `0x26ccac` | **yes**, at file offset `0x6248` | `0x26f1dc` | no |
| `PollReactor::dispatch_loop` | `0x26e3a8` | **yes**, at file offset `0x6254` | `0x2708d8` | no |
| `ReactorCompletion::wait` | `0x26a640` | **yes**, at file offset `0x6260` | `0x26cb70` | no |

All four 07-era addresses are present; none of the four 0113 addresses
appear anywhere in the file. This directly demonstrates the loaded
library's hardcoded patch table targets the 07-era function layout, in a
process running the 0113-era `elegoo_printer` (addresses shifted by
`+0x2530` throughout this port, see `ADDRESS_MAPPING_0113.md`).

## 3. Why this did not visibly corrupt anything: the fingerprint check held

`/opt/usr/cc2-reactor-runtime-v2/activation.marker` did **not** exist when
checked. `reactor_module.cpp`'s own `cc2_activate()` only writes that
marker after every one of its four `patch()` calls succeeds (each
`patch()` call first checks an 8-byte fingerprint at the target address
and refuses to patch on a mismatch). The absence of the marker, together
with the running `elegoo_printer` process being stable (same PID across
multiple checks, no new entries in `dmesg`), is consistent with the
fingerprint check rejecting the patch at the wrong (07-era) addresses and
failing closed, as designed. This is the most likely explanation
supported by the available evidence; it is not a formal proof that no
single byte was ever written anywhere (that would require a full memory
diff, which was not performed).

## 4. The `fallback()` bug: confirmed; fixed on the device, open in the repository

The real, on-device start chain (read directly, not assumed from the
repository's committed copy, which turned out to match it) is:

```
/etc/init.d/printer (start) -> run_printer.sh start
run_printer.sh -> /etc/init.d/cc2-reactor-launch $CFG...
cc2-reactor-launch -> hash/payload checks -> export LD_LIBRARY_PATH=/opt/lib;
                       export LD_PRELOAD=.../libcc2-reactor-v2.so; exec elegoo_printer
```

`/opt/usr/cc2-reactor-disabled` was created on this device (intentionally,
to stop the mismatched library from being preloaded again). This makes
`cc2-reactor-launch` take its `fallback()` path instead:

```sh
fallback() {
    logger -t cc2-reactor "Experimental callback disabled or unavailable; starting vendor binary"
    [ "$PREPARE_ONLY" -eq 0 ] || exit 0
    exec /opt/bin/elegoo_printer "$@"
}
```

This function execs `elegoo_printer` **without** exporting
`LD_LIBRARY_PATH=/opt/lib` first (unlike the normal/success path, which
does export it). Observed, exact failure after `run_printer.sh stop;
run_printer.sh start` with the disable marker present:

```
/opt/bin/elegoo_printer: error while loading shared libraries: libcolib.so: cannot open shared object file: No such file or directory
```

The printer was brought back up only by a **manual, one-off** invocation
bypassing the whole launcher chain:

```sh
LD_LIBRARY_PATH=/opt/lib /opt/bin/elegoo_printer /opt/inst/printer_dsp.cfg -s /opt/usr/cfg/autosave.cfg -a /tmp/elegoo_uds &
```

At the time this was first found, `/etc/init.d/cc2-reactor-launch` itself
had **not** been edited: re-read right after the incident, its `md5sum`
was `9e15e1116264c2438b388f7052db57ac` and its `fallback()` body was
byte-for-byte the same as quoted above — still missing the
`LD_LIBRARY_PATH` export. The same bug is
present in this repository's own committed
[`builder/current/components/reactor/firmware-launcher.sh`](../../../builder/current/components/reactor/firmware-launcher.sh),
whose `fallback()` has the same shape and the same missing export.

**Correction (added after a follow-up check): the failure depends on the
calling environment, and the normal boot path was not affected.** An
earlier version of this document claimed a real reboot would have hit
this bug and failed to start `elegoo_printer`. That claim was wrong:

- `/etc/profile` line 31 on the device reads
  `export LD_LIBRARY_PATH=/opt/lib:$LD_LIBRARY_PATH`.
- `/etc/init.d/printer`'s `start()` runs `source /etc/profile` before it
  calls `run_printer.sh start`, so at boot `fallback()` inherits
  `LD_LIBRARY_PATH` from its parent even though it does not export it
  itself.
- The observed failure came from `run_printer.sh stop; run_printer.sh
  start` issued as a non-interactive SSH command, an environment that
  does not source `/etc/profile`.
- Consistent with this: the device went through a full reboot during the
  same session while the disable marker was present and before the
  script was fixed, and `elegoo_printer` was running afterwards (seen as
  PID 1998, no `LD_PRELOAD`). That it was started by the boot sequence
  is inferred from the lower PID and the reset uptime, not separately
  logged.

So the defect is real but narrower: `fallback()` silently relied on its
caller's environment, and failed whenever the launcher was started
outside a profile-sourced shell. The fix in §7 removes that dependency.

## 5. Confirmed device state at that point (before the fix in §7)

- `elegoo_printer` running, single instance, no `LD_PRELOAD` in its
  environment (`cat /proc/<pid>/environ | grep -i preload` empty).
- `/opt/bin/elegoo_printer` on disk: unmodified, hash
  `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd`
  (matches the hash `cc2-reactor-launch` itself pins as `EXPECTED`).
- `/opt/usr/cc2-reactor-disabled` present → future boots take the
  `fallback()` path (which worked at boot, but failed when started from
  a shell without `/etc/profile` — see the correction in §4).
- Reactor hook not activated at any point checked (no
  `activation.marker`).

## 6. What is not established

- The provenance of the active `8f78646d…` library — which build, from
  where, installed when. Not claimed here, and no earlier conversation
  record in this repository documents it. The operator reports that the
  same hash is present under `/opt/inst/cc2-reactor/` and
  `/opt/usr/cc2-reactor-before-firmware-4353/`, and that a `.so.bak` next
  to the latter has the hash `4639364c…`. That `.bak` hash equals a
  locally built file on the analysis machine
  (`~/reactor-analysis/libcc2-reactor-v2.so`), so the `.bak` probably
  came from that machine; this is a hash match on an operator-reported
  value, not a traced copy.
- Byte-level proof that nothing was ever written to the wrong addresses
  before the fingerprint check (if any) rejected the patch — inferred
  from process stability and the missing marker, not from a memory diff.

## 7. Fix verified

The `fallback()` bug from §4 is fixed directly on the affected device.
The matching repository change was made and then taken back; see
"Launcher fix and repository pin chain" below.

- **Device fix:** the exact same one-line change was applied directly to
  `/etc/init.d/cc2-reactor-launch` on the device, after backing up the
  original file locally first.
- **Test performed:** a real `run_printer.sh stop; run_printer.sh start`
  cycle — the same real start chain that previously failed — with
  `/opt/usr/cc2-reactor-disabled` still present, so the device went
  through the real, on-device `fallback()` path, not a simulation.
- **Result:**
  - `elegoo_printer` came up as a new, clean process instance, **PID
    2411**.
  - No `libcolib.so` error occurred anywhere in the startup log.
  - The full, normal vendor startup sequence completed through the real
    `fallback()` path: MCU/toolhead/bed-sensor configuration, bed mesh,
    input shaper, HTTP server on port 80, MQTT connection, Canvas
    firmware detection, filament sensor, device state reaching `IDLE`.
  - Confirmed afterward: `ps w | grep elegoo_printer` showed exactly one
    instance (PID 2411); `cat /proc/2411/environ | grep -i preload`
    returned nothing — `DONE_NO_PRELOAD_MEANS_EMPTY_ABOVE` printed with
    no `LD_PRELOAD` line above it, confirming the running process is in
    plain vendor mode, as intended.

**This closes the defect from §4:** the fixed `fallback()` now sets
`LD_LIBRARY_PATH` itself, exactly as the success path already did, so it
no longer depends on whether its caller sourced `/etc/profile`. The
normal boot path already worked before the fix (see the correction in
§4); what the fix changes is that a manual or scripted start outside a
profile-sourced shell now works too. The incident is resolved on the
device.

### Launcher fix and repository pin chain

The launcher fix was applied directly on the device and remains
effective there, independent of the repository state.

Commit `c0f9fc0b` on this fork branch made the same one-line change in
`builder/current/components/reactor/firmware-launcher.sh` and updated
`FIRMWARE_LAUNCHER_SHA256` in `builder/current/core/reactor_component.py`.
It did not update the pins that depend on those two files:

- `REACTOR_HELPER_SHA256` in `firmware_builder.py` no longer matched
  `reactor_component.py`.
- `connections_component.py` still pinned the installed
  `etc/init.d/cc2-reactor-launch` to the previous hash `8d4a19c9…`.

A real build from the branch would therefore have been refused. The host
test suite does not cover this chain, which is why it still passed.

The code part of `c0f9fc0b` has been reverted on this branch to restore
the pinned verification chain: both files are back at their previous
hashes (`8d4a19c9…` and `d9af4d96…`), and the changelog entry was
removed. No pin was changed.

Where this is tracked (all three entries were opened by the author of
this fork; contents checked on 2026-11-09):

- **Fork PR #1**
  (<https://github.com/lpgamerzockt-ops/centauri-carbon-2-community/pull/1>,
  "Document experimental reactor 02.01.01.13 address mapping") has this
  branch, `docs/reactor-0113-address-mapping`, as its head. Commit
  `c0f9fc0b` and its revert are therefore part of that pull request, and
  the reactor launcher belongs to its subject. It is the entry that
  tracks the launcher fix. Its description does not mention the fix yet.
- **Upstream issue #334**
  (<https://github.com/Neme77/centauri-carbon-2-community/issues/334>)
  is the hand-off for the port of the community patches (printer, GUI,
  camera) to 02.01.01.13. It is the umbrella for the 0113 work but does
  not describe the reactor launcher.
- **Upstream #336**
  (<https://github.com/Neme77/centauri-carbon-2-community/pull/336>) is a
  draft pull request, not an issue: tooling and documentation for that
  patch port (`experimental/firmware-port-0113/`), from the branch
  `update-2.03.03.13` that fork PR #1 is stacked on. It does not contain
  the launcher or this branch's commits.

The launcher fix is not applied in the repository. Applying it properly
means updating, in one reviewed change: the launcher, `FIRMWARE_LAUNCHER_SHA256`,
`REACTOR_HELPER_SHA256`, the Connections launcher pin,
the `INTEGRATION_HELPERS` pin of `connections_component.py`, and
`BUILDER_SHA256` in `launch_helpers/run_build.py`. Whether the Connections
runtime needs requalification because its launcher baseline changes has
not been checked. Whether the fix is carried in fork PR #1, added to
#336, or proposed upstream separately is the fork author's decision.

## Affected device paths

| Path | Role |
|---|---|
| `/etc/init.d/cc2-reactor-launch` | The launcher: checks hashes/payload, stages the runtime, and either `exec`s `elegoo_printer` with the reactor preloaded, or (via `fallback()`) without it. |
| `/opt/bin/elegoo_printer` | The target vendor binary the reactor hook patches into; must match the pinned `EXPECTED` hash for the launcher to proceed past its checks. |
| `/opt/lib/libcolib.so` | A vendor-supplied library `elegoo_printer` needs at load time; also hash-checked by the launcher, and the one `fallback()` fails to let it find (missing `LD_LIBRARY_PATH`). |
| `/opt/usr/cc2-reactor-runtime-v2/` | The active, staged reactor runtime directory: holds the currently loaded `libcc2-reactor-v2.so` and its own `SHA256SUMS`/marker files. |
| `/opt/inst/cc2-reactor/` | The payload source the launcher stages from into `/opt/usr/cc2-reactor-runtime-v2/` whenever the payload hash changes. |
| `/opt/usr/cc2-reactor-disabled` | The disable switch: a marker file whose mere presence routes the launcher to `fallback()` (reboot-persistent, since it is a file, not an environment variable). |

## Scope of this document

Documents a real incident on real hardware. The address-generation
mismatch (§1–§3) and the `fallback()` bug (§4) are both confirmed by
direct evidence, and the `fallback()` bug is fixed on the device and verified by a
real test (§7) — this incident is **resolved**, not merely worked
around. This does not change the "experimental, unqualified" status of
the 0113 port, does not update `VENDOR_SHA256`, and does not authorize
re-enabling the reactor hook or any further hardware action. The
provenance of the mismatched `8f78646d…` library (§6) remains
unestablished and is a separate open question from the `fallback()` bug.
