#!/usr/bin/env python3
"""Apply verified address remapping to reactor_module.cpp and port-manifest.json.

Creates .bak backups before modifying. Only run after verify_mapping.py reports
21 OK / 1 MISSING (poll, expected).
"""
import re, shutil, os, sys

REPO = 'centauri-carbon-2-community/experimental/firmware-port/reactor'
CPP = os.path.join(REPO, 'reactor_module.cpp')
MANIFEST = os.path.join(REPO, 'port-manifest.json')

# verified mapping: old community address -> new address
MAPPING = {
    '0x35ee3c': '0x35cbd0',  # get_monotonic, -0x226c
    '0xbf70c':  '0xbf628',   # poll PLT, -0xe4
    '0x271c90': '0x2741c0',
    '0xcfd10':  '0xd2240',
    '0xd1194':  '0xd36c4',
    '0x26d3a0': '0x26f8d0',
    '0x26e3a8': '0x2708d8',
    '0x26ccac': '0x26f1dc',
    '0x26b8b8': '0x26dde8',
    '0x2738bc': '0x275dec',
    '0x26a640': '0x26cb70',
    '0x26c0c4': '0x26e5f4',
    '0x272350': '0x274880',
    '0x109e4c': '0x10c37c',
    '0x2720d4': '0x274604',
    '0x26dddc': '0x27030c',
    '0x26ab5c': '0x26d08c',
    '0xdb05c':  '0xdd58c',
    '0xd0460':  '0xd2990',
    '0x26a4ec': '0x26ca1c',
    '0xc3ee8':  '0xc6418',
    '0x1d62b4': '0x1d87e4',
}

def patch_file(path):
    if not os.path.exists(path):
        print(f'SKIP (not found): {path}')
        return
    backup = path + '.bak'
    shutil.copy2(path, backup)
    with open(path) as f:
        content = f.read()
    count = 0
    for old, new in MAPPING.items():
        # word-boundary regex to avoid partial matches
        pattern = re.compile(r'\b' + re.escape(old) + r'\b')
        content, n = pattern.subn(new, content)
        count += n
    with open(path, 'w') as f:
        f.write(content)
    print(f'PATCHED {count} occurrences in {path} (backup: {backup})')

def main():
    patch_file(CPP)
    patch_file(MANIFEST)
    print('\nDone. Review diffs with:')
    print(f'  diff {CPP}.bak {CPP}')
    print(f'  diff {MANIFEST}.bak {MANIFEST}')

if __name__ == '__main__':
    main()
