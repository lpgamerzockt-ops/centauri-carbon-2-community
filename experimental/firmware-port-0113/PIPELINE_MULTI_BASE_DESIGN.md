# Design draft: building for more than one qualified ELEGOO base

**Status: draft for review. No code, pin, profile or device is changed by
this document.** It proposes how `builder/current` could build for both
02.01.01.07 and 02.01.01.13 without weakening its fail-closed hash
pinning. It does not qualify 02.01.01.13; `ported_patch_set` stays
`false` and `QUALIFIED_BASE` stays `'02.01.01.07'`.

Line numbers refer to the tree at commit `ce3a2771`.

## 1. What the builder does today

The vendor base is selected by `builder/current/BASE_FIRMWARE` (currently
`02.01.01.13` on this branch, so a default build is refused) or
`--base-version`, and described by `base_profiles/<version>.json`.

**Already base-aware.** `main()` reads the vendor *input* pins from the
selected profile (`pins=profile['sha256']`, `firmware_builder.py:537`) and
checks the stock package, CPIO members, rootfs, GUI, printer and daemon
against them (`:590-607`). `decode_stock_package()` takes a `profile`
argument.

**Single-base by construction.** Everything else is bound to 02.01.01.07:

| Where | What | Kind |
|---|---|---|
| `firmware_builder.py:26-27,45,65,69-70` | `QUALIFIED_BASE`, `QUALIFIED_PROFILE_SHA256`, `require_buildable_profile()`, `QUALIFIED_PROFILE`, `_QPIN` | the gate |
| `firmware_builder.py:77-103` `EXPECTED` | vendor pins copied from the 07 profile at import time; `patched_gui` (`5f312285…`) is a 07 *output* | mixed |
| `:353-365` `patch_zoffset`, `patch_dual` | re-check `EXPECTED['stock_gui']`, `['patched_gui']`, `['daemon_stock']` instead of the selected profile | output + input |
| `:104-110` `GOLDEN` | byte-level release outputs of the 07 build | output |
| `:452-454` `PRINTER_RELEASE_SHA256/MD5/SIZE` | patched `elegoo_printer` (`19712b59…`) | output |
| `:486` `CAMERA_RELEASE_SHA256` | patched `ai_camera` (`ec5633b1…`) | output |
| `:487` `PORT_HASHES`, `PORT_DIR` | the patch modules and manifests in `experimental/firmware-port/`, whose offsets and before/after bytes are 07-only | patch set |
| `:416-420` HTTP upload v1 pins | older baseline; not on the 4.4 build path | legacy |
| `core/native_display_component.py:20`, `experimental/native-display/baseline-pins.txt` | native display GUI output (`db09fa37…`), built from the **07** Z-offset-patched GUI (`5f312285…`) | input + output |
| `core/camera_component.py:13-14` | `CAMERA_SHA`, `INIT_SHA` (vendor `etc/init.d/printer`) | input + output |
| `core/reactor_component.py:10-12`, `components/reactor/firmware-launcher.sh:11` | `VENDOR_SHA256`, `EXPECTED` (`19712b59…`), launcher pin | output |
| `core/connections_component.py:100-110` | `NATIVE` pins of vendor files (`wifi_daemon`, `aic8800_fdrv.ko`, `etc/init.d/printer`, …) and `lib/modules/5.4.61/cc2` kernel modules | input |
| `launch_helpers/run_build.py:27-42` | its own profile check; requires `ported_patch_set` but not a qualified version | weaker gate |
| `tests/test_base_profiles.py` | asserts that a complete 02.01.01.13 profile is refused even with `ported_patch_set: true` | test |

Base-independent pins (`sshd*`, `aes_key`, patcher tools, `sig_tool`,
SquashFS tools, signing keys) need no change.

### The pin chain

Helper files are themselves pinned by their consumers:
`REACTOR_HELPER_SHA256`, `CAMERA_HELPER_SHA256` and `INTEGRATION_HELPERS`
pin the component helpers inside `firmware_builder.py`;
`run_build.py:18` pins `firmware_builder.py` (`BUILDER_SHA256`);
`connections_component.py:107` pins the installed
`etc/init.d/cc2-reactor-launch`.

