#!/usr/bin/env python3
"""Verify reactor port address mapping between old and new elegoo_printer binaries.

Reads port-manifest.json mapping (vendor_va -> community_va), looks up each
community address in the old binary's symbol table, finds the same symbol in
the new binary, and reports the delta.

Usage:
    python3 verify_mapping.py [path/to/port-manifest.json]

Expects old.sym and new.sym in the current directory (llvm-readelf -sW output).
"""
import json, re, sys, os

DEFAULT_MANIFEST = 'centauri-carbon-2-community/experimental/firmware-port/reactor/port-manifest.json'

def load_syms(path):
    syms = {}
    with open(path) as f:
        for line in f:
            # llvm-readelf -sW line format:
            # Num: Value Size Type Bind Vis Ndx Name
            m = re.match(r'\s*\d+:\s+([0-9a-fA-F]+)\s+\d+\s+(FUNC|OBJECT)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)', line)
            if m:
                addr = int(m.group(1), 16)
                name = m.group(6)
                syms.setdefault(addr, []).append(name)
    return syms

def main():
    manifest_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_MANIFEST
    if not os.path.exists(manifest_path):
        print(f'ERROR: manifest not found: {manifest_path}')
        sys.exit(1)
    if not (os.path.exists('old.sym') and os.path.exists('new.sym')):
        print('ERROR: old.sym / new.sym not found in current directory.')
        print('Create them with:')
        print('  llvm-readelf -sW old.elegoo_printer > old.sym')
        print('  llvm-readelf -sW new.elegoo_printer > new.sym')
        sys.exit(1)

    manifest = json.load(open(manifest_path))
    mapping = manifest.get('mapping', {})
    old = load_syms('old.sym')
    new = load_syms('new.sym')

    ok, warn, missing = 0, 0, 0
    print(f'{"community":>10}  {"new":>10}  {"delta":>9}  status  symbol')
    print('-' * 100)
    for vendor, comm in sorted(mapping.items(), key=lambda x: int(x[1], 16)):
        comm_addr = int(comm, 16)
        names = old.get(comm_addr, [])
        if not names:
            print(f'{comm:>10}  {"?":>10}  {"?":>9}  MISSING  (no symbol at this address in old)')
            missing += 1
            continue
        name = names[0]
        candidates = [a for a, ns in new.items() if name in ns]
        if not candidates:
            print(f'{comm:>10}  {"?":>10}  {"?":>9}  MISSING  {name[:60]}')
            missing += 1
            continue
        new_addr = candidates[0]
        delta = new_addr - comm_addr
        if delta == 0x2530:
            status = 'OK (+0x2530)'
            ok += 1
        elif delta == -0x226c:
            status = 'OK (get_monotonic)'
            ok += 1
        elif delta == -0xe4:
            status = 'OK (poll PLT)'
            ok += 1
        else:
            status = 'CHECK'
            warn += 1
        print(f'{comm:>10}  {new_addr:#10x}  {delta:+#9x}  {status:<18} {name[:60]}')

    print('-' * 100)
    print(f'OK: {ok}   CHECK: {warn}   MISSING: {missing}')
    if warn or missing:
        print('NOT all entries verified - do NOT blindly apply +0x2530.')
        sys.exit(2)
    print('All mapping entries verified.')

if __name__ == '__main__':
    main()
