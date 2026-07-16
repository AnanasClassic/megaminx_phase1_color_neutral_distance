#include "phase1_common.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using phase1::State;

struct BfsResult {
    std::vector<std::uint64_t> layer_counts;
    std::vector<State> antipodes;
    std::vector<State> top_two_layers;
    std::uint64_t visited{};
    int solved_eccentricity{};
    double seconds{};
};

BfsResult bfs_for_block(int block, bool collect_top_two) {
    const auto start_time = std::chrono::steady_clock::now();
    std::vector<std::uint8_t> distance(phase1::kCoordinateSize, 0xff);
    const auto solved = phase1::solved_block_state(block);
    const auto root = phase1::encode(solved);
    distance[root] = 0;

    std::vector<std::uint32_t> current{root};
    std::vector<std::uint32_t> next;
    std::vector<std::uint64_t> counts{1};
    std::uint64_t visited = 1;
    int depth = 0;

    while (!current.empty()) {
        next.clear();
        // This cap avoids pathological over-allocation while retaining good performance.
        const std::size_t reserve_hint = std::min<std::size_t>(
            static_cast<std::size_t>(phase1::kCoordinateSize - visited),
            current.size() * 3u + 1024u);
        next.reserve(reserve_hint);

        for (const auto index : current) {
            const State original = phase1::decode(index);
            for (const auto& face : phase1::kFaceMoves) {
                State state = original;
                for (int power = 1; power <= 4; ++power) {
                    phase1::apply_quarter_turn(state, face);
                    const auto neighbor = phase1::encode(state);
                    if (distance[neighbor] == 0xff) {
                        distance[neighbor] = static_cast<std::uint8_t>(depth + 1);
                        next.push_back(neighbor);
                    }
                }
            }
        }
        if (next.empty()) break;
        ++depth;
        visited += next.size();
        counts.push_back(next.size());
        current.swap(next);
    }

    BfsResult result;
    result.layer_counts = counts;
    result.visited = visited;
    result.solved_eccentricity = depth;
    for (std::uint32_t index = 0; index < phase1::kCoordinateSize; ++index) {
        const auto d = distance[index];
        if (d == depth) result.antipodes.push_back(phase1::decode(index));
        if (collect_top_two && d >= depth - 1) result.top_two_layers.push_back(phase1::decode(index));
    }
    const auto end_time = std::chrono::steady_clock::now();
    result.seconds = std::chrono::duration<double>(end_time - start_time).count();
    return result;
}

void write_state_file(const fs::path& path, const std::array<std::uint8_t, 3>& incident,
                      const std::vector<State>& states) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot open output file: " + path.string());
    const auto size = static_cast<std::uint32_t>(states.size());
    const std::array<char, 4> little_endian_size{
        static_cast<char>(size & 0xffu),
        static_cast<char>((size >> 8u) & 0xffu),
        static_cast<char>((size >> 16u) & 0xffu),
        static_cast<char>((size >> 24u) & 0xffu),
    };
    out.write(little_endian_size.data(), little_endian_size.size());
    out.write(reinterpret_cast<const char*>(incident.data()), incident.size());
    out.write(reinterpret_cast<const char*>(states.data()),
              static_cast<std::streamsize>(states.size() * sizeof(State)));
    if (!out) throw std::runtime_error("failed while writing: " + path.string());
}

int main(int argc, char** argv) {
    fs::path output = "generated";
    int first = 0;
    int last = phase1::kCorners;
    bool top_two = true;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) output = argv[++i];
        else if (arg == "--first" && i + 1 < argc) first = std::stoi(argv[++i]);
        else if (arg == "--last" && i + 1 < argc) last = std::stoi(argv[++i]);
        else if (arg == "--antipodes-only") top_two = false;
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--out DIR] [--first N] [--last N] [--antipodes-only]\n";
            return 2;
        }
    }
    if (first < 0 || first >= last || last > phase1::kCorners) {
        std::cerr << "Invalid block range\n";
        return 2;
    }

    fs::create_directories(output / "antipodes");
    if (top_two) fs::create_directories(output / "top2");
    std::ofstream log(output / ("generation_" + std::to_string(first) + "_" +
                                std::to_string(last) + ".log"));
    if (!log) throw std::runtime_error("cannot open generation log");

    for (int block = first; block < last; ++block) {
        const auto result = bfs_for_block(block, top_two);
        const auto incident = phase1::incident_edges(block);
        if (result.visited != phase1::kCoordinateSize) {
            throw std::runtime_error("BFS did not visit the full coordinate space");
        }
        write_state_file(output / "antipodes" /
                             ("phase1_antipodes_block_" + std::to_string(block) + ".bin"),
                         incident, result.antipodes);
        if (top_two) {
            write_state_file(output / "top2" /
                                 ("phase1_top2_block_" + std::to_string(block) + ".bin"),
                             incident, result.top_two_layers);
        }

        auto emit = [&](std::ostream& os) {
            os << "block=" << block
               << " solved_eccentricity=" << result.solved_eccentricity
               << " visited=" << result.visited
               << " antipodes=" << result.antipodes.size();
            if (top_two) os << " top2=" << result.top_two_layers.size();
            os << " seconds=" << std::fixed << std::setprecision(3) << result.seconds << "\n";
            os << "layers";
            for (std::size_t d = 0; d < result.layer_counts.size(); ++d) {
                os << " " << d << ":" << result.layer_counts[d];
            }
            os << "\n";
        };
        emit(std::cout);
        emit(log);
        std::cout.flush();
        log.flush();

        if (block == 0) {
            std::ofstream csv(output / "depth_distribution.csv");
            csv << "depth,states\n";
            for (std::size_t d = 0; d < result.layer_counts.size(); ++d) {
                csv << d << ',' << result.layer_counts[d] << '\n';
            }
        }
    }
    return 0;
}
