# Resolving the open ABI blockers from ABI_VENDOR_INTERFACE_VERIFICATION_0113.md

For each of the three remaining blockers (offset `108`, offsets `20`/`248`,
the exception/unwind boundary): the assumption, how the responsible code
was identified, the disassembly evidence, the comparison against
02.01.01.07, and the result. Anything still unclear stays a **BLOCKER**,
not "probably compatible."

## Offset `108` (async-callback vector) — **now RESOLVED**

- **Assumption:** `at(owner, 108)` holds the `std::vector<Shared>` that
  `SelectReactor::register_async_callback` pushes an async callback onto.
- **Identification method:** the previous entry found that
  `register_async_callback` defers the actual work into a lambda,
  queued via `std::queue<std::function<void()>>::push`. That lambda's
  const call operator has its own mangled symbol:
  `...register_async_callback(...)::{lambda()#1}::operator()() const`,
  present at old `0x26bac4` / new `0x26dff4` (both size 120 bytes).
- **Disassembly evidence:**
  ```
  old@26bafc: mov  r3, r0                      ; r3 = SelectReactor* (via shared_ptr::operator->)
  old@26bb00: add  r3, r3, #108                 ; <-- direct object-field access
  old@26bb04: sub  r2, r11, #12
  old@26bb08: mov  r1, r2
  old@26bb0c: mov  r0, r3
  old@26bb10: bl   vector<shared_ptr<ReactorCallback>>::push_back
  ```
  The identical sequence (same registers, same immediate, same call
  target symbol) appears in the new build at the uniformly shifted
  addresses (`26bafc`→`26e02c`, `26bb00`→`26e030`, …).
- **Comparison with 02.01.01.07:** full instruction-by-instruction
  comparison of both 120-byte functions: 23 instructions identical
  as-is, 7 differ only in branch-target address (resolving to the same
  symbol on both sides) — the same method as
  `INSTRUCTION_DIFF_0113.md`/`SUPPORT_FUNCTIONS_DIFF_0113.md`, no
  content differences at all.
- **Result: CONFIRMED.** Offset `108` is a genuine, unchanged
  object-field access in an instruction-identical function.

## Offsets `20` / `248` (RB-tree node value / root pointer) — **still a BLOCKER**

- **Assumption:** `at(owner, 248)` holds the root pointer of a
  `std::map`-like structure keyed by file descriptor, whose nodes expose
  the mapped `Shared` value at node-relative offset `20`.
- **Identification method:** searched the vendor symbol table for
  plausible fd-registration functions and found
  `SelectReactor::register_fd(int, std::function<void(double)>, std::function<void(double)>)`
  (old `0x26c630`, new `0x26eb60`, both size 152 bytes) as the most
  likely candidate — the function that associates a file descriptor
  with read/write handlers.
- **Disassembly evidence:** the function **is** instruction-identical
  between the two builds (31 instructions identical, 7 differ only by
  branch-target address to the same symbol — no content differences).
  However, reading its body shows it does **not** perform a map/tree
  insertion directly:
  ```
  old@26c660: ldr  r3, [r11, #-0x1c]   ; r3 = the "read" std::function argument
  old@26c664: ldr  r3, [r3]           ; dereference std::function's internal storage
  old@26c668: add  r3, r3, #16        ; offset into std::function's internal layout
  old@26c66c: ldr  r4, [r3]           ; load a function pointer from it
  ...
  old@26c690: blx  r4                 ; invoke it indirectly
  ```
  This is `std::function`'s own internal invocation mechanism (calling
  the stored callable), not a tree/map operation. `register_fd` builds a
  `ReactorFileHandler` (via `make_shared`, line 18) and invokes the
  caller-supplied callback through it, but the actual insertion of that
  handler into the structure `handler()` later traverses (at offsets
  `20`/`248`) happens somewhere else — either inside the
  `make_shared<ReactorFileHandler>` constructor itself, inside the
  caller of `register_fd`, or in a function this search did not locate.
- **Result: NOT RESOLVED.** `register_fd` itself is confirmed
  instruction-identical, which is useful corroborating context, but it
  does not establish anything about offsets `20`/`248` specifically,
  because it does not touch them. **Remains a BLOCKER** — the actual
  insertion site is still unidentified, and "the nearby function I could
  find is unchanged" is explicitly not treated as equivalent to
  confirming the offsets themselves.

## Exception/unwind boundary between vendor-compiled and bridge-compiled frames — **partially addressed, still not a full PASS**

- **Assumption:** a C++ exception thrown inside vendor-compiled code
  (e.g. inside `SelectReactor::pause`, called from this session's own
  `cc2_wait`) can unwind correctly through a mixed vendor/bridge call
  stack.
- **Method:** checked, on the actual vendor binaries (not this
  session's own toolchain output), which personality routine and unwind
  mechanism the vendor binary itself uses, and compared it to what this
  session's candidate-toolchain-built bridge (`libcc2-reactor-v2.so`)
  uses.
- **Evidence:**
  ```
  old.elegoo_printer relocations: __gxx_personality_v0@CXXABI_1.3  (R_ARM_JUMP_SLOT)
  libcc2-reactor-v2.so relocations: __gxx_personality_v0@CXXABI_1.3  (R_ARM_JUMP_SLOT)
  ```
  Both resolve to the **same** personality-routine symbol — the
  standard Itanium/GCC C++ exception personality, table-based
  (`.ARM.exidx`/`.ARM.extab`), **not** the alternative SJLJ
  (`__gxx_personality_sj0`) mechanism. The vendor binary's `.ARM.exidx`
  section is present and substantial (`0x44348` bytes), confirming
  table-based unwind data exists throughout.
- **Result: the unwind *mechanism* matches (table-based, same
  personality routine) — this is new, concrete evidence, not merely
  assumed.** It is **not** a full proof of interoperability: this check
  does not confirm the LSDA (exception-table) encoding details, RTTI
  `type_info` comparison semantics, or stack-frame-layout assumptions
  the personality routine itself relies on are identical bit-for-bit
  across whatever two (possibly different) compiler versions actually
  produced the vendor binary and this session's candidate toolchain's
  output. **Still marked a BLOCKER for full confidence**, but the
  specific, previously-unknown risk ("might be using an entirely
  different, incompatible unwind scheme such as SJLJ") is now ruled out
  rather than merely assumed away.

## Updated status

| Area | Status |
|---|---|
| Offset 96 | CONFIRMED (prior entry) |
| Offset 108 | **CONFIRMED (this entry)** |
| Offset 20 | BLOCKER — responsible code still not located |
| Offset 248 | BLOCKER — responsible code still not located |
| Exception/unwind mechanism (personality routine, table vs. SJLJ) | **Narrowed (this entry): same mechanism confirmed** — full LSDA/RTTI-level compatibility still unconfirmed |

8 of 11 hardcoded struct offsets are now confirmed
(`144, 32, 16, 264, 8, 24, 96, 108`); `20` and `248` remain open. No
hardware test is proposed or authorized by this document.

## Scope of this document

Local, read-only analysis only. No hash gate was loosened, no file was
installed or changed on any device, no hardware or print test was
started, and nothing was pushed.
