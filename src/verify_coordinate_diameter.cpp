#include "phase1_common.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

struct Root {
    int corner{};
    std::array<int, 3> edges{};
};

int eccentricity(const Root& root) {
    const phase1::State root_state{
        static_cast<std::uint8_t>(root.corner),
        0,
        {static_cast<std::uint8_t>(root.edges[0]),
         static_cast<std::uint8_t>(root.edges[1]),
         static_cast<std::uint8_t>(root.edges[2])},
        0,
    };
    const auto root_index = phase1::encode(root_state);
    std::vector<std::uint8_t> distance(phase1::kCoordinateSize, 0xff);
    distance[root_index] = 0;
    std::vector<std::uint32_t> current{root_index};
    std::vector<std::uint32_t> next;
    std::uint64_t visited = 1;
    int depth = 0;

    while (!current.empty()) {
        next.clear();
        const std::size_t reserve_hint = std::min<std::size_t>(
            static_cast<std::size_t>(phase1::kCoordinateSize - visited),
            current.size() * 3u + 1024u);
        next.reserve(reserve_hint);
        for (const auto index : current) {
            const auto original = phase1::decode(index);
            for (const auto& face : phase1::kFaceMoves) {
                auto state = original;
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
        current.swap(next);
    }
    if (visited != phase1::kCoordinateSize) {
        throw std::runtime_error("BFS did not cover the coordinate graph");
    }
    return depth;
}

std::vector<Root> read_roots(const fs::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open representatives file: " + path.string());
    std::vector<Root> roots;
    Root root;
    while (input >> root.corner >> root.edges[0] >> root.edges[1] >> root.edges[2]) {
        if (root.corner < 0 || root.corner >= phase1::kCorners
            || root.edges[0] < 0 || root.edges[2] >= phase1::kEdges
            || !(root.edges[0] < root.edges[1] && root.edges[1] < root.edges[2])) {
            throw std::runtime_error("invalid positional root");
        }
        roots.push_back(root);
    }
    if (!input.eof()) throw std::runtime_error("malformed representatives file");
    if (roots.size() != 708) {
        throw std::runtime_error("expected 708 orbit representatives");
    }
    return roots;
}

void write_result(const fs::path& path,
                  const std::array<std::uint64_t, 32>& histogram,
                  int diameter) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot open result file: " + path.string());
    output << "{\n"
           << "  \"coordinate_vertices\": 11692800,\n"
           << "  \"spatial_automorphisms\": 120,\n"
           << "  \"positional_roots\": 81200,\n"
           << "  \"orbit_representatives\": 708,\n"
           << "  \"eccentricity_histogram\": {\"10\": " << histogram[10]
           << ", \"11\": " << histogram[11] << "},\n"
           << "  \"diameter\": " << diameter << "\n"
           << "}\n";
}

int main(int argc, char** argv) try {
    fs::path representatives_path;
    fs::path output_path;
    int thread_count = 4;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--representatives" && i + 1 < argc) {
            representatives_path = argv[++i];
        } else if (argument == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (argument == "--threads" && i + 1 < argc) {
            thread_count = std::stoi(argv[++i]);
        } else {
            std::cerr << "Usage: " << argv[0]
                      << " --representatives FILE --output FILE [--threads N]\n";
            return 2;
        }
    }
    if (representatives_path.empty() || output_path.empty() || thread_count < 1) {
        throw std::runtime_error("representatives, output, and a positive thread count are required");
    }

    const auto roots = read_roots(representatives_path);
    std::atomic<std::size_t> next_root{0};
    std::atomic<int> maximum{0};
    std::array<std::atomic<std::uint64_t>, 32> atomic_histogram{};
    std::atomic<bool> failed{false};
    std::exception_ptr failure;
    std::mutex failure_mutex;
    std::mutex output_mutex;

    auto worker = [&] {
        try {
            while (!failed.load()) {
                const auto index = next_root.fetch_add(1);
                if (index >= roots.size()) return;
                const int value = eccentricity(roots[index]);
                if (value < 0 || value >= static_cast<int>(atomic_histogram.size())) {
                    throw std::runtime_error("eccentricity outside histogram range");
                }
                atomic_histogram[value].fetch_add(1);
                int old = maximum.load();
                while (value > old && !maximum.compare_exchange_weak(old, value)) {}
                if ((index + 1) % 25 == 0) {
                    std::lock_guard lock(output_mutex);
                    std::cout << "processed_index=" << index
                              << " eccentricity=" << value
                              << " current_max=" << maximum.load() << "\n";
                    std::cout.flush();
                }
            }
        } catch (...) {
            failed.store(true);
            std::lock_guard lock(failure_mutex);
            if (!failure) failure = std::current_exception();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) threads.emplace_back(worker);
    for (auto& thread : threads) thread.join();
    if (failure) std::rethrow_exception(failure);

    std::array<std::uint64_t, 32> histogram{};
    for (std::size_t i = 0; i < histogram.size(); ++i) {
        histogram[i] = atomic_histogram[i].load();
    }
    if (histogram[10] != 637 || histogram[11] != 71 || maximum.load() != 11) {
        throw std::runtime_error("diameter result does not match the certified expectation");
    }
    for (std::size_t i = 0; i < histogram.size(); ++i) {
        if (i != 10 && i != 11 && histogram[i] != 0) {
            throw std::runtime_error("unexpected eccentricity in representative histogram");
        }
    }

    write_result(output_path, histogram, maximum.load());
    std::cout << "representatives=" << roots.size() << "\n"
              << "eccentricity_10=" << histogram[10] << "\n"
              << "eccentricity_11=" << histogram[11] << "\n"
              << "exact_diameter=" << maximum.load() << "\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "ERROR: " << error.what() << "\n";
    return 1;
}