This chain is not covered end to end by the host tests. Commit
`c0f9fc0b` on this branch changed `firmware-launcher.sh` and
`reactor_component.py`; the test suite still passed, but
`REACTOR_HELPER_SHA256` (`d9af4d96…`) and the Connections launcher pin
(`8d4a19c9…`) no longer matched, so a real build on this branch would
have been refused. The code part of that commit has since been reverted
(phase 0); the launcher fix itself is tracked in fork PR #1 (see
`reactor/INCIDENT_0113_ADDRESS_MISMATCH.md`).

## 2. Scope correction: what a 02.01.01.13 release needs

`base_profiles/02.01.01.13.json` names three patches for
`ported_patch_set` (Z-offset, HTTP/network, camera). The build path
needs more than those, and none of the following has been checked
against 02.01.01.13 in this repository:

- **Native display.** The release GUI is not the Z-offset-patched GUI
  but the native Community display built on top of it. Its baseline pins
  are the 07 binary and images.
- **Connections.** `NATIVE` pins vendor files, and the USB-Ethernet
  kernel modules were built for the 07 kernel image; the 02.01.01.13
  kernel has a different hash (same version string 5.4.61).
- **Camera runtime.** `INIT_SHA` pins the vendor `etc/init.d/printer`.
- **Reactor.** Bound to the 07 `elegoo_printer`; the 0113 mapping is
  experimental and stays disabled.
- **CC2 Control.** `cc2-control/installer/compatibility.json` refuses
  other `elegoo_printer`/`daemon-000` hashes.

`HARDWARE_CONFIRMATION_0113.md` covers only four binaries (GUI with the
Z-offset patch, camera, printer, Dual Trust daemon). It says nothing
about the items above.

## 3. Proposed structure

### 3.1 A qualified *set*, pinned in code

```python
QUALIFIED_BASES = {            # version -> SHA-256 of base_profiles/<version>.json
    '02.01.01.07': '<pin>',
}
```

`load_base_profile()` checks the file hash for every version in the set.
`require_buildable_profile()` requires `version in QUALIFIED_BASES`,
`ported_patch_set is True`, and no placeholders. A base becomes buildable
only through a reviewed code change that adds its profile hash; editing a
JSON file alone can never qualify a base. `run_build.py` uses the same
rule instead of its own weaker copy.

### 3.2 Base-specific outputs move into the pinned profile

Each profile gains a section next to `sha256` (vendor inputs):

```json
"port": {
  "dir": "experimental/firmware-port",
  "files": { "patch_http44.py": "<sha256>", "http44-manifest.json": "<sha256>", "...": "..." },
  "outputs": {
    "patched_gui": "<sha256>",
    "printer_release": { "sha256": "...", "md5": "...", "size": 0 },
    "camera_release": "<sha256>",
    "daemon_dual": "<sha256>",
    "native_gui": "<sha256>"
  },
  "components": { "camera_init": "<sha256>", "connections_native": { "...": "..." } },
  "golden": { "rootfs": "...", "swu": "...", "swusig": "...", "zip": "...", "outer": "..." }
}
```

Because the whole profile file is hash-pinned in code (3.1), one pin per
base covers every base-specific value. The loader validates the exact key
set, as it does for `VENDOR_PIN_KEYS` today.

### 3.3 One immutable build context instead of module globals

`main()` resolves the profile once into a frozen context (version, vendor
pins, port directory, port file pins, output pins, component pins) and
passes it down. Functions that read globals today take the context:
`patch_zoffset`, `patch_dual`, `patch_printer44`, `validate_port_inputs`,
`port_module`, `golden_report`, and the `load`/`install`/`audit`
functions of the camera, native display, reactor and Connections
components.

`EXPECTED` keeps only base-independent entries. `_QPIN`,
`QUALIFIED_PROFILE`, `PRINTER_RELEASE_*`, `CAMERA_RELEASE_SHA256`,
`PORT_HASHES` and `GOLDEN` disappear as globals.

### 3.4 One patch-set directory per base

`experimental/firmware-port/` stays as it is for 02.01.01.07. A ported
set lives in its own directory with its own modules and manifests,
referenced by the profile's `port.dir`. No manifest is shared or
parameterised across bases.

### 3.5 Components that are not qualified for a base

