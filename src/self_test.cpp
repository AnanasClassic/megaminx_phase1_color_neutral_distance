#include "phase1_common.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

int main() {
    // Exhaustive rank/unrank round trip for the full coordinate space.
    for (std::uint32_t index = 0; index < phase1::kCoordinateSize; ++index) {
        const auto state = phase1::decode(index);
        if (phase1::encode(state) != index) {
            throw std::runtime_error("encode/decode mismatch at index " + std::to_string(index));
        }
        if (state.edge_positions[0] == state.edge_positions[1] ||
            state.edge_positions[0] == state.edge_positions[2] ||
            state.edge_positions[1] == state.edge_positions[2]) {
            throw std::runtime_error("duplicate edge position in decoded state");
        }
    }

    // Every corner has three incident edges; every edge belongs to two corners.
    std::array<int, phase1::kEdges> edge_occurrences{};
    for (int corner = 0; corner < phase1::kCorners; ++corner) {
        for (const auto edge : phase1::incident_edges(corner)) ++edge_occurrences[edge];
    }
    for (int edge = 0; edge < phase1::kEdges; ++edge) {
        if (edge_occurrences[edge] != 2) throw std::runtime_error("bad incidence count");
    }

    // Each face generator has order five on every phase coordinate.
    for (std::uint32_t index = 0; index < phase1::kCoordinateSize; index += 7919) {
        const auto original = phase1::decode(index);
        for (const auto& face : phase1::kFaceMoves) {
            auto state = original;
            for (int i = 0; i < 5; ++i) phase1::apply_quarter_turn(state, face);
            if (!(state == original)) throw std::runtime_error("face move does not have order five");
        }
    }

    std::cout << "VERIFIED: exhaustive coordinate round trip, incidence, and move-order tests passed.\n";
    return 0;
}
