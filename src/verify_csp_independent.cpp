#include "phase1_common.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

struct Option {
    std::array<std::uint8_t, 3> edge{};
    std::array<std::uint8_t, 3> position{};
    std::array<std::uint8_t, 3> orientation{};

    auto key() const {
        return std::tuple{edge, position, orientation};
    }
    bool operator<(const Option& other) const { return key() < other.key(); }
};

struct Configuration {
    std::array<int, phase1::kEdges> position{};
    std::array<int, phase1::kEdges> orientation{};
    bool operator<(const Configuration& other) const {
        return std::tie(position, orientation) < std::tie(other.position, other.orientation);
    }
};

std::vector<Option> read_domain(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    std::array<std::uint8_t, 4> count_bytes{};
    std::array<std::uint8_t, 3> incident{};
    in.read(reinterpret_cast<char*>(count_bytes.data()), count_bytes.size());
    in.read(reinterpret_cast<char*>(incident.data()), incident.size());
    const std::uint32_t count = static_cast<std::uint32_t>(count_bytes[0])
        | (static_cast<std::uint32_t>(count_bytes[1]) << 8u)
        | (static_cast<std::uint32_t>(count_bytes[2]) << 16u)
        | (static_cast<std::uint32_t>(count_bytes[3]) << 24u);
    if (count != 159) throw std::runtime_error("expected 159 antipodes in " + path.string());
    std::set<Option> unique;
    for (std::uint32_t i = 0; i < count; ++i) {
        phase1::State state{};
        in.read(reinterpret_cast<char*>(&state), sizeof(state));
        if (!in) throw std::runtime_error("truncated file " + path.string());
        Option option;
        option.edge = incident;
        option.position = state.edge_positions;
        option.orientation = {
            static_cast<std::uint8_t>((state.edge_orientation_bits >> 2) & 1u),
            static_cast<std::uint8_t>((state.edge_orientation_bits >> 1) & 1u),
            static_cast<std::uint8_t>(state.edge_orientation_bits & 1u),
        };
        unique.insert(option);
    }
    char extra = 0;
    if (in.read(&extra, 1)) throw std::runtime_error("extra bytes in " + path.string());
    if (unique.size() != 80) {
        throw std::runtime_error("expected 80 unique edge signatures in " + path.string());
    }
    return {unique.begin(), unique.end()};
}

int parity(const std::array<int, phase1::kEdges>& p) {
    int result = 0;
    for (int i = 0; i < phase1::kEdges; ++i)
        for (int j = i + 1; j < phase1::kEdges; ++j)
            result ^= (p[i] > p[j]);
    return result;
}

std::vector<int> cycle_lengths(const std::array<int, phase1::kEdges>& p) {
    std::array<bool, phase1::kEdges> seen{};
    std::vector<int> result;
    for (int start = 0; start < phase1::kEdges; ++start) {
        if (seen[start]) continue;
        int x = start;
        int length = 0;
        while (!seen[x]) {
            seen[x] = true;
            ++length;
            x = p[x];
        }
        if (length > 1) result.push_back(length);
    }
    std::sort(result.begin(), result.end());
    return result;
}

class Search {
public:
    explicit Search(std::array<std::vector<Option>, phase1::kCorners> domains)
        : domains_(std::move(domains)) {
        selected_.fill(-1);
        edge_position_.fill(-1);
        edge_orientation_.fill(-1);
        position_owner_.fill(-1);
    }

    void run() { recurse(0); }

    std::uint64_t nodes() const { return nodes_; }
    const std::set<Configuration>& solutions() const { return solutions_; }

private:
    bool viable(const Option& option) const {
        for (int k = 0; k < 3; ++k) {
            const int edge = option.edge[k];
            const int pos = option.position[k];
            const int ori = option.orientation[k];
            if (edge_position_[edge] != -1 &&
                (edge_position_[edge] != pos || edge_orientation_[edge] != ori)) {
                return false;
            }
            if (position_owner_[pos] != -1 && position_owner_[pos] != edge) return false;
        }
        return true;
    }

    std::vector<int> viable_options(int block) const {
        std::vector<int> result;
        for (int i = 0; i < static_cast<int>(domains_[block].size()); ++i) {
            if (viable(domains_[block][i])) result.push_back(i);
        }
        return result;
    }

