#!/usr/bin/env python3
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--generated',type=Path,required=True)
parser.add_argument('--certificates',type=Path,default=Path('certificates'))
args=parser.parse_args()

for kind,stem in [('antipodes','phase1_antipodes'),('top2','phase1_top2')]:
    for block in range(20):
        name=f'{stem}_block_{block}.bin'
        a=args.generated/kind/name
        b=args.certificates/kind/name
        if not a.exists() or not b.exists():
            raise SystemExit(f'missing {a if not a.exists() else b}')
        if a.read_bytes()!=b.read_bytes():
            raise SystemExit(f'certificate mismatch: {kind} block {block}')
print('VERIFIED: regenerated antipode and top-two-layer certificates are byte-identical.')
