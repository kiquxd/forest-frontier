#include "world.hpp"
#include "perf.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace frontier;
static void check(bool condition,const char* message) {if(!condition) throw std::runtime_error(message);}
int main() {
  try {
    check(chunkAt({-.1f,-768.1f})==Coord{-1,-2},"Negative chunk coordinates");
    for(Vec p: {Vec{0,0},Vec{-124,895},Vec{2300,-2000}}) check(length(unproject(project(p))-p)<.001f,"Projection round trip");
    auto a=generateChunk({-2,3},742019),b=generateChunk({-2,3},742019),c=generateChunk({-2,3},5);
    check(a.tiles.size()==256,"Chunk tiles");check(a.resources.size()==b.resources.size(),"Deterministic resource count");
    for(size_t i=0;i<a.resources.size();i++) {
      auto& r=a.resources[i];check(r.id==b.resources[i].id&&r.pos.x==b.resources[i].pos.x&&r.pos.y==b.resources[i].pos.y,"Deterministic resource positions");
      check(chunkAt(r.pos)==a.coord,"Resource stays in chunk");
    }
    check(!a.resources.empty()&&!c.resources.empty()&&a.resources.front().id!=c.resources.front().id,"Seed changes world");
    auto temp=std::filesystem::temp_directory_path()/("frontier-test-"+std::to_string(hash(uint64_t(Clock::now().time_since_epoch().count()))));
    std::filesystem::create_directories(temp);
    Save save;save.player={-50,125};save.inventory={7,3,2};save.harvested.insert(a.resources.front().id);
    writeSave(temp/"world.json",save);auto loaded=loadSave(temp/"world.json");
    check(loaded.seed==save.seed&&loaded.harvested==save.harvested&&loaded.player.x==-50&&loaded.inventory.wood==7,"Save round trip preserves uint64 IDs");
    World w(save.seed);w.request({-900,2700},1);
    auto deadline=Clock::now()+std::chrono::seconds(3);
    while(w.chunks.size()<9&&Clock::now()<deadline) {w.integrate();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    check(w.chunks.size()==9&&w.pending()==0,"Async generation complete");
    Vec rp=a.resources.front().pos;
    check(w.nearest(rp,std::unordered_set<uint64_t>{},5)!=nullptr,"Resource lookup");
    check(w.nearest(rp,save.harvested,5)==nullptr,"Harvest stays removed");
    w.request({10000,-10000},1);w.integrate();
    deadline=Clock::now()+std::chrono::seconds(3);
    while(w.chunks.size()<9&&Clock::now()<deadline) {w.integrate();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    check(w.chunks.size()==9,"Bounded resident chunk cache");
    for(auto& [coord,chunk]:w.chunks) check(std::abs(coord.x-chunkAt({10000,-10000}).x)<=1,"Old chunks evicted");
    {Perf perf(temp);perf.frame(10,1,2,7,false);perf.frame(30,1,2,27,true);perf.flush();check(perf.p95==30&&perf.fps==50,"Frame statistics");}
    std::filesystem::remove_all(temp);
    std::cout<<"world, persistence, worker, projection and telemetry checks passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
