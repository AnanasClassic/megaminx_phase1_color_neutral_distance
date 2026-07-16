#!/usr/bin/env python3
"""Independent Schreier--Sims reachability audit of the lower-bound witness.

The action is on 120 oriented cubie-position states: 20*3 corner states followed
by 30*2 edge states. No generated BFS certificate is used.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

try:
    from sympy.combinatorics import Permutation, PermutationGroup
except ImportError as exc:
    raise SystemExit(
        "SymPy is required for the independent group audit. "
        "Install it with: python3 -m pip install -r requirements-audit.txt"
    ) from exc

FACES = [
    ([0,1,2,3,4],[0,1,2,3,4],False,"U"),
    ([0,4,9,11,5],[4,9,14,16,5],True,"R"),
    ([1,0,5,10,6],[0,5,10,15,6],True,"F"),
    ([2,1,6,14,7],[1,6,11,19,7],True,"L"),
    ([3,2,7,13,8],[2,7,12,18,8],True,"BL"),
    ([4,3,8,12,9],[3,8,13,17,9],True,"BR"),
    ([15,16,17,18,19],[25,26,27,28,29],False,"D"),
    ([15,19,14,6,10],[29,24,11,15,20],True,"FL"),
    ([16,15,10,5,11],[25,20,10,16,21],True,"FR"),
    ([17,16,11,9,12],[26,21,14,17,22],True,"DR"),
    ([18,17,12,8,13],[27,22,13,18,23],True,"B"),
    ([19,18,13,7,14],[28,23,12,19,24],True,"DL"),
]


def face_generator(face: tuple[list[int], list[int], bool, str]) -> Permutation:
    corners, edges, changes_orientation, _ = face
    images = list(range(120))
    for position in range(20):
        for orientation in range(3):
            p, o = position, orientation
            if position in corners:
                i = corners.index(position)
                p = corners[(i + 1) % 5]
                if changes_orientation:
                    o = (orientation + (1 if i == 0 else 2)) % 3
            images[position * 3 + orientation] = p * 3 + o
    for position in range(30):
        for orientation in range(2):
            p, o = position, orientation
            if position in edges:
                i = edges.index(position)
                p = edges[(i + 1) % 5]
                if changes_orientation and i in (2, 4):
                    o = orientation ^ 1
            images[60 + position * 2 + orientation] = 60 + p * 2 + o
    return Permutation(images)


def witness_permutation(w: dict[str, list[int]]) -> Permutation:
    cp, co = w["corner_piece_to_position"], w["corner_orientation"]
    ep, eo = w["edge_piece_to_position"], w["edge_orientation"]
    images = list(range(120))
    for piece in range(20):
        for orientation in range(3):
            images[piece * 3 + orientation] = cp[piece] * 3 + ((orientation + co[piece]) % 3)
    for piece in range(30):
        for orientation in range(2):
            images[60 + piece * 2 + orientation] = 60 + ep[piece] * 2 + (orientation ^ eo[piece])
    return Permutation(images)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--certificates", type=Path, default=Path("certificates"))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    witness = json.loads((args.certificates / "witness.json").read_text())
    generators = [face_generator(face) for face in FACES]
    generator_orders = [int(g.order()) for g in generators]
    if generator_orders != [5] * 12:
        raise AssertionError(generator_orders)

    group = PermutationGroup(generators)
    expected = (math.factorial(20) // 2) * 3**19 * (math.factorial(30) // 2) * 2**29
    computed = int(group.order())
    member = bool(group.contains(witness_permutation(witness)))
    report = {
        "implementation": "SymPy Schreier-Sims on 120 oriented cubie-position states",
        "generator_orders": generator_orders,
        "computed_group_order": computed,
        "expected_invariant_space_order": expected,
        "orders_match": computed == expected,
        "witness_is_member": member,
    }
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    for key, value in report.items():
        print(f"{key}={value}")
    if computed != expected or not member:
        raise SystemExit(1)
    print("INDEPENDENT GROUP REACHABILITY AUDIT PASSED")


if __name__ == "__main__":
    main()
