#!/usr/bin/env python3
"""Verify the distance-9 lower-bound witness against certified BFS layers.

The binary layer files are ordered by the rank used by the BFS. Membership is
checked by direct binary search, so verification uses constant auxiliary memory.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

RECORD = struct.Struct("6B")


def permutation_parity(p: list[int]) -> int:
    return sum(p[i] > p[j] for i in range(len(p)) for j in range(i + 1, len(p))) & 1


def encode_record(record: tuple[int, int, int, int, int, int]) -> int:
    cp, co, p0, p1, p2, eo = record
    r2 = p1 - int(p0 < p1)
    r3 = p2 - int(p0 < p2) - int(p1 < p2)
    base = co + 3 * cp + 60 * p0 + 1800 * r2 + 52200 * r3
    return (base << 3) | eo


def read_file(path: Path) -> tuple[bytes, int, tuple[int, int, int]]:
    data = path.read_bytes()
    if len(data) < 7:
        raise ValueError(f"truncated certificate: {path}")
    count = struct.unpack_from("<I", data, 0)[0]
    incident = tuple(data[4:7])
    expected = 7 + RECORD.size * count
    if len(data) != expected:
        raise ValueError(f"wrong length for {path}: got {len(data)}, expected {expected}")
    return data, count, incident


def contains_sorted(data: bytes, count: int, target: tuple[int, ...]) -> bool:
    target_rank = encode_record(target)  # type: ignore[arg-type]
    lo, hi = 0, count
    while lo < hi:
        mid = (lo + hi) // 2
        record = RECORD.unpack_from(data, 7 + RECORD.size * mid)
        rank = encode_record(record)
        if rank < target_rank:
            lo = mid + 1
        else:
            hi = mid
    if lo == count:
        return False
    return RECORD.unpack_from(data, 7 + RECORD.size * lo) == target


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--certificates", type=Path, default=Path("certificates"))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    root = args.certificates
    witness = json.loads((root / "witness.json").read_text())
    cp = witness["corner_piece_to_position"]
    co = witness["corner_orientation"]
    ep = witness["edge_piece_to_position"]
    eo = witness["edge_orientation"]

    checks = {
        "corner_positions_are_permutation": sorted(cp) == list(range(20)),
        "edge_positions_are_permutation": sorted(ep) == list(range(30)),
        "corner_permutation_even": permutation_parity(cp) == 0,
        "edge_permutation_even": permutation_parity(ep) == 0,
        "corner_orientation_sum_mod_3": sum(co) % 3 == 0,
        "edge_orientation_sum_mod_2": sum(eo) % 2 == 0,
    }
    if not all(checks.values()):
        raise AssertionError(checks)

    distances: list[int] = []
    local_coordinates: list[dict[str, object]] = []
    for block in range(20):
        top_data, top_count, top_incident = read_file(
            root / "top2" / f"phase1_top2_block_{block}.bin"
        )
        anti_data, anti_count, anti_incident = read_file(
            root / "antipodes" / f"phase1_antipodes_block_{block}.bin"
        )
        if top_count != 364_616 or anti_count != 159:
            raise AssertionError(f"unexpected layer counts for block {block}")
        if top_incident != anti_incident:
            raise AssertionError(f"incidence mismatch for block {block}")
        incident = top_incident
        orient_bits = (eo[incident[0]] << 2) | (eo[incident[1]] << 1) | eo[incident[2]]
        coordinate = (
            cp[block], co[block],
            ep[incident[0]], ep[incident[1]], ep[incident[2]], orient_bits,
        )
        if not contains_sorted(top_data, top_count, coordinate):
            raise AssertionError(f"block {block} is not in depth 9 or 10")
        if contains_sorted(anti_data, anti_count, coordinate):
            raise AssertionError(f"block {block} has depth 10")
        distances.append(9)
        local_coordinates.append({
            "block": block,
            "incident_edges": list(incident),
            "coordinate": list(coordinate),
            "distance": 9,
        })

    report = {
        "reachability_invariants": checks,
        "distances": distances,
        "minimum_over_targets": min(distances),
        "maximum_over_targets": max(distances),
        "local_coordinates": local_coordinates,
        "conclusion": "The witness has distance 9 for all 20 first-block targets.",
    }
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("corner permutation parity: even")
    print("edge permutation parity: even")
    print("corner orientation sum mod 3: 0")
    print("edge orientation sum mod 2: 0")
    print("20 target distances:", " ".join(map(str, distances)))
    print("VERIFIED: every target has distance exactly 9.")


if __name__ == "__main__":
    main()
