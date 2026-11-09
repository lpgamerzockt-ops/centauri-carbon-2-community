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

## 4. The `fallback()` bug: confirmed, and still unfixed on disk

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

**`/etc/init.d/cc2-reactor-launch` itself was not edited.** Re-read after
the incident, its `md5sum` is `9e15e1116264c2438b388f7052db57ac` and its
`fallback()` body is byte-for-byte the same as quoted above — still
missing the `LD_LIBRARY_PATH` export. This is an identical bug to the one
already present in this repository's own committed
[`builder/current/components/reactor/firmware-launcher.sh`](../../../builder/current/components/reactor/firmware-launcher.sh),
whose `fallback()` has the same shape and the same missing export.

**Consequence, stated plainly: a real reboot of this device right now
would hit this same bug again and fail to start `elegoo_printer` at
boot**, because the disable marker routes every boot through the broken
`fallback()` path. This is not resolved by anything done so far — only
worked around for the single current (still-running) process instance.

## 5. Confirmed current device state (as of this incident)

- `elegoo_printer` running, single instance, no `LD_PRELOAD` in its
  environment (`cat /proc/<pid>/environ | grep -i preload` empty).
- `/opt/bin/elegoo_printer` on disk: unmodified, hash
  `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd`
  (matches the hash `cc2-reactor-launch` itself pins as `EXPECTED`).
- `/opt/usr/cc2-reactor-disabled` present → future boots take the
  `fallback()` path (and will currently fail there — see §4).
- Reactor hook not activated at any point checked (no
  `activation.marker`).

## 6. What is not established

- The provenance of the `8f78646d…` library — which build, from where,
  installed when. Not claimed here, and no earlier conversation record in
  this repository documents it.
- Byte-level proof that nothing was ever written to the wrong addresses
  before the fingerprint check (if any) rejected the patch — inferred
  from process stability and the missing marker, not from a memory diff.
- A fix for the `fallback()` `LD_LIBRARY_PATH` bug — not applied here,
  neither on the device nor in this repository's own copy of the same
  bug in `firmware-launcher.sh`. Left as a known, open, reproducible
  defect.

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

Documents a real incident on real hardware, already resolved for the
*current* running process only. Does not change the "experimental,
unqualified" status of the 0113 port, does not update `VENDOR_SHA256`,
and does not authorize re-enabling the reactor hook or any further
hardware action. The reboot-time `fallback()` bug remains open and
should be fixed (in `firmware-launcher.sh` and, separately, on this
device) before the disable marker is relied on across a reboot.
