#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p build logs generated_quick
CXX="${CXX:-g++}"
COMMON=(-std=c++20 -Wall -Wextra -Wpedantic -I src)
python3 - <<'PY'
import sympy
if sympy.__version__ != "1.14.0":
    raise SystemExit(f"SymPy 1.14.0 is required; found {sympy.__version__}")
print("VERIFIED: SymPy 1.14.0 is available.")
PY
"$CXX" -O3 "${COMMON[@]}" src/self_test.cpp -o build/self_test
"$CXX" -O3 "${COMMON[@]}" src/verify_csp_independent.cpp -o build/verify_csp_independent
build/self_test | tee logs/self_test_quick.log
python3 src/verify_csp.py --certificates certificates --output generated_quick/csp_solutions_python.json \
  | tee logs/verify_csp_python.log
build/verify_csp_independent --certificates certificates --output generated_quick/csp_solutions_cpp.json \
  | tee logs/verify_csp_cpp.log
python3 src/verify_witness.py --certificates certificates --output generated_quick/witness_verification.json \
  | tee logs/verify_witness.log
python3 src/verify_witness_python.py --certificates certificates --output generated_quick/witness_verification_python.json \
  | tee logs/verify_witness_python.log
python3 src/verify_reachability_group.py --certificates certificates --output generated_quick/witness_group_audit.json \
  | tee logs/verify_reachability_group.log
python3 - <<'PY'
import json
from pathlib import Path
p=json.loads(Path('generated_quick/csp_solutions_python.json').read_text())
c=json.loads(Path('generated_quick/csp_solutions_cpp.json').read_text())
a=[x['edge_piece_to_position'] for x in p['global_relaxed_edge_configurations']]
b=[x['edge_piece_to_position'] for x in c['configurations']]
assert a==b
print('VERIFIED: the two exhaustive CSP implementations return the same five configurations.')
PY
cmp generated_quick/csp_solutions_python.json certificates/csp_solutions_python.json
cmp generated_quick/csp_solutions_cpp.json certificates/csp_solutions_cpp.json
cmp generated_quick/witness_verification.json certificates/witness_verification.json
cmp generated_quick/witness_verification_python.json certificates/witness_verification_python.json
cmp generated_quick/witness_group_audit.json certificates/witness_group_audit.json
echo "QUICK VERIFICATION COMPLETE"
