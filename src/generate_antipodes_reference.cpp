#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <string>
#include <filesystem>

constexpr uint32_t N = 11692800;
struct State { uint8_t cp, co; uint8_t ep[3]; uint8_t eo; };
struct Face { std::array<uint8_t,5> c,e; bool orient; };
static constexpr Face faces[12] = {
    {{{0,1,2,3,4}}, {{0,1,2,3,4}}, false},
    {{{0,4,9,11,5}}, {{4,9,14,16,5}}, true},
    {{{1,0,5,10,6}}, {{0,5,10,15,6}}, true},
    {{{2,1,6,14,7}}, {{1,6,11,19,7}}, true},
    {{{3,2,7,13,8}}, {{2,7,12,18,8}}, true},
    {{{4,3,8,12,9}}, {{3,8,13,17,9}}, true},
    {{{15,16,17,18,19}}, {{25,26,27,28,29}}, false},
    {{{15,19,14,6,10}}, {{29,24,11,15,20}}, true},
    {{{16,15,10,5,11}}, {{25,20,10,16,21}}, true},
    {{{17,16,11,9,12}}, {{26,21,14,17,22}}, true},
    {{{18,17,12,8,13}}, {{27,22,13,18,23}}, true},
    {{{19,18,13,7,14}}, {{28,23,12,19,24}}, true},
};
inline uint32_t encode(const State&s){
 uint32_t r2=s.ep[1]-(s.ep[0]<s.ep[1]);
 uint32_t r3=s.ep[2]-(s.ep[0]<s.ep[2])-(s.ep[1]<s.ep[2]);
 return ((uint32_t(s.co)+3u*s.cp+60u*s.ep[0]+1800u*r2+52200u*r3)<<3)|s.eo;
}
inline uint8_t unrank1(uint8_t r,uint8_t a){return r+(r>=a);} 
inline uint8_t unrank2(uint8_t r,uint8_t a,uint8_t b){uint8_t lo=std::min(a,b),hi=std::max(a,b),x=r;if(x>=lo)++x;if(x>=hi)++x;return x;}
inline State decode(uint32_t h){State s{};s.eo=h&7;uint32_t b=h>>3;s.co=b%3;b/=3;s.cp=b%20;b/=20;s.ep[0]=b%30;b/=30;uint8_t r2=b%29;b/=29;s.ep[1]=unrank1(r2,s.ep[0]);s.ep[2]=unrank2(b,s.ep[0],s.ep[1]);return s;}
inline void turn(State&s,const Face&f){
 for(int i=0;i<5;i++)if(s.cp==f.c[i]){if(f.orient)s.co=(s.co+(i==0?1:2))%3;s.cp=f.c[(i+1)%5];break;}
 for(int k=0;k<3;k++)for(int i=0;i<5;i++)if(s.ep[k]==f.e[i]){if(f.orient&&(i==2||i==4))s.eo^=1u<<(2-k);s.ep[k]=f.e[(i+1)%5];break;}
}
std::vector<State> bfs(State root,int idx){
 std::vector<uint8_t> dist(N,255); std::vector<uint32_t> cur{encode(root)},next; dist[cur[0]]=0; uint8_t dep=0; uint64_t seen=1;
 while(!cur.empty()){
  next.clear(); next.reserve(cur.size()*4);
  for(uint32_t h:cur){State s0=decode(h);for(auto &f:faces){State s=s0;for(int p=0;p<4;p++){turn(s,f);uint32_t nh=encode(s);if(dist[nh]==255){dist[nh]=dep+1;next.push_back(nh);}}}}
  if(next.empty()) break;
  dep++;
  seen+=next.size();
  cur.swap(next);
 }
 std::vector<State>a;for(uint32_t h=0;h<N;h++)if(dist[h]==dep)a.push_back(decode(h));
 std::printf("block %d root c%d e%d,%d,%d solved_eccentricity %u depth10 %zu seen %llu\n",idx,root.cp,root.ep[0],root.ep[1],root.ep[2],dep,a.size(),(unsigned long long)seen);std::fflush(stdout);
 return a;
}
int main(int argc,char**argv){
 // face names encoded 0..11; canonical corner triples and edge pairs from repository comments.
 const int corners[20][3]={{0,1,2},{0,2,3},{0,3,4},{0,4,5},{0,5,1},{6,2,1},{7,3,2},{8,4,3},{9,5,4},{10,1,5},{2,6,7},{1,10,6},{5,9,10},{4,8,9},{3,7,8},{11,7,6},{11,6,10},{11,10,9},{11,9,8},{11,8,7}};
 const int edges[30][2]={{0,2},{0,3},{0,4},{0,5},{0,1},{1,2},{2,3},{3,4},{4,5},{5,1},{6,2},{7,3},{8,4},{9,5},{10,1},{2,7},{1,6},{5,10},{4,9},{3,8},{7,6},{6,10},{10,9},{9,8},{8,7},{11,6},{11,10},{11,9},{11,8},{11,7}};
 int start=argc>1?std::stoi(argv[1]):0; int end=argc>2?std::stoi(argv[2]):20;
 std::filesystem::path output=argc>3?argv[3]:"generated_reference/antipodes";
 std::filesystem::create_directories(output);
 for(int c=start;c<end;c++){
  std::array<uint8_t,3> inc{};int n=0;
  for(int e=0;e<30;e++){
   bool a=false,b=false;for(int k=0;k<3;k++){if(corners[c][k]==edges[e][0])a=true;if(corners[c][k]==edges[e][1])b=true;}
   if(a&&b)inc[n++]=e;
  }
  if(n!=3){std::fprintf(stderr,"corner %d incident %d\n",c,n);return 1;}
  std::sort(inc.begin(),inc.end());
  State root{uint8_t(c),0,{inc[0],inc[1],inc[2]},0};
  auto A=bfs(root,c);
  auto fn=output/("phase1_antipodes_block_"+std::to_string(c)+".bin"); std::ofstream out(fn,std::ios::binary); uint32_t sz=A.size(); const std::array<char,4> header{char(sz&255u),char((sz>>8u)&255u),char((sz>>16u)&255u),char((sz>>24u)&255u)}; out.write(header.data(),header.size());out.write((char*)inc.data(),3);out.write((char*)A.data(),A.size()*sizeof(State));
 }
}