If a component (the reactor, for 02.01.01.13) is not qualified for a
base, the profile says so explicitly, the builder skips its install, and
the audit asserts its absence. Silent omission is not allowed. Whether a
release without the reactor is acceptable is a maintainer decision
(section 7).

## 4. Compatibility with 02.01.01.07

The refactor is done first with only `02.01.01.07` in `QUALIFIED_BASES`.
It is accepted only if:

- every existing host test passes, with changes limited to names that
  moved;
- the values now read from the 07 profile are character-for-character the
  constants removed from the code (a test compares them against a
  frozen copy);
- a full 07 build still reproduces `GOLDEN`, checked by the maintainer
  with `--require-golden` on the reference build host;
- 02.01.01.13 is still refused.

Adding the `port` section changes the 07 profile file, so its pinned hash
changes once, in the same reviewed commit.

## 5. Security boundaries

1. **Fail closed.** A missing key, unknown base, or absent component pin
   raises; there is no default and no fallback to another base's value.
2. **No cross-base mixing.** All pins used in one build come from one
   context. Error labels include the base version.
3. **Qualification is a code change.** See 3.1.
4. **Expected values are never derived from build results.** The builder
   only compares; it does not record a new pin.
5. **Selection cannot bypass the gate.** `BASE_FIRMWARE`,
   `--base-version` and `run_build.py` all go through
   `require_buildable_profile()`.
6. **The pin chain is tested.** A new host test recomputes every
   helper/launcher hash and compares it with the pin that consumes it,
   so a change like `c0f9fc0b` fails the suite instead of a later build.
7. **No vendor material in the repository.** Profiles hold hashes only.
8. **Evidence fields are not gates.** `ported_patch_set` documents a
   maintainer decision; hardware evidence is linked from the profile
   comment, not inferred by the builder.

## 6. Phases and tests

| Phase | Content | Exit criterion |
|---|---|---|
| 0 | Restore a consistent pin chain on this branch: the code part of `c0f9fc0b` is reverted; the launcher fix stays a separate, not yet applied change in this fork, tracked in fork PR #1 | done by revert; the pin-chain test of phase 1 keeps it that way |
| 1 | Refactor to context + `QUALIFIED_BASES`, 07 only | section 4 |
| 2 | Add the `port` section to the 0113 profile with `ported_patch_set: false`; commit the 0113 patch-set directory | 0113 refused; static tests for the 0113 manifests pass |
| 3 | Check and port the remaining modules of section 2; emulation tests per patch, modelled on `experimental/firmware-port/test_*44_emulation.py` | emulation tests pass; outputs match the profile |
| 4 | Hardware tests per `docs/TESTING.md`, on a build produced by the builder, not hand-installed binaries | recorded results |
| 5 | Maintainer adds 02.01.01.13 to `QUALIFIED_BASES` and sets `ported_patch_set` | release decision |

Host tests to add or change:

- `test_base_profiles.py`: unknown base refused; qualified base with a
  modified profile file refused; qualified base with
  `ported_patch_set: false` refused; a base with `ported_patch_set: true`
  but absent from `QUALIFIED_BASES` refused (the current 0113 assertion,
  kept).
- New: profile `port` section schema; missing or extra key refused.
- New: no base-specific constant remains at module level (static check
  for the removed names).
- New: pin-chain consistency (boundary 6).
- New: two contexts built in one process do not share state.
- `test_44_port_static.py`, `test_reactor_firmware.py`,
  `test_camera_firmware.py`, `test_native_display_firmware.py`,
  `test_connections_firmware.py`: read pins through the context.
- Launcher tests: `run_build.py` refuses the same cases as the builder.

## 7. Open questions

1. Is a per-profile `port` section acceptable, or should outputs live in
   a separate pinned file per base?
2. May a 02.01.01.13 release ship without the reactor, and how is that
   reflected in the firmware version family (`4.4.x` vs a new family)?
3. Do the Connections kernel modules load on the 02.01.01.13 kernel, or
   do they need a rebuild?
4. Should 02.01.01.07 remain buildable after 02.01.01.13 is qualified,
   and for how long?
5. Where should `GOLDEN` for a new base come from (first reference build
   on the maintainer's host)?