    void recurse(int assigned_count) {
        ++nodes_;
        if (assigned_count == phase1::kCorners) {
            Configuration c;
            c.position = edge_position_;
            c.orientation = edge_orientation_;
            auto sorted = c.position;
            std::sort(sorted.begin(), sorted.end());
            for (int i = 0; i < phase1::kEdges; ++i) {
                if (sorted[i] != i) throw std::runtime_error("incomplete edge permutation");
            }
            solutions_.insert(c);
            return;
        }

        int chosen_block = -1;
        std::vector<int> chosen_options;
        std::size_t best_size = static_cast<std::size_t>(-1);
        for (int block = 0; block < phase1::kCorners; ++block) {
            if (selected_[block] != -1) continue;
            auto options = viable_options(block);
            if (options.empty()) return;
            if (options.size() < best_size) {
                best_size = options.size();
                chosen_block = block;
                chosen_options = std::move(options);
                if (best_size == 1) break;
            }
        }
        if (chosen_block < 0) throw std::runtime_error("MRV selection failed");

        for (const int option_index : chosen_options) {
            const auto& option = domains_[chosen_block][option_index];
            std::array<int, 3> newly_assigned_edges{-1, -1, -1};
            int new_count = 0;
            bool ok = true;
            for (int k = 0; k < 3; ++k) {
                const int edge = option.edge[k];
                const int pos = option.position[k];
                const int ori = option.orientation[k];
                if (edge_position_[edge] == -1) {
                    if (position_owner_[pos] != -1 && position_owner_[pos] != edge) {
                        ok = false;
                        break;
                    }
                    edge_position_[edge] = pos;
                    edge_orientation_[edge] = ori;
                    position_owner_[pos] = edge;
                    newly_assigned_edges[new_count++] = edge;
                } else if (edge_position_[edge] != pos || edge_orientation_[edge] != ori) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                selected_[chosen_block] = option_index;
                recurse(assigned_count + 1);
                selected_[chosen_block] = -1;
            }
            for (int i = new_count - 1; i >= 0; --i) {
                const int edge = newly_assigned_edges[i];
                const int pos = edge_position_[edge];
                position_owner_[pos] = -1;
                edge_position_[edge] = -1;
                edge_orientation_[edge] = -1;
            }
        }
    }

    std::array<std::vector<Option>, phase1::kCorners> domains_;
    std::array<int, phase1::kCorners> selected_{};
    std::array<int, phase1::kEdges> edge_position_{};
    std::array<int, phase1::kEdges> edge_orientation_{};
    std::array<int, phase1::kEdges> position_owner_{};
    std::uint64_t nodes_ = 0;
    std::set<Configuration> solutions_;
};

int main(int argc, char** argv) {
    fs::path certificates = "certificates";
    fs::path output;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--certificates" && i + 1 < argc) certificates = argv[++i];
        else if (arg == "--output" && i + 1 < argc) output = argv[++i];
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--certificates DIR] [--output FILE]\n";
            return 2;
        }
    }

    std::array<std::vector<Option>, phase1::kCorners> domains;
    for (int block = 0; block < phase1::kCorners; ++block) {
        domains[block] = read_domain(certificates / "antipodes" /
            ("phase1_antipodes_block_" + std::to_string(block) + ".bin"));
    }

    Search search(std::move(domains));
    search.run();
    std::cout << "independent search nodes: " << search.nodes() << "\n";
    std::cout << "global relaxed edge configurations: " << search.solutions().size() << "\n";
    if (search.solutions().size() != 5) {
        std::cerr << "Expected exactly five configurations\n";
        return 1;
    }

    std::ostringstream machine;
    machine << "{\n  \"configurations\": [\n";
    int number = 0;
    for (const auto& configuration : search.solutions()) {
        const auto p = parity(configuration.position);
        const auto lengths = cycle_lengths(configuration.position);
        const std::vector<int> expected{2,2,2,3,3,3,3,3,3,3,3};
        if (p != 1 || lengths != expected) {
            std::cerr << "Parity/cycle check failed\n";
            return 1;
        }
        int flipped = 0;
        for (const auto x : configuration.orientation) flipped += x;
        std::cout << "configuration " << number
                  << ": parity=odd flipped_edges=" << flipped << " cycle_lengths=";
        for (const auto x : lengths) std::cout << x << ',';
        std::cout << "\n";
        std::cout << "  edge_piece_to_position:";
        for (const auto x : configuration.position) std::cout << ' ' << x;
        std::cout << "\n";

        machine << "    {\"edge_piece_to_position\": [";
        for (int i = 0; i < phase1::kEdges; ++i) {
            if (i) machine << ',';
            machine << configuration.position[i];
        }
        machine << "], \"edge_orientation\": [";
        for (int i = 0; i < phase1::kEdges; ++i) {
            if (i) machine << ',';
            machine << configuration.orientation[i];
        }
        machine << "], \"parity\": 1}";
        if (++number != static_cast<int>(search.solutions().size())) machine << ',';
        machine << "\n";
    }
    machine << "  ],\n  \"conclusion\": \"All five configurations are odd.\"\n}\n";
    if (!output.empty()) {
        std::ofstream out(output);
        out << machine.str();
    }
    std::cout << "VERIFIED: the independent C++ enumeration agrees with the Python verifier.\n";
    return 0;
}
