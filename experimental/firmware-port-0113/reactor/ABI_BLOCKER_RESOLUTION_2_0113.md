# Resolving offsets 20/248 — follow-up to ABI_BLOCKER_RESOLUTION_0113.md

`ABI_BLOCKER_RESOLUTION_0113.md` identified `SelectReactor::register_fd`
as instruction-identical but found it does not itself touch offsets
`20`/`248` — it only invokes the caller-supplied `std::function` handler.
This entry searched further, specifically for the `std::map<int,
std::shared_ptr<ReactorFileHandler>>` instantiation's own construction
and insertion code, using the real vendor symbol table rather than
guessing.

## Method: find every template instantiation for this exact map type

Searched both symbol tables for every mangled symbol containing
`ReactorFileHandler` combined with `map`/`Rb_tree`/`operator[]`/`insert`.
This surfaced the map's own `operator[]`
(`std::map<int,shared_ptr<ReactorFileHandler>,...>::operator[]`, old
`0x273bf8`/new `0x276128`, both 320 bytes) and the RB-tree's own
`_M_get_insert_unique_pos` (old `0x27bb30`/new `0x27e060`, both 412
bytes) — the actual libstdc++ functions that create and position a new
node, and `PollReactor`'s own constructor (old `0x26df1c`/new
`0x27044c`, both 256 bytes), which is where the map member is
initialized.

## Offset `240` (map member base inside `PollReactor`) — **CONFIRMED**

- **Assumption:** `at(owner, 240)` is the `std::map<int,
  shared_ptr<ReactorFileHandler>>` member inside the `PollReactor`
  object (`owner` in `reactor_module.cpp`'s sense is a `PollReactor*`).
- **Disassembly evidence (`PollReactor::PollReactor()`, old binary):**
  ```
  26df44: ldr  r3, [r11, #-0x28]   ; r3 = this
  26df48: add  r3, r3, #240        ; <-- direct object-field access
  26df4c: mov  r0, r3
  26df50: bl   map<int,shared_ptr<ReactorFileHandler>,...>::map()
  ```
  The identical sequence (same registers, same immediate `#240`, same
  call target symbol) is present in the new build at the uniformly
  shifted addresses.
- **Comparison with 02.01.01.07:** the full 256-byte constructor is
  instruction-identical between builds (44 instructions identical as-is,
  12 differ only by branch-target address to the same symbol, 7 are the
  already-established benign diagnostic-string pattern — resolved
  individually below — and one apparent difference in the trailing
  literal-pool `.word` data is the same kind of parsing artifact
  documented in `SUPPORT_FUNCTIONS_DIFF_0113.md`, not a real
  instruction).
- **Diagnostic strings resolved** (not discarded): the constructor's own
  `__func__` name (`"PollReactor"`) and its success message
  (`"create PollReactor success!"`) are identical in both builds; only
  the `__FILE__` build-path pointer differs, which is the same,
  already-documented diagnostic-only difference seen throughout this
  port's analysis.
- **Result: CONFIRMED**, by direct disassembly, not assumed.

## Offset `248` (`240 + 8`, RB-tree root) — **supported by library-ABI stability, not by a direct dereference; not claimed as fully CONFIRMED**

- Neither `map::operator[]` nor `_M_get_insert_unique_pos` contain a
  literal `#248` (or `#8`-from-the-map-base) instruction in either
  build — both call libstdc++ helper functions
  (`_M_begin()`/`_M_end()`/`_M_get_insert_unique_pos` itself) that
  encapsulate the header/root access rather than computing it inline at
  the call site this search inspected. No vendor or application code was
  found that computes this `+8` offset directly and visibly in
  disassembly.
- What **is** established: (1) the map's base is confirmed at `owner +
  240` (above); (2) the `+8` relationship is not application-specific —
  it is fixed by libstdc++'s own `_Rb_tree_header`/`_Rb_tree_node_base`
  layout for a given libstdc++ version, the same for every `std::map` in
  the binary; (3) both the old and the 0113 binary were already
  confirmed (`ABI_VENDOR_INTERFACE_VERIFICATION_0113.md`) to report the
  same maximum `GLIBCXX_*` symbol version ceiling, i.e. built against the
  same library ABI generation.
- **This is a narrower, more defensible claim than "offset 248 is
  confirmed":** it says the `+8` constant is a library-ABI fact shared
  by both binaries' C++ standard library, not an application-specific
  value that could have drifted between the 07 and 0113 builds
  independently of the library. It is **not** the same as having found
  and compared an actual instruction that dereferences `owner+248`.
- **Status: upgraded from "function not identified" to "indirectly
  well-supported," but still short of a direct disassembly CONFIRMED.**
  Treat it as the weaker of the two remaining items, not as resolved.

## Offset `20` (node-relative `Shared` value) — **still not directly confirmed**

No additional direct-dereference evidence for this specific
node-relative offset was found in this pass either; the same
library-ABI argument applies (the node's value-member offset after its
`_Rb_tree_node_base` header is a libstdc++ layout fact, shared across
both binaries' identical library version), but, as with `248`, this is
not the same as a direct instruction-level confirmation.

## Updated status

| Offset | Status |
|---|---|
| 240 | **CONFIRMED** (direct disassembly, this entry) |
| 248 | Indirectly well-supported (library-ABI argument); not directly confirmed |
| 20 | Indirectly well-supported (library-ABI argument); not directly confirmed |

9 of 11 hardcoded offsets now have direct disassembly confirmation
(`144, 32, 16, 264, 8, 24, 96, 108, 240`); the remaining two (`20`, `248`)
rest on a library-ABI argument rather than a located dereference
instruction — reported as such, not upgraded to "confirmed."

No hardware test is proposed or authorized by this document.

## Scope of this document

Local, read-only analysis only. No hash gate was loosened, no file was
installed or changed on any device, no hardware or print test was
started, and nothing was pushed.
