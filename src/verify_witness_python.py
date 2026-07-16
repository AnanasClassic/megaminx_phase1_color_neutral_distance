#!/usr/bin/env python3
"""Independent Python verifier for the distance-9 witness.

This implementation intentionally loads the sorted certificate records into
Python sets rather than sharing the binary-search/ranking code used by
verify_witness.py.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

RECORD = struct.Struct("6B")


def parity(p: list[int]) -> int:
    return sum(p[i] > p[j] for i in range(len(p)) for j in range(i + 1, len(p))) & 1


def records(path: Path) -> tuple[set[tuple[int, ...]], tuple[int, int, int]]:
    data = path.read_bytes()
    if len(data) < 7:
        raise ValueError(f"truncated certificate: {path}")
    count = struct.unpack_from("<I", data, 0)[0]
    incident = tuple(data[4:7])
    expected = 7 + 6 * count
    if len(data) != expected:
        raise ValueError(f"wrong length for {path}: got {len(data)}, expected {expected}")
    return {RECORD.unpack_from(data, 7 + 6 * i) for i in range(count)}, incident


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--certificates", type=Path, default=Path("certificates"))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    root = args.certificates
    w = json.loads((root / "witness.json").read_text())
    cp, co = w["corner_piece_to_position"], w["corner_orientation"]
    ep, eo = w["edge_piece_to_position"], w["edge_orientation"]

    checks = {
        "corner_positions_are_permutation": sorted(cp) == list(range(20)),
        "edge_positions_are_permutation": sorted(ep) == list(range(30)),
        "corner_permutation_even": parity(cp) == 0,
        "edge_permutation_even": parity(ep) == 0,
        "corner_orientation_sum_mod_3": sum(co) % 3 == 0,
        "edge_orientation_sum_mod_2": sum(eo) % 2 == 0,
    }
    if not all(checks.values()):
        raise AssertionError(checks)

    distances: list[int] = []
    local_coordinates: list[dict[str, object]] = []
    for block in range(20):
        top, top_incident = records(root / "top2" / f"phase1_top2_block_{block}.bin")
        anti, anti_incident = records(root / "antipodes" / f"phase1_antipodes_block_{block}.bin")
        if len(top) != 364_616 or len(anti) != 159:
            raise AssertionError(f"unexpected layer counts for block {block}")
        if top_incident != anti_incident:
            raise AssertionError(f"incidence mismatch for block {block}")
        incident = top_incident
        orient = (eo[incident[0]] << 2) | (eo[incident[1]] << 1) | eo[incident[2]]
        target = (cp[block], co[block], ep[incident[0]], ep[incident[1]], ep[incident[2]], orient)
        if target not in top:
            raise AssertionError((block, "not in depth 9 or 10"))
        if target in anti:
            raise AssertionError((block, "depth 10"))
        distances.append(9)
        local_coordinates.append({"block": block, "coordinate": list(target), "distance": 9})

    report = {
        "implementation": "independent Python set-membership verifier",
        "reachability_invariants": checks,
        "distances": distances,
        "local_coordinates": local_coordinates,
        "conclusion": "The witness has distance 9 for all 20 first-block targets.",
    }
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("20 color-neutral block distances:", distances)
    print("INDEPENDENT WITNESS LAYER VERIFICATION PASSED")


if __name__ == "__main__":
    main()
