# Reproducibility guide

## 1. Coordinate

One target block contains one corner and its three incident edges. A coordinate records:

- corner position: 20 values;
- corner orientation: 3 values;
- ordered distinct edge positions: `30 x 29 x 28` values;
- three edge orientations: `2^3` values.

Hence the coordinate space has

```text
20 * 3 * 30 * 29 * 28 * 8 = 11,692,800
```

states.

## 2. Binary certificate format

Each `phase1_antipodes_block_N.bin` and `phase1_top2_block_N.bin` contains:

```text
uint32 little-endian record_count
uint8[3] incident_edge_piece_indices
record[record_count]
```

Each six-byte record contains:

```text
uint8 corner_position
uint8 corner_orientation
uint8 edge_position_0
uint8 edge_position_1
uint8 edge_position_2
uint8 edge_orientation_bits
```

Records are sorted by the rank implemented in `src/phase1_common.hpp`.

Expected counts per target:

```text
depth 10:       159 records
depths 9 or 10: 364,616 records
```

The value 10 is the eccentricity of the solved target coordinate, not the ordinary graph diameter.

## 3. Fixed-target distribution

| Depth | States |
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

The total is 11,692,800.

## 4. Ordinary coordinate-graph diameter

```bash
make diameter
```

The checker normalizes orientation coordinates and labels of the three tracked edges, enumerates 81,200 positional roots, and reduces them by 120 spatial symmetries to 708 orbit representatives. Complete BFS from every representative must report:

```text
eccentricity_10=637
eccentricity_11=71
exact_diameter=11
```

The eccentricity counts refer to orbit representatives, not weighted vertex counts.

## 5. Direct witness-reachability audit

The lower-bound witness is checked directly for membership in the face-turn group using Schreier-Sims on 120 oriented cubie-position states:

```bash
python3 -m pip install -r requirements-audit.txt
python3 src/verify_reachability_group.py --certificates certificates
```

The verifier must compute group order

```text
(20!/2) * 3^19 * (30!/2) * 2^29
```

and confirm membership of the witness. The parity and orientation invariants are checked separately as consistency controls.

## 6. Quick verification

```bash
make test
```

Expected final line:

```text
QUICK VERIFICATION COMPLETE
```

The Python and C++ upper-bound solvers must return identical lexicographically ordered lists of five global edge assignments. Every assignment has cycle type `2^3 3^8` and odd parity. Two witness verifiers must return twenty distances equal to 9, and the direct group-membership audit must pass.

## 7. Full regeneration

```bash
make reproduce
```

Expected final line:

```text
FULL REPRODUCTION COMPLETE
```

The script compiles the generators, rebuilds all 20 tables, compares all 40 generated binary files byte-for-byte with the distributed certificates, reruns both CSP solvers, verifies the witness against regenerated tables, and rechecks the ordinary diameter.

## 8. Build the manuscript

Required LaTeX packages include TikZ, cleveref, listings, booktabs, tabularx, microtype, and Latin Modern fonts.

```bash
make paper
```

Output:

```text
release/megaminx_phase1_preprint.pdf
```

The build script rejects undefined references and overfull boxes. The publication-polished source builds to nine A4 pages in the audited environment and was visually inspected page by page.

## 9. Immutable computational artifact

The proof code and certificates used by the publication-polished manuscript are pinned at:

```text
f67b6d1530166eba37cd8ee0a3161d4b4e3f14ac
```

Immutable URL:

```text
https://github.com/AnanasClassic/megaminx_phase1_color_neutral_distance/tree/f67b6d1530166eba37cd8ee0a3161d4b4e3f14ac
```

Later commits in PR #1 modify the presentation and metadata only. The complete current tree is bound by `checksums.sha256`. No DOI has been assigned.

## 10. Audited platform

- Linux x86_64
- GCC 13.3.0
- Python 3.12.13
- SymPy 1.14.0
- pdfTeX 1.40.25 / TeX Live 2023

The BFS and CSP implementations use only the C++ and Python standard libraries. SymPy is required only for the independent group-membership audit.
