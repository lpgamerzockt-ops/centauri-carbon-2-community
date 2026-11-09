# Reactor address mapping: 02.01.01.07 → 02.01.01.13 `elegoo_printer`

## Inputs

| Binary | Base | SHA-256 | Role |
|---|---|---|---|
| `elegoo_printer` (old) | 02.01.01.07 | `19712b5943b10bb8cb6aab9a4ed498ee962b28538b23c91b7ed003eb9e590745` | Qualified Community V4.4.3 build — the pin currently in `builder/current/core/reactor_component.py`'s `VENDOR_SHA256`. Unchanged by this commit. |
| `elegoo_printer` (new) | 02.01.01.13 | `688b7bf8a27762d56b35d9738ea5b0c3d45580b402e122b0f77ee49dab1a8cfd` | This repository's own HTTP/LAN-WAN payload port + real-hardware crash fix output (see `../README_PORT.md`, `../RUNTIME_FINDINGS_20261108.md`). Contains no reactor code. |
| `libcolib.so` | both | `14288a4a73c339b039dd89597618ebb05702f90d668b76f74019d5d15ce67403` | Confirmed byte-identical across both bases (already noted elsewhere in this port's risk list) — not re-attached here. |

Neither vendor binary is attached in this directory, per
`../HANDOFF_PACKAGE_CHECKLIST.md` §3.

## Method

1. `llvm-readelf -sW` dumped the full symbol table of each binary
   (`old.sym`, `new.sym` — not attached; regenerable from the two hashes
   above with that one command each).
2. For each of the 22 addresses in the existing
   `experimental/firmware-port/reactor/port-manifest.json` `mapping`
   (02.01.01.07 community-side values), `verify_mapping.py` looked up the
   mangled symbol name at that address in `old.sym`, then searched `new.sym`
   for the same name and reported the new address and delta.
3. `apply_mapping.py` holds the resulting old→new address table as a fixed
   dict (reproduced in the table below) — it is a separate, explicit review
   step from the lookup in (2), not an automated accept-all.

## Result: 21 of 22 confirmed by symbol-name match, 1 by fixed offset

| Old VA (07) | New VA (.13) | Delta | Basis | Symbol / role |
|---|---|---|---|---|
| `0x35ee3c` | `0x35cbd0` | `-0x226c` | symbol match | `get_monotonic` |
| `0xbf70c` | `0xbf628` | `-0xe4` | **fixed offset, not symbol-matched** | `poll` PLT stub — PLT entries carry no stable symbol name in the table; mapped by the same `-0xe4` delta observed on the adjacent, symbol-confirmed entries, not independently verified |
| `0xc3ee8` | `0xc6418` | `+0x2530` | symbol match | `_ZNKSt14_Function_base8_M_emptyEv` |
| `0xcfd10` | `0xd2240` | `+0x2530` | symbol match | `_ZNSt10shared_ptrI12ReactorTimerED1Ev` |
| `0xd0460` | `0xd2990` | `+0x2530` | symbol match | `_ZNSt10shared_ptrI12ReactorTimerEC1ERKS1_` |
| `0xd1194` | `0xd36c4` | `+0x2530` | symbol match | `nlohmann::json` dtor (basic_json instantiation) |
| `0xdb05c` | `0xdd58c` | `+0x2530` | symbol match | `nlohmann::json` copy (basic_json instantiation) |
| `0x109e4c` | `0x10c37c` | `+0x2530` | symbol match | `nlohmann::json` const accessor (basic_json instantiation) |
| `0x1d62b4` | `0x1d87e4` | `+0x2530` | symbol match | `_ZNKSt8functionIFvdEEclEd` |
| `0x26a4ec` | `0x26ca1c` | `+0x2530` | symbol match | `_ZN17ReactorCompletion8completeE...` |
| `0x26a640` | `0x26cb70` | `+0x2530` | symbol match | `_ZN17ReactorCompletion4waitEdRK...` |
| `0x26ab5c` | `0x26d08c` | `+0x2530` | symbol match | `_ZN15ReactorCallback6invokeEd` |
| `0x26b8b8` | `0x26dde8` | `+0x2530` | symbol match | `_ZN13SelectReactor16unregister_timerESt10shared_ptrI12Reactor...` |
| `0x26c0c4` | `0x26e5f4` | `+0x2530` | symbol match | `_ZN13SelectReactor5pauseEd` |
| `0x26ccac` | `0x26f1dc` | `+0x2530` | symbol match | `_ZN13SelectReactor12check_timersEdb` |
| `0x26d3a0` | `0x26f8d0` | `+0x2530` | symbol match | `_ZN13SelectReactor12end_greenletESt10shared_ptrI15ReactorGreen...` |
| `0x26dddc` | `0x27030c` | `+0x2530` | symbol match | `_ZN13SelectReactor10getcurrentEv` |
| `0x26e3a8` | `0x2708d8` | `+0x2530` | symbol match | `_ZN11PollReactor13dispatch_loopEv` |
| `0x271c90` | `0x2741c0` | `+0x2530` | symbol match | `_ZNSt10shared_ptrI15ReactorGreenletEaSERKS1_` |
| `0x2720d4` | `0x274604` | `+0x2530` | symbol match | `_ZNSt6vectorISt10shared_ptrI15ReactorGreenletESaIS2_EE9push_...` |
| `0x272350` | `0x274880` | `+0x2530` | symbol match | `std::function<nlohmann::json(...)>::operator()` instantiation |
| `0x2738bc` | `0x275dec` | `+0x2530` | symbol match | `_ZNKSt8functionIFddEEclEd` |

`verify_mapping.py`'s summary line for this run: `OK: 21   CHECK: 0   MISSING: 1`
(the 1 "MISSING" is the expected `poll` PLT stub, called out above — the
script's own docstring and `apply_mapping.py`'s comment both document this
as the one address accepted without a symbol match, not an unexplained
gap).

The four reactor entry points patched by `reactor_module.cpp`'s
`activate()` — `ReactorCallback::invoke`, `SelectReactor::check_timers`,
`PollReactor::dispatch_loop`, `ReactorCompletion::wait` — are the four
`+0x2530` rows above with `old_size`/`new_size` unchanged (308/820/1400/600
bytes respectively, per the existing `port-manifest.json` structure); this
mapping does not by itself confirm
`instructions_equal_except_call_targets` for the new binary — that
re-comparison is part of the still-outstanding ABI re-verification in
`REACTOR_CAMERA_PORT_TASKS.md` item 3, not re-done here.

## What this does not establish

- Whether the RB-tree layout offsets and `shared_ptr`-ABI helper
  (`struct Shared` in `reactor_module.cpp`) still match the new binary's
  actual compiled layout.
- Whether `reactor_module.cpp`, rebuilt with the pinned GCC 6.5.0 / glibc
  2.23 toolchain against these new addresses, runs correctly on real ARM
  hardware.
- Whether the reactor hook actually activates against `688b7bf8…` today —
  it does not; see `../RUNTIME_FINDINGS_20261108.md` and
  `../RUNTIME_FINDINGS_20261109.md`.

No hash constant, in this repository or anywhere else, was changed by this
commit.
