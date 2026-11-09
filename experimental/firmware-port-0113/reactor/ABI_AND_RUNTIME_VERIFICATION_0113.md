# Reactor bridge ABI cross-check and isolated runtime self-test

Scope: three local, device-free checks requested as the next step after
`SUPPORT_FUNCTIONS_DIFF_0113.md` — (1) whether the struct-offset
assumptions `reactor_module.cpp` hardcodes are backed by the
already-verified instruction-identical vendor functions, (2) whether
virtual dispatch/vtables are relevant at all, and (3) an isolated runtime
self-test of the bridge's own logic with the vendor entirely mocked out.

## 1. Vtables: not applicable

`reactor_module.cpp` contains no `virtual` keyword and no vtable-pointer
access. Every vendor interaction is either a call through a hardcoded
function-pointer address (`original<F>(addr)(...)`) or a raw struct-field
read/write via the `at(ptr, offset)` helper. There is no virtual-dispatch
ABI surface for this bridge to be compatible or incompatible with.

## 2. Struct-offset cross-check against the already-verified vendor code

`reactor_module.cpp` hardcodes these field offsets on vendor objects
(`owner`/`self`/`node`, in bytes): `144`, `12`, `32`, `96`, `108`, `16`,
`264`, `248` (`240+8`), `20`, `8`, `24`. Rather than assume these still
hold, each was searched for in the four patch-target functions' own
disassembly (`invoke`/`check_timers`/`dispatch_loop`/`wait`), which
`INSTRUCTION_DIFF_0113.md` already proved instruction-identical between
02.01.01.07 and the 0113 target — a hit there means the exact byte
pattern computing that offset is confirmed unchanged, not merely assumed.

**Confirmed present, as a genuine object-field access (not a stack-frame
offset), inside an already-verified-identical function:**

| Offset | Found as | In |
|---|---|---|
| `144` | `add r2/r3/r4, r3, #144` (×10) | `dispatch_loop`, `check_timers` |
| `32` | `add r3,r3,#32` / `vldr d6,[r3,#32]` / `strd r2,r3,[r1,#32]` | `invoke`, `check_timers` |
| `16` | `add r2/r3,r3,#16` / `ldrd/strd [r3,#16]` | `invoke`, `check_timers`, `dispatch_loop` |
| `264` | `add r3,r3,#264` (×3) | `dispatch_loop` |
| `8` | `add r2/r3,r3,#8` (×4) | `invoke`, `wait` |
| `24` | `add r3/r4,r3,#24` (×5) | `wait` |

**Not confirmed by this specific cross-check — stated plainly, not
glossed over:**

| Offset | Why not confirmed |
|---|---|
| `96` | The only matches found (`sub r3, r11, #96` in `dispatch_loop`) are frame-pointer-relative local-variable accesses, not accesses to an owner/self object field — a false match for this method, not a disconfirmation. |
| `108` | The only matches found are branch-displacement immediates that happen to equal `0x6c` by coincidence, not field accesses. |
| `248` | No occurrence of this offset in any of the four already-verified functions at all — it is used only in the bridge's own `handler()`/`handler_object()` helpers, which operate on a part of the `owner` structure none of the four patched functions directly touch in their own bodies. |
| `20` | Same situation as `108`: the one textual match is a frame-relative local, not a field access. |

These four remain exactly what they were in the original 07 port: an
assumption carried over, not independently re-validated against the
0113 binary by this check. No instruction-level comparison exists yet
for whatever vendor function(s) *do* read/write offsets `96`, `108`,
`20`, `248` on these objects — that would require identifying those
functions first (they are not among the 22 addresses `reactor_module.cpp`
already names), which is out of scope for this entry.

## 3. Isolated runtime self-test (no vendor binary, no device)

The repository already ships exactly this test:
`experimental/firmware-port/reactor/bridge_test.cpp`, built with
`-DCC2_NATIVE_CO` per `prepare_reactor.py`. It mocks every vendor entry
point (`shared_ptr`/`json` helpers, `pause`, `poll`, `get_monotonic`, …)
with host-side C++ stand-ins and exercises the bridge's own compiled
logic — initialization (`Owner` construction, which places a
`std::vector<Shared>`/`Shared`/`std::vector<pollfd>` at exactly the
offsets above via the same `at()` helper the bridge uses), callback
registration, timer/poll/wait cycles, and clean teardown — entirely
independent of any real vendor binary or device.

**Run method:** the binary built in `HASH_LEDGER.md` §3
(`cc2-reactor-bridge-test`, SHA-256
`c4261c65d86b8a1d7807baea5dd2717224d69a483b9126b0114826141bfaaac2`) is an
ARM32 hard-float ELF; it was executed under `qemu-arm-static` (installed
in an image layered on top of the §2 toolchain image, as root, before
dropping back to the unprivileged `builder` user — the original build
container was not touched) with `QEMU_LD_PREFIX` set to the toolchain's
own sysroot and `LD_LIBRARY_PATH` pointing at the test's `vendor-link/`
directory for `libcolib.so`. No network, no device, no SSH.

**Result — real ARM32 execution, not emulation of the logic in the
abstract:**

```
PASS: 256 timer suspend/resume cycles preserve time and leave disabled timers inactive despite VFP register reuse
PASS: replacement bridge with 1024 native libco cycles, including 512 exceptions after suspension
PASS: replacement bridge callback/timer/poll/wait logic, refcounts, exception cleanup, reallocation and fd removal
LIMIT: vendor entry calls mocked; original ARM function binding is separately fingerprint-audited and requires runtime testing.
exit=0
```

This exercises, under real ARM32 hard-float execution with the
test-candidate toolchain's own compiled code: the `Shared`/`shared_ptr`
bridge refcounting (`static_assert(sizeof(Shared)==8)` and the
`offsetof` asserts in `bridge_test.cpp` itself all held at compile time),
C++ exception propagation across a suspended-coroutine boundary (512 of
the 1024 native cycles throw and are correctly caught after `co_yield`),
and VFP callee-saved register integrity across context switches (the
`d8`–`d15` poisoning in `floating_cycles()`).

**What this does and does not establish — the test's own printed line
says it directly:** "vendor entry calls mocked; original ARM function
binding is separately fingerprint-audited and requires runtime testing."
This confirms the bridge's *own* logic and ABI assumptions are internally
consistent and executable on real ARM32 hardware (emulated). It does
**not** exercise a single real vendor function call, does not touch
`688b7bf8…` or any other vendor binary, and is not a hardware or device
test of any kind.

## Status

- Vtables: not applicable (confirmed, not merely assumed).
- 6 of 10 hardcoded struct offsets (`144`, `32`, `16`, `264`, `8`, `24`):
  confirmed present inside already instruction-verified vendor code.
- 4 of 10 (`96`, `108`, `20`, `248`): not confirmed by this method,
  explicitly left open rather than assumed.
- Bridge-internal logic, refcounting, exception/unwind behavior, and VFP
  register handling: PASS under real ARM32 execution with the
  test-candidate toolchain, vendor calls mocked.
- Still **NOT RUN**: anything involving the actual `688b7bf8…` vendor
  binary, any device, any hash-gate or firmware-pin change.

## Scope of this document

Local, read-only analysis plus an isolated, device-free ARM32 emulated
self-test. No hash gate was loosened, no file was installed or changed
on any device, and no hardware or print test was started.
