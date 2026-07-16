#ifndef MEGAMINX_PHASE1_COMMON_HPP
#define MEGAMINX_PHASE1_COMMON_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace phase1 {

constexpr std::uint32_t kCoordinateSize = 11'692'800;
constexpr int kCorners = 20;
constexpr int kEdges = 30;
constexpr int kFaces = 12;

struct State {
    std::uint8_t corner_position{};
    std::uint8_t corner_orientation{};
    std::array<std::uint8_t, 3> edge_positions{};
    std::uint8_t edge_orientation_bits{};
};
static_assert(sizeof(State) == 6, "The certificate format assumes a packed 6-byte State");

struct Face {
    std::array<std::uint8_t, 5> corners;
    std::array<std::uint8_t, 5> edges;
    bool changes_orientation;
};

inline constexpr std::array<Face, kFaces> kFaceMoves{{
    {{{0,1,2,3,4}},       {{0,1,2,3,4}},       false}, // U
    {{{0,4,9,11,5}},      {{4,9,14,16,5}},     true }, // R
    {{{1,0,5,10,6}},      {{0,5,10,15,6}},     true }, // F
    {{{2,1,6,14,7}},      {{1,6,11,19,7}},     true }, // L
    {{{3,2,7,13,8}},      {{2,7,12,18,8}},     true }, // BL
    {{{4,3,8,12,9}},      {{3,8,13,17,9}},     true }, // BR
    {{{15,16,17,18,19}},  {{25,26,27,28,29}},  false}, // D
    {{{15,19,14,6,10}},   {{29,24,11,15,20}},  true }, // FL
    {{{16,15,10,5,11}},   {{25,20,10,16,21}},  true }, // FR
    {{{17,16,11,9,12}},   {{26,21,14,17,22}},  true }, // DR
    {{{18,17,12,8,13}},   {{27,22,13,18,23}},  true }, // B
    {{{19,18,13,7,14}},   {{28,23,12,19,24}},  true }, // DL
}};

// Each corner is represented by the three incident face labels. Each edge is
// represented by its two incident face labels. These labels match the public
// MegaminxStructure.go / 12Gen.go implementation cited in the paper.
inline constexpr int kCornerFaces[kCorners][3] = {
    {0,1,2},{0,2,3},{0,3,4},{0,4,5},{0,5,1},
    {6,2,1},{7,3,2},{8,4,3},{9,5,4},{10,1,5},
    {2,6,7},{1,10,6},{5,9,10},{4,8,9},{3,7,8},
    {11,7,6},{11,6,10},{11,10,9},{11,9,8},{11,8,7}
};

inline constexpr int kEdgeFaces[kEdges][2] = {
    {0,2},{0,3},{0,4},{0,5},{0,1},
    {1,2},{2,3},{3,4},{4,5},{5,1},
    {6,2},{7,3},{8,4},{9,5},{10,1},
    {2,7},{1,6},{5,10},{4,9},{3,8},
    {7,6},{6,10},{10,9},{9,8},{8,7},
    {11,6},{11,10},{11,9},{11,8},{11,7}
};

inline std::array<std::uint8_t, 3> incident_edges(int corner) {
    if (corner < 0 || corner >= kCorners) {
        throw std::out_of_range("corner index");
    }
    std::array<std::uint8_t, 3> result{};
    int count = 0;
    for (int edge = 0; edge < kEdges; ++edge) {
        bool first = false;
        bool second = false;
        for (int j = 0; j < 3; ++j) {
            first  = first  || (kCornerFaces[corner][j] == kEdgeFaces[edge][0]);
            second = second || (kCornerFaces[corner][j] == kEdgeFaces[edge][1]);
        }
        if (first && second) {
            if (count >= 3) throw std::runtime_error("more than three incident edges");
            result[count++] = static_cast<std::uint8_t>(edge);
        }
    }
    if (count != 3) throw std::runtime_error("corner does not have three incident edges");
    std::sort(result.begin(), result.end());
    return result;
}

