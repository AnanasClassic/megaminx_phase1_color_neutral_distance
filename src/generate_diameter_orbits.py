#!/usr/bin/env python3
"""Generate orbit representatives for the phase-coordinate diameter check.

Independent translations of the corner- and edge-orientation coordinates and
permutations of the three tracked edge labels reduce every coordinate to a
state with zero orientations and sorted edge positions.  The remaining
20*C(30,3) positional roots are reduced by the 120 automorphisms of the
dodecahedral face-incidence structure.  Reflections are valid here because the
face-turn metric contains every non-identity power of every face generator.
"""

from __future__ import annotations

import argparse
import json
from itertools import combinations
from pathlib import Path


CORNER_FACES = [
    (0, 1, 2), (0, 2, 3), (0, 3, 4), (0, 4, 5), (0, 5, 1),
    (6, 2, 1), (7, 3, 2), (8, 4, 3), (9, 5, 4), (10, 1, 5),
    (2, 6, 7), (1, 10, 6), (5, 9, 10), (4, 8, 9), (3, 7, 8),
    (11, 7, 6), (11, 6, 10), (11, 10, 9), (11, 9, 8), (11, 8, 7),
]

EDGE_FACES = [
    (0, 2), (0, 3), (0, 4), (0, 5), (0, 1),
    (1, 2), (2, 3), (3, 4), (4, 5), (5, 1),
    (6, 2), (7, 3), (8, 4), (9, 5), (10, 1),
    (2, 7), (1, 6), (5, 10), (4, 9), (3, 8),
    (7, 6), (6, 10), (10, 9), (9, 8), (8, 7),
    (11, 6), (11, 10), (11, 9), (11, 8), (11, 7),
]


def face_automorphisms() -> list[tuple[int, ...]]:
    adjacency = [set() for _ in range(12)]
    for first, second in EDGE_FACES:
        adjacency[first].add(second)
        adjacency[second].add(first)

    result: list[tuple[int, ...]] = []
    mapping = [-1] * 12
    used = [False] * 12

    def consistent(source: int, target: int) -> bool:
        return all(
            image == -1
            or ((other in adjacency[source]) == (image in adjacency[target]))
            for other, image in enumerate(mapping)
        )

    def search() -> None:
        if all(image != -1 for image in mapping):
            result.append(tuple(mapping))
            return
        unmapped = [face for face, image in enumerate(mapping) if image == -1]
        source = max(
            unmapped,
            key=lambda face: sum(mapping[neighbor] != -1 for neighbor in adjacency[face]),
        )
        for target in range(12):
            if not used[target] and consistent(source, target):
                mapping[source] = target
                used[target] = True
                search()
                used[target] = False
                mapping[source] = -1

    search()
    if len(result) != 120:
        raise RuntimeError(f"expected 120 face automorphisms, found {len(result)}")
    return result


def position_actions() -> list[tuple[tuple[int, ...], tuple[int, ...]]]:
    corner_index = {frozenset(faces): index for index, faces in enumerate(CORNER_FACES)}
    edge_index = {frozenset(faces): index for index, faces in enumerate(EDGE_FACES)}
    result = []
    for action in face_automorphisms():
        corners = tuple(
            corner_index[frozenset(action[face] for face in faces)]
            for faces in CORNER_FACES
        )
        edges = tuple(
            edge_index[frozenset(action[face] for face in faces)]
            for faces in EDGE_FACES
        )
        if sorted(corners) != list(range(20)) or sorted(edges) != list(range(30)):
            raise RuntimeError("a face automorphism did not induce position permutations")
        result.append((corners, edges))
    return result


def rank_root(corner: int, edges: tuple[int, int, int]) -> int:
    first, second, third = edges
    return ((corner * 30 + first) * 30 + second) * 30 + third


def representatives() -> list[tuple[int, int, int, int]]:
    actions = position_actions()
    result = []
    for corner in range(20):
        for edges in combinations(range(30), 3):
            own = rank_root(corner, edges)
            canonical = min(
                rank_root(corners[corner], tuple(sorted(edge_action[edge] for edge in edges)))
                for corners, edge_action in actions
            )
            if own == canonical:
                result.append((corner, *edges))
    if len(result) != 708:
        raise RuntimeError(f"expected 708 position orbits, found {len(result)}")
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    rows = representatives()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        "".join(" ".join(map(str, row)) + "\n" for row in rows),
        encoding="ascii",
    )
    print(json.dumps({
        "spatial_automorphisms": 120,
        "positional_roots": 20 * 4060,
        "orbit_representatives": len(rows),
    }, sort_keys=True))


if __name__ == "__main__":
    main()
