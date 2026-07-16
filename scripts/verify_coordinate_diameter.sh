#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
mkdir -p build logs generated_diameter
CXX="${CXX:-g++}"
THREADS="${DIAMETER_THREADS:-4}"
COMMON=(-std=c++20 -Wall -Wextra -Wpedantic -I src)
python3 src/generate_diameter_orbits.py \
  --output generated_diameter/position_orbit_representatives.txt \
  | tee logs/generate_diameter_orbits.log
cmp generated_diameter/position_orbit_representatives.txt \
  certificates/diameter_position_orbit_representatives.txt
"$CXX" -O3 -march=x86-64 -mtune=generic "${COMMON[@]}" \
  src/verify_coordinate_diameter.cpp -o build/verify_coordinate_diameter
build/verify_coordinate_diameter \
  --representatives generated_diameter/position_orbit_representatives.txt \
  --output generated_diameter/coordinate_diameter.json \
  --threads "$THREADS" | tee logs/verify_coordinate_diameter.log
cmp generated_diameter/coordinate_diameter.json certificates/coordinate_diameter.json
echo "COORDINATE DIAMETER VERIFIED"
