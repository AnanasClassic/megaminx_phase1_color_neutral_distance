#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p build logs generated_reproduction
CXX="${CXX:-g++}"
COMMON=(-std=c++20 -Wall -Wextra -Wpedantic -I src)
python3 - <<'PY'
import sympy
if sympy.__version__ != "1.14.0":
    raise SystemExit(f"SymPy 1.14.0 is required; found {sympy.__version__}")
print("VERIFIED: SymPy 1.14.0 is available.")
PY
"$CXX" -O3 -march=x86-64 -mtune=generic "${COMMON[@]}" src/generate_phase1_tables.cpp -o build/generate_phase1_tables
"$CXX" -O3 "${COMMON[@]}" src/self_test.cpp -o build/self_test
"$CXX" -O3 "${COMMON[@]}" src/verify_csp_independent.cpp -o build/verify_csp_independent
build/self_test | tee logs/reproduce_self_test.log
rm -rf generated_reproduction
mkdir -p generated_reproduction
# Four independent workers, five target blocks each. Typical total RAM is below 300 MiB.
pids=()
for first in 0 5 10 15; do
  last=$((first+5))
  build/generate_phase1_tables --out generated_reproduction --first "$first" --last "$last" \
    >"logs/generate_${first}_${last}.log" 2>&1 &
  pids+=("$!")
done
for pid in "${pids[@]}"; do wait "$pid"; done
cat logs/generate_0_5.log logs/generate_5_10.log logs/generate_10_15.log logs/generate_15_20.log
python3 scripts/compare_certificates.py --generated generated_reproduction --certificates certificates
python3 src/verify_csp.py --certificates generated_reproduction --output generated_reproduction/csp_solutions_python.json \
  | tee logs/reproduce_csp_python.log
build/verify_csp_independent --certificates generated_reproduction --output generated_reproduction/csp_solutions_cpp.json \
  | tee logs/reproduce_csp_cpp.log
cp certificates/witness.json generated_reproduction/witness.json
python3 src/verify_witness.py --certificates generated_reproduction --output generated_reproduction/witness_verification.json \
  | tee logs/reproduce_witness.log
python3 src/verify_witness_python.py --certificates generated_reproduction --output generated_reproduction/witness_verification_python.json \
  | tee logs/reproduce_witness_python.log
python3 src/verify_reachability_group.py --certificates generated_reproduction --output generated_reproduction/witness_group_audit.json \
  | tee logs/reproduce_reachability_group.log
./scripts/verify_coordinate_diameter.sh
echo "FULL REPRODUCTION COMPLETE"
