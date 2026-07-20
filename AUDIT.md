# Computational audit

## Scope

This audit covers the independently certified statement

```text
The worst-case color-neutral distance of the first corner-block phase is exactly 9.
```

It does not independently reproduce the externally reported 114-move global bound.

## Checks completed

1. **Coordinate rank/unrank**
   - `encode(decode(i)) == i` for all 11,692,800 indices.
   - All decoded tracked-edge positions are distinct.

2. **Move implementation**
   - All 12 face generators have order five on audited coordinates.
   - Every target corner has exactly three incident target edges.
   - Every target edge belongs to exactly two target blocks.

3. **Complete fixed-target BFS**
   - All 20 target tables were regenerated from source.
   - Every BFS visited exactly 11,692,800 coordinates.
   - Every solved coordinate had eccentricity 10, with 159 states at depth 10 and 364,616 states in depths 9--10.
   - All 40 regenerated binary files were byte-identical to the distributed certificates.

4. **Ordinary coordinate-graph diameter**
   - Orientation translations and tracked-edge relabelling reduce every coordinate to one of 81,200 positional roots.
   - The 120 spatial symmetries reduce these roots to 708 orbit representatives.
   - Complete BFS from all representatives gives 637 representative orbits of eccentricity 10 and 71 of eccentricity 11.
   - Therefore the ordinary Schreier-graph diameter is 11. Orbit counts are not vertex counts.

5. **Upper-bound CSP**
   - Python solver: 145 recursive search nodes, exactly five solutions.
   - Independent C++ solver: 1,015 recursive search nodes, exactly five solutions.
   - The two implementations return the same five edge permutations.
   - Every permutation has cycle type `2^3 3^8` and odd parity.

6. **Lower-bound witness**
   - Two independently written layer verifiers obtain a distance vector of twenty copies of 9.
   - The corner and edge permutations are even.
   - Corner and edge orientation sums are 0 modulo 3 and 2.
   - An independent SymPy Schreier-Sims computation obtains the full invariant-compatible group order and directly confirms membership of the witness in the face-turn group.

7. **Compiler/runtime checks**
   - GCC builds with `-Wall -Wextra -Wpedantic`.
   - GCC undefined-behavior sanitizer completes the exhaustive coordinate self-test.

8. **Document checks**
   - pdfLaTeX compilation completes successfully.
   - Undefined references and overfull boxes are treated as build failures.
   - The publication-polished PDF was rendered page by page and visually inspected for clipping, overlap, missing glyphs, and malformed URLs.

## Logical dependency of the lower witness

Direct Schreier-Sims group membership is the primary computational certificate of reachability. The standard parity and orientation invariants are an independent consistency check. Thus the lower-bound proof does not depend solely on a brief literature summary of the Megaminx reachability characterization.

## Logical dependency of the conditional 113 corollary

The artifact proves the new first-phase contribution `10 -> 9`. The numerical corollary `114 -> 113` assumes the validity and compatibility of the externally reported 114-move computation by Botz, Whitmore, and Rokicki. The manuscript labels this dependency explicitly.

## Artifact identity

The unchanged proof code and certificate set used by the publication-polished manuscript are pinned at commit:

```text
f67b6d1530166eba37cd8ee0a3161d4b4e3f14ac
```

The current `checksums.sha256` file binds the publication tree. No DOI is claimed.

## Publication notes

- The manuscript source is versioned; generated PDFs remain ignored and should be attached to an arXiv submission or immutable release.
- `CITATION.cff` provides citation metadata for the computational artifact.
- The author remains responsible for final venue-specific formatting, attribution, and disclosure requirements.
