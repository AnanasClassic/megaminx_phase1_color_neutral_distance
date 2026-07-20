# Exact worst-case color-neutral distance for Megaminx phase 1

This repository is the reproducibility package for the manuscript

> **The Exact Worst-Case Color-Neutral Distance for the First Block Phase of the Megaminx**

## Main result

For one fixed target corner, the first phase solves that corner and its three incident edges. The corresponding FTM coordinate graph has 11,692,800 states. The solved coordinate has eccentricity 10, with 159 states at distance 10; the ordinary Schreier-graph diameter is 11.

When the target corner may be selected among all 20 corners, the exact worst-case distance is

```text
D_CN = 9.
```

The upper bound is established by exhaustive compatibility enumeration of the 20 depth-10 sets. After all corner constraints are discarded, exactly five global edge assignments remain, and all five have odd edge-permutation parity. The lower bound is certified by a reachable full cubie state whose 20 target distances are all 9.

Conditional on the externally reported 114-move FTM upper bound, replacing the fixed first-phase contribution 10 by the exact color-neutral value 9 gives the corresponding conditional value 113. This repository independently certifies the phase-1 result; it does not reproduce the much larger computation underlying 114.

## Author

- Vladislav Kuznetsov
- Moscow Institute of Physics and Technology (MIPT), Dolgoprudny, Russia
- vladkuznecov266@gmail.com

## Proof objects

| Claim | Evidence | Independent check |
|---|---|---|
| Fixed-target distribution | 20 complete BFS tables | Coordinate/move self-tests and byte-identical regeneration |
| Ordinary diameter 11 | BFS from 708 symmetry-orbit representatives | Machine-readable orbit report |
| Upper bound `D_CN <= 9` | Serialized depth-10 domains and five complete relaxed-CSP solutions | Independent Python and C++ exhaustive enumerators; parity check |
| Lower bound `D_CN >= 9` | Full cubie witness and layer-membership certificates | Two layer verifiers and direct Schreier-Sims group membership |

No trained model or heuristic search belongs to the trusted proof.

## Fast verification

Requirements: Python 3.10+, a C++20 compiler, and SymPy 1.14 for the direct group-membership audit.

```bash
python3 -m pip install -r requirements-audit.txt
make test
# Equivalent: make quick-verify
```

The command checks the coordinate implementation, independently enumerates the five relaxed configurations in Python and C++, checks their parity, verifies twenty witness distances equal to 9 with two layer-membership implementations, and directly checks witness membership in the generated face-turn group by Schreier-Sims.

## Full reproduction

```bash
make reproduce
```

This rebuilds all 20 BFS tables (233,856,000 visited quotient coordinates in total), requires all 40 generated certificate files to be byte-identical to the distributed versions, reruns the upper- and lower-bound checks, and performs the orbit-reduced exhaustive computation of ordinary coordinate-graph diameter 11.

The diameter check can also be run separately:

```bash
make diameter
```

Use `make integrity` to check the repository-wide SHA-256 manifest.

## Manuscript

- Source: `paper/main.tex`
- Author metadata: `paper/author.tex`
- Build: `make paper`
- Output: `release/megaminx_phase1_preprint.pdf`

The build fails on undefined references and overfull boxes. The publication-polished manuscript is nine pages in the repository build environment.

## Artifact version

The proof code and certificates used by the publication-polished manuscript are pinned at commit:

```text
f67b6d1530166eba37cd8ee0a3161d4b4e3f14ac
```

That immutable revision contains the computational proof objects. Later commits in PR #1 polish the manuscript, metadata, and documentation without changing numerical results, source algorithms, or certificates. `checksums.sha256` binds all distributed files in the current tree.

No DOI has been assigned at the time of writing. Citation metadata is provided in `CITATION.cff`.

## Repository map

- `paper/main.tex`: manuscript source.
- `src/generate_phase1_tables.cpp`: complete BFS generator.
- `src/generate_diameter_orbits.py`, `src/verify_coordinate_diameter.cpp`: ordinary-diameter reduction and checker.
- `src/verify_csp.py`, `src/verify_csp_independent.cpp`: independent exhaustive upper-bound enumerators.
- `src/verify_witness.py`, `src/verify_witness_python.py`: independent lower-bound layer verifiers.
- `src/verify_reachability_group.py`: direct Schreier-Sims witness-membership verifier.
- `certificates/antipodes/`: 20 depth-10 sets.
- `certificates/top2/`: 20 depth-9-or-10 sets.
- `certificates/witness.json`: lower-bound witness.
- `AUDIT.md`: completed checks and logical dependencies.
- `REPRODUCIBILITY.md`: formats, commands, and audited environment.
