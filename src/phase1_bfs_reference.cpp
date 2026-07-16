#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <string>

constexpr uint32_t N = 11692800;

struct State {
    uint8_t cp, co;
    uint8_t ep[3];
    uint8_t eo; // bits 2,1,0 correspond pieces 23,27,28
};

struct Face {
    std::array<uint8_t,5> c;
    std::array<uint8_t,5> e;
    bool orient;
};

static constexpr Face faces[12] = {
    {{{0,1,2,3,4}}, {{0,1,2,3,4}}, false}, // U
    {{{0,4,9,11,5}}, {{4,9,14,16,5}}, true}, // R
    {{{1,0,5,10,6}}, {{0,5,10,15,6}}, true}, // F
    {{{2,1,6,14,7}}, {{1,6,11,19,7}}, true}, // L
    {{{3,2,7,13,8}}, {{2,7,12,18,8}}, true}, // BL
    {{{4,3,8,12,9}}, {{3,8,13,17,9}}, true}, // BR
    {{{15,16,17,18,19}}, {{25,26,27,28,29}}, false}, // D
    {{{15,19,14,6,10}}, {{29,24,11,15,20}}, true}, // FL
    {{{16,15,10,5,11}}, {{25,20,10,16,21}}, true}, // FR
    {{{17,16,11,9,12}}, {{26,21,14,17,22}}, true}, // DR
    {{{18,17,12,8,13}}, {{27,22,13,18,23}}, true}, // B
    {{{19,18,13,7,14}}, {{28,23,12,19,24}}, true}, // DL
};

inline uint8_t rank_excluding(uint8_t x, uint8_t a) {
    return x - (a < x);
}
inline uint8_t rank_excluding2(uint8_t x, uint8_t a, uint8_t b) {
    return x - (a < x) - (b < x);
}

inline uint32_t encode(const State &s) {
    uint32_t r2 = rank_excluding(s.ep[1], s.ep[0]);
    uint32_t r3 = rank_excluding2(s.ep[2], s.ep[0], s.ep[1]);
    uint32_t base = uint32_t(s.co) + 3u*uint32_t(s.cp) + 60u*uint32_t(s.ep[0]) + 1800u*r2 + 52200u*r3;
    return (base << 3) | s.eo;
}

inline uint8_t unrank_excluding(uint8_t r, uint8_t a) {
    return r + (r >= a);
}
inline uint8_t unrank_excluding2(uint8_t r, uint8_t a, uint8_t b) {
    // Return r-th element of [0,29] excluding a,b.
    uint8_t lo = std::min(a,b), hi = std::max(a,b);
    uint8_t x = r;
    if (x >= lo) ++x;
    if (x >= hi) ++x;
    return x;
}

inline State decode(uint32_t h) {
    State s{};
    s.eo = h & 7u;
    uint32_t base = h >> 3;
    s.co = base % 3u; base /= 3u;
    s.cp = base % 20u; base /= 20u;
    s.ep[0] = base % 30u; base /= 30u;
    uint8_t r2 = base % 29u; base /= 29u;
    uint8_t r3 = base; // 0..27
    s.ep[1] = unrank_excluding(r2, s.ep[0]);
    s.ep[2] = unrank_excluding2(r3, s.ep[0], s.ep[1]);
    return s;
}

inline void quarter_turn(State &s, const Face &f) {
    // position cycle source c[i] -> c[(i+1)%5]
    for (int i=0;i<5;i++) {
        if (s.cp == f.c[i]) {
            if (f.orient) s.co = (s.co + (i==0 ? 1 : 2)) % 3;
            s.cp = f.c[(i+1)%5];
            break;
        }
    }
    for (int k=0;k<3;k++) {
        for (int i=0;i<5;i++) {
            if (s.ep[k] == f.e[i]) {
                if (f.orient && (i==2 || i==4)) s.eo ^= (1u << (2-k));
                s.ep[k] = f.e[(i+1)%5];
                break;
            }
        }
    }
}

int main(int argc, char** argv) {
    std::vector<uint8_t> dist(N, 255);
    State solved{18,0,{23,27,28},0};
    uint32_t root = encode(solved);
    if (root >= N) { std::fprintf(stderr,"bad root %u\n",root); return 1; }
    dist[root]=0;
    std::vector<uint32_t> cur{root}, next;
    std::vector<uint64_t> counts;
    counts.push_back(1);
    auto t0=std::chrono::steady_clock::now();
    uint8_t depth=0;
    uint64_t seen=1;
    while(!cur.empty()) {
        next.clear();
        next.reserve(cur.size()*4);
        for(uint32_t h: cur) {
            State s0=decode(h);
            for(const auto &f: faces) {
                State s=s0;
                for(int p=1;p<=4;p++) {
                    quarter_turn(s,f);
                    uint32_t nh=encode(s);
                    if(dist[nh]==255) {
                        dist[nh]=depth+1;
                        next.push_back(nh);
                    }
                }
            }
        }
        if(next.empty()) break;
        depth++;
        seen += next.size();
        counts.push_back(next.size());
        auto now=std::chrono::steady_clock::now();
        double sec=std::chrono::duration<double>(now-t0).count();
        std::printf("depth %u: %zu (seen %llu) %.2fs\n",depth,next.size(),(unsigned long long)seen,sec);
        cur.swap(next);
    }
    std::printf("solved_eccentricity %u seen %llu / %u\n",depth,(unsigned long long)seen,N);
    for(size_t i=0;i<counts.size();i++) std::printf("%zu %llu\n",i,(unsigned long long)counts[i]);
    std::vector<uint32_t> antipodes;
    for(uint32_t h=0;h<N;h++) if(dist[h]==depth) antipodes.push_back(h);
    std::printf("antipodes %zu\n",antipodes.size());
    const std::filesystem::path output = argc > 1 ? argv[1] : "generated_reference/single_root";
    std::filesystem::create_directories(output);
    std::ofstream out(output / "phase1_depth10_ranks.bin",std::ios::binary);
    out.write(reinterpret_cast<const char*>(antipodes.data()), antipodes.size()*sizeof(uint32_t));
    std::ofstream dout(output / "phase1_distances.bin",std::ios::binary);
    dout.write(reinterpret_cast<const char*>(dist.data()), dist.size());
}
