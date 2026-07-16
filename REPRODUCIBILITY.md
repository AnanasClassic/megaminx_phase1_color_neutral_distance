# Reproducibility guide

## 1. Coordinate

One target block consists of one corner piece and its three incident edge
pieces. A coordinate records:

- corner position: 20 values;
- corner orientation: 3 values;
- ordered distinct edge positions: 30 x 29 x 28 values;
- three edge orientations: 2^3 values.

Therefore the coordinate has

```text
20 * 3 * 30 * 29 * 28 * 8 = 11,692,800
```

states.

## 2. Binary certificate format

Every `phase1_antipodes_block_N.bin` or `phase1_top2_block_N.bin` has:

```text
uint32 little-endian record_count
uint8[3] incident_edge_piece_indices
record[record_count]
```

Each record is six bytes:

```text
uint8 corner_position
uint8 corner_orientation
uint8 edge_position_0
uint8 edge_position_1
uint8 edge_position_2
uint8 edge_orientation_bits
```

The records are sorted by the coordinate rank implemented in
`phase1_common.hpp`.

Expected counts:

```text
antipodes: 159 records per target
upper two layers: 364,616 records per target
```

These are distances from the solved target coordinate. In particular, 10 is
the eccentricity of that coordinate, not the ordinary graph diameter.
The certificate filenames retain `antipodes` as shorthand for the vertices
furthest from this solved root.

## 3. Fixed-target BFS output

Expected distribution:

| depth | states |
|---:|---:|
| 0 | 1 |
| 1 | 12 |
| 2 | 168 |
| 3 | 2,214 |
| 4 | 26,466 |
| 5 | 256,292 |
| 6 | 1,668,044 |
| 7 | 5,125,524 |
| 8 | 4,249,463 |
| 9 | 364,457 |
| 10 | 159 |

The total must be 11,692,800.

## 4. Ordinary coordinate-graph diameter

The full coordinate graph is a Schreier graph rather than a Cayley graph. Its
ordinary diameter is checked separately:

```bash
./scripts/verify_coordinate_diameter.sh
```

The script normalizes the orientation coordinates and the three tracked-edge
labels, enumerates 81,200 positional roots, and reduces them by 120 spatial
symmetries to 708 orbit representatives. A complete BFS from every
representative must produce:

```text
eccentricity_10=637
eccentricity_11=71
exact_diameter=11
```

The eccentricity counts are counts of orbit representatives, not weighted
vertex counts. The expected machine-readable result is stored in
`certificates/coordinate_diameter.json`.

## 5. Independent reachability audit

The lower-bound witness is checked directly for membership in the face-turn
group using Schreier--Sims on 120 oriented cubie-position states. This verifier
requires SymPy 1.14:

```bash
python3 -m pip install -r requirements-audit.txt
python3 src/verify_reachability_group.py --certificates certificates
```

It must report that the computed group order equals
`(20!/2) * 3^19 * (30!/2) * 2^29` and that the witness is a member.

## 6. Quick verification

```bash
./scripts/quick_verify.sh
```

Expected final line:

```text
QUICK VERIFICATION COMPLETE
```

The Python and C++ solvers use different search implementations and must return
identical lexicographically ordered lists of five global edge configurations.
Every configuration must have cycle type `2^3 3^8` and odd parity.

## 7. Full regeneration

```bash
./scripts/reproduce.sh
```

Expected final line:

```text
FULL REPRODUCTION COMPLETE
```

The script compiles the generator, launches four workers, compares all 40
generated binary files with the distributed certificates, reruns both CSP
solvers, verifies the witness against the regenerated tables, and rechecks the
ordinary coordinate-graph diameter.

## 8. Build the forthcoming manuscript

Requirements: pdfLaTeX with TikZ, cleveref, listings, booktabs, microtype, and
Latin Modern fonts.

```bash
./scripts/build_paper.sh
```

The output is `release/megaminx_phase1_preprint.pdf`.

## 9. Platform audited

- Date: 2026-07-13
- Linux x86_64
- GCC 13.3.0
- Python 3.12.13
- pdfTeX 1.40.25 / TeX Live 2023

The BFS and CSP code uses only the C++ and Python standard libraries. The
independent Schreier--Sims reachability audit additionally uses SymPy 1.14.0.
