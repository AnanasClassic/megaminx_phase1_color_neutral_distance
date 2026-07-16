# Exact worst-case color-neutral distance for Megaminx phase 1

This repository is the reproducibility package for the forthcoming paper

> **The Exact Worst-Case Color-Neutral Distance for the First Block Phase of the Megaminx**

## Result

For a fixed target corner, the first phase solves that corner and its three
incident edges. Its face-turn-metric coordinate graph has 11,692,800 states.
The solved coordinate has eccentricity 10, with 159 states at distance 10; the
ordinary graph diameter is 11. If the target corner may be selected among all
20 corners, the exact worst-case color-neutral distance is

```text
D_CN = 9.
```

The upper bound is established by exhaustive compatibility enumeration of the
20 depth-10 sets. After all corner constraints are discarded, exactly five
global edge assignments remain, and all five have odd edge-permutation parity.
The lower bound is certified by a reachable full cubie state whose 20 target
distances are all 9.

Conditional on the previously reported 114-move FTM upper bound, replacing its
first-phase contribution 10 by the exact color-neutral value 9 gives the
corresponding conditional upper bound 113. This repository independently
audits the new phase-1 result. It does not reproduce the large computation
underlying 114.

## Author

- Vladislav Kuznetsov
- Moscow Institute of Physics and Technology (MIPT), Dolgoprudny, Russia
- vladkuznecov266@gmail.com

## Proof outline

For the upper bound, the verifier combines the 20 fixed-target depth-10 sets.
Even after relaxing the problem by discarding all corner constraints, the
compatibility search leaves only five global edge assignments. Every one has
odd edge-permutation parity and is therefore unreachable. Thus every legal
Megaminx state has at least one target block at distance at most 9.

For the matching lower bound, `certificates/witness.json` describes a reachable
full cubie state. Two independent layer-membership implementations find all 20
target distances to be exactly 9, and a separate Schreier--Sims calculation
checks direct membership in the face-turn group.

No trained model or heuristic search is part of the trusted proof.

## Fast verification

Requirements: Python 3.10+, a C++20 compiler, and SymPy 1.14 for the independent Schreier--Sims reachability audit. Install the latter with
`python3 -m pip install -r requirements-audit.txt`.

```bash
python3 -m pip install -r requirements-audit.txt
make test
# Equivalent: make quick-verify
```

This checks the coordinate implementation, independently enumerates the five
relaxed configurations in both Python and C++, checks their parity, verifies the
distance-9 witness with two layer-membership implementations, and independently
checks witness reachability by Schreier--Sims group membership.

## Full reproduction

```bash
make reproduce
```

The full run rebuilds all 20 BFS tables, totaling 233,856,000 visited quotient
coordinates across the 20 isomorphic targets, and requires every generated
certificate to be byte-identical to the distributed version. It also performs
the orbit-reduced exhaustive check that the ordinary coordinate-graph diameter
is 11. Four table-generation workers use roughly 300 MiB of RAM on the audited
system; the diameter check has a separate `--threads` setting.

The ordinary diameter check can also be run separately with `make diameter`.
Use `make integrity` to check the SHA-256 manifest. A containerized fast audit
is available with:

```bash
docker build -t megaminx-phase1-audit .
docker run --rm megaminx-phase1-audit
```

## Certificate format

Each file in `certificates/antipodes/` or `certificates/top2/` starts with a
little-endian `uint32` record count and three `uint8` incident-edge indices.
Each sorted six-byte record then stores one corner position and orientation,
three edge positions, and three packed edge-orientation bits. The antipode
files contain 159 depth-10 records per target. The top-two-layer files contain
364,616 depth-9-or-10 records per target.

The distributed JSON certificates contain the five independently enumerated
relaxed CSP solutions, the ordinary-diameter result, the full lower-bound
witness, and the expected independent witness audits. See
`REPRODUCIBILITY.md` for the complete binary schema and replay commands.

## Main files

- `paper/main.tex`: source of the forthcoming manuscript.
- `src/generate_phase1_tables.cpp`: complete BFS generator.
- `src/generate_diameter_orbits.py` and `src/verify_coordinate_diameter.cpp`: orbit reduction and exhaustive ordinary-diameter checker.
- `src/verify_csp.py`: exhaustive Python CSP solver.
- `src/verify_csp_independent.cpp`: independent C++ CSP solver.
- `src/verify_witness.py` and `src/verify_witness_python.py`: independent lower-bound layer verifiers.
- `src/verify_reachability_group.py`: independent Schreier--Sims reachability verifier.
- `certificates/antipodes/`: 20 depth-10 sets.
- `certificates/top2/`: 20 depth-9-or-10 sets.
- `certificates/coordinate_diameter.json`: expected orbit-reduced diameter result.
- `certificates/witness.json`: full legal distance-9 witness.
- `AUDIT.md`: verification record and limitations.
- `REPRODUCIBILITY.md`: file formats and commands.

The manuscript PDF is intentionally not versioned. Build it locally with
`make paper`; the output under `release/` is ignored by Git.