inline std::uint8_t rank_excluding(std::uint8_t x, std::uint8_t a) {
    return static_cast<std::uint8_t>(x - (a < x));
}
inline std::uint8_t rank_excluding_two(std::uint8_t x, std::uint8_t a, std::uint8_t b) {
    return static_cast<std::uint8_t>(x - (a < x) - (b < x));
}
inline std::uint8_t unrank_excluding(std::uint8_t r, std::uint8_t a) {
    return static_cast<std::uint8_t>(r + (r >= a));
}
inline std::uint8_t unrank_excluding_two(std::uint8_t r, std::uint8_t a, std::uint8_t b) {
    const auto lo = std::min(a, b);
    const auto hi = std::max(a, b);
    std::uint8_t x = r;
    if (x >= lo) ++x;
    if (x >= hi) ++x;
    return x;
}

inline std::uint32_t encode(const State& state) {
    const std::uint32_t second = rank_excluding(state.edge_positions[1], state.edge_positions[0]);
    const std::uint32_t third  = rank_excluding_two(
        state.edge_positions[2], state.edge_positions[0], state.edge_positions[1]);
    const std::uint32_t base =
        static_cast<std::uint32_t>(state.corner_orientation)
        + 3u * static_cast<std::uint32_t>(state.corner_position)
        + 60u * static_cast<std::uint32_t>(state.edge_positions[0])
        + 1800u * second
        + 52200u * third;
    return (base << 3u) | state.edge_orientation_bits;
}

inline State decode(std::uint32_t index) {
    if (index >= kCoordinateSize) throw std::out_of_range("coordinate index");
    State state{};
    state.edge_orientation_bits = static_cast<std::uint8_t>(index & 7u);
    std::uint32_t base = index >> 3u;
    state.corner_orientation = static_cast<std::uint8_t>(base % 3u); base /= 3u;
    state.corner_position = static_cast<std::uint8_t>(base % 20u); base /= 20u;
    state.edge_positions[0] = static_cast<std::uint8_t>(base % 30u); base /= 30u;
    const auto second = static_cast<std::uint8_t>(base % 29u); base /= 29u;
    const auto third = static_cast<std::uint8_t>(base); // 0..27
    state.edge_positions[1] = unrank_excluding(second, state.edge_positions[0]);
    state.edge_positions[2] = unrank_excluding_two(
        third, state.edge_positions[0], state.edge_positions[1]);
    return state;
}

inline void apply_quarter_turn(State& state, const Face& face) {
    for (int i = 0; i < 5; ++i) {
        if (state.corner_position == face.corners[i]) {
            if (face.changes_orientation) {
                const int delta = (i == 0) ? 1 : 2;
                state.corner_orientation = static_cast<std::uint8_t>(
                    (state.corner_orientation + delta) % 3);
            }
            state.corner_position = face.corners[(i + 1) % 5];
            break;
        }
    }
    for (int tracked = 0; tracked < 3; ++tracked) {
        for (int i = 0; i < 5; ++i) {
            if (state.edge_positions[tracked] == face.edges[i]) {
                if (face.changes_orientation && (i == 2 || i == 4)) {
                    state.edge_orientation_bits ^= static_cast<std::uint8_t>(1u << (2 - tracked));
                }
                state.edge_positions[tracked] = face.edges[(i + 1) % 5];
                break;
            }
        }
    }
}

inline State solved_block_state(int corner) {
    const auto edges = incident_edges(corner);
    return State{static_cast<std::uint8_t>(corner), 0, edges, 0};
}

inline bool operator==(const State& a, const State& b) {
    return a.corner_position == b.corner_position
        && a.corner_orientation == b.corner_orientation
        && a.edge_positions == b.edge_positions
        && a.edge_orientation_bits == b.edge_orientation_bits;
}

} // namespace phase1

#endif
