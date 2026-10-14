#include "world.hpp"
#include "adventure.hpp"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <queue>

namespace frontier {
Vec operator+(Vec a, Vec b) { return {a.x+b.x, a.y+b.y}; }
Vec operator-(Vec a, Vec b) { return {a.x-b.x, a.y-b.y}; }
Vec operator*(Vec a, float b) { return {a.x*b, a.y*b}; }
float length(Vec a) { return std::sqrt(a.x*a.x+a.y*a.y); }
Vec normalized(Vec a) { auto l=length(a); return l>0 ? a*(1/l) : Vec{}; }
Vec project(Vec a) { return {(a.x-a.y)*0.8660254f, (a.x+a.y)*0.5f}; }
Vec unproject(Vec a) { return {a.y+a.x/1.7320508f, a.y-a.x/1.7320508f}; }
Coord chunkAt(Vec p) { return {static_cast<int>(std::floor(p.x/chunkSize)), static_cast<int>(std::floor(p.y/chunkSize))}; }
uint64_t hash(uint64_t n) {
  n+=0x9e3779b97f4a7c15ULL; n=(n^(n>>30))*0xbf58476d1ce4e5b9ULL;
  n=(n^(n>>27))*0x94d049bb133111ebULL; return n^(n>>31);
}
size_t CoordHash::operator()(Coord c) const { return hash((uint64_t(uint32_t(c.x))<<32)|uint32_t(c.y)); }
uint64_t objectId(int x, int y, uint64_t seed) { return hash(((uint64_t(uint32_t(x))<<32)|uint32_t(y))^seed); }
Terrain terrainAt(Vec p,uint64_t seed) {
  // Quantize before classifying so generation, collision and shoreline agree exactly.
  p={(std::floor(p.x/tileSize)+.5f)*tileSize,(std::floor(p.y/tileSize)+.5f)*tileSize};
  for(auto& s:settlements()) if(std::abs(p.x-s.pos.x)<370&&std::abs(p.y-s.pos.y)<370&&length(p-s.pos)<370) return Terrain::Forest;
  float variation=float(seed%97)*.025f;
  float ripple=std::sin(p.x*.006f+variation)*.01f+std::cos(p.y*.009f)*.01f;
  auto lake=[&](Vec center,float rx,float ry) {
    Vec d=p-center;return std::sqrt(d.x*d.x/(rx*rx)+d.y*d.y/(ry*ry))+ripple;
  };
  float shore=std::min(lake({1100,-1100},700,700),lake({2850,2050},850,850));
  // Further north lies an open coastline, with deterministic bays.
  shore=std::min(shore,(p.y+3100)/570+std::sin(p.x*.0018f+variation)*.13f);
  if(shore<.72f) return Terrain::Deep;
  if(shore<1.f) return Terrain::Shallow;
  if(shore<1.17f) return Terrain::Sand;
  return Terrain::Forest;
}
const char* terrainName(Terrain t) {
  switch(t) {case Terrain::Forest:return "Pine forest";case Terrain::Sand:return "Lake shore";
    case Terrain::Shallow:return "Shallows";case Terrain::Deep:return "Lake";}return "";
}
static std::vector<Settlement> towns;
static uint64_t configuredSeed=0;
void configureWorld(uint64_t seed) {
  if(!towns.empty()&&configuredSeed==seed) return;
  configuredSeed=seed;towns.clear();
  auto add=[&](std::string name,Vec pos,const std::string& type,int attitude=999) {
    int index=townType(type);auto& def=content().towns[index];
    towns.push_back({std::move(name),pos,index,def.rarity,attitude==999 ? def.attitude:attitude});
  };
  add("Pineford",{0,0},"village",20);add("Quiet Glade",{2000,700},"village",25);
  add("Saltwater Wharf",{-1000,-2300},"port",15);add("Ironhold",{3600,700},"forge",0);
  add("Moon Academy",{4200,-1200},"academy",10);add("Copper Beacon",{6000,-2400},"engineers",0);
  add("Pearl Haven",{3100,2900},"divers",10);add("Black Palisade",{-2200,1400},"raiders",-55);
  add("Archive of Tides",{-3600,2200},"ancients",-10);
  const char* names[]={"White", "Far", "North", "Stone", "Quiet", "River", "Green", "Old"};
  const char* suffix[]={"Bastion", "Port", "Frontier", "Cape", "Island", "Haven"};
  for(int y=-2;y<=3;y++) for(int x=-3;x<=3;x++) {
    auto n=objectId(x,y,seed^0x781aULL);if(n%100>=55) continue;
    Vec pos{float(x)*4800+float((n>>8)%1400)-700,float(y)*4800+float((n>>24)%1400)-700};
    bool close=false;for(auto& city:towns) if(length(city.pos-pos)<2700) close=true;if(close) continue;
    int type=weightedTown(hash(n));auto& def=content().towns[type];
    towns.push_back({std::string(names[(n>>32)%8])+" "+suffix[(n>>40)%6],pos,type,def.rarity,def.attitude});
  }
}
const std::vector<Settlement>& settlements() {if(towns.empty()) configureWorld(742019);return towns;}
Coord mapCell(Vec p) {return {int(std::floor(p.x/mapCellSize)),int(std::floor(p.y/mapCellSize))};}
bool exploredAt(const std::unordered_set<Coord,CoordHash>& cells,Vec p) {return cells.contains(mapCell(p));}
std::vector<Npc> makeNpcs() {
  std::vector<Npc> out;
  for(size_t city=0;city<settlements().size();city++) {
    auto& settlement=settlements()[city];auto& type=content().towns[settlement.type];
    int residents=6+settlement.rarity;
    for(int i=0;i<residents;i++) {
      auto n=objectId(int(city),i,configuredSeed^0x842ULL);
      int kind=i==0 ? npcType(type.teacher):i==1 ? npcType("elder"):i==residents-1 ? npcType("wanderer"):weightedNpc(n);
      auto& def=content().npcs[kind];float angle=i*6.2831853f/residents;
      Vec home=settlement.pos+Vec{std::cos(angle)*110,std::sin(angle)*110};
      out.push_back({int(out.size()),int(city),def.names[(n>>16)%def.names.size()],def.role,def.personality,home,home,float(i+city*3),home,0,kind,def.rarity});
      if(i==residents-1) {int origin=int(hash(n)%uint64_t(settlements().size()-1));if(origin>=int(city)) origin++;out.back().originCity=origin;}
      out.back().ruler=i==1;if(i==1) {out.back().role="Ruler";out.back().pos=out.back().home=out.back().previous=settlement.pos;}
    }
  }return out;
}
std::vector<Building> makeBuildings() {
  std::vector<Building> out;
  for(auto& city:settlements()) for(int i=0;i<5+city.rarity;i++) {
    float angle=i*6.2831853f/(5+city.rarity);
    out.push_back({city.pos+Vec{std::cos(angle)*235,std::sin(angle)*235},city.type%3});
  }
  for(auto& city:settlements()) out.push_back({city.pos,0,true});
  return out;
}
bool pathAt(Vec p) {
  for(auto& city:settlements()) if(std::abs(p.x-city.pos.x)<310&&std::abs(p.y-city.pos.y)<310&&length(p-city.pos)<290) return true;
  const auto a=settlements()[0].pos,b=settlements()[1].pos;const auto d=b-a;
  float t=std::clamp(((p.x-a.x)*d.x+(p.y-a.y)*d.y)/(d.x*d.x+d.y*d.y),0.f,1.f);
  return length(p-(a+d*t))<32;
}
Chunk generateChunk(Coord c, uint64_t seed) {
  const auto start=std::chrono::steady_clock::now();
  Chunk out; out.coord=c; out.tiles.reserve(chunkTiles*chunkTiles);
  for(int y=0;y<chunkTiles;y++) for(int x=0;x<chunkTiles;x++) {
    int tx=c.x*chunkTiles+x,ty=c.y*chunkTiles+y;
    const uint64_t n=objectId(tx,ty,seed);
    Vec center={(tx+.5f)*tileSize,(ty+.5f)*tileSize};
    bool path=pathAt(center);
    auto terrain=terrainAt(center,seed);
    out.tiles.push_back({uint8_t(n%8),path,terrain});
    if(path) continue;
    const unsigned chance=n%100;
    if(terrain!=Terrain::Forest) {
      if(terrain==Terrain::Sand&&chance<16) out.resources.push_back({n,center,ResourceKind::Reed,int((n>>32)%3)});
      if(terrain==Terrain::Shallow&&chance<9) out.resources.push_back({n,center,ResourceKind::Fish,int((n>>32)%3)});
      if(terrain==Terrain::Deep||terrain==Terrain::Shallow) {
        if(chance<4) out.underwater.push_back({hash(n^0xd1ULL),center,ResourceKind::Pearl,0});
        else if(chance<6) out.underwater.push_back({hash(n^0xd2ULL),center,ResourceKind::Crystal,0});
        else if(chance<8) out.underwater.push_back({hash(n^0xd3ULL),center,ResourceKind::Salvage,0});
      }
      continue;
    }
    if(chance<21) {
      auto kind=chance<14 ? ResourceKind::Tree : chance<18 ? ResourceKind::Stone : ResourceKind::Berry;
      Vec p=center+Vec{float((n>>16)%21)-10,float((n>>24)%21)-10};
      out.resources.push_back({n,p,kind,int((n>>32)%3)});
    }
    if(chance==93) out.resources.push_back({n,center,ResourceKind::Ore,0});
    if(chance==97 && out.animals.size()<4) out.animals.push_back({center,center,float((n>>8)%628)*.01f,center});
    if(chance==99&&out.enemies.empty()) {
      bool safe=false;for(auto& s:settlements()) if(length(center-s.pos)<500) safe=true;
      if(!safe) out.enemies.push_back({hash(n^0xabcdeULL),center,center,center});
    }
  }
  for(size_t city=0;city<settlements().size();city++) for(int guard=0;guard<2;guard++) {
    auto home=settlements()[city].pos+Vec{guard ? -315.f:315.f,40};
    if(chunkAt(home)!=c) continue;
    Enemy enemy{objectId(int(city),guard,seed^0xbcbcULL),home,home,home};enemy.kind=1;enemy.city=int(city);enemy.hp=100;
    out.enemies.push_back(enemy);
  }
  out.generationMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  return out;
}
Save loadSave(const std::filesystem::path& path) {
  Save s;
  if(!std::filesystem::exists(path)) return s;
  std::ifstream f(path); nlohmann::json j; f>>j;
  int version=j.at("version").get<int>();
  if(version<1||version>5) throw std::runtime_error("Unsupported save version");
  s.seed=j.at("seed").get<uint64_t>();
  s.player={j.at("player").at(0).get<float>(),j.at("player").at(1).get<float>()};
  if(!std::isfinite(s.player.x)||!std::isfinite(s.player.y)||length(s.player)>1e7f) throw std::runtime_error("Invalid player coordinates");
  s.inventory={j.at("wood").get<int>(),j.at("stone").get<int>(),j.at("berries").get<int>()};
  if(s.inventory.wood<0||s.inventory.stone<0||s.inventory.berries<0) throw std::runtime_error("Invalid inventory");
  for(auto& id:j.at("harvested")) s.harvested.insert(id.get<uint64_t>());
  if(version>=2) {
    auto& inv=j.at("inventory");
    s.inventory.reeds=inv.at("reeds");s.inventory.fish=inv.at("fish");s.inventory.leather=inv.at("leather");
    s.inventory.potions=inv.at("potions");s.inventory.campfires=inv.at("campfires");
    s.inventory.axe=inv.at("axe");s.inventory.pickaxe=inv.at("pickaxe");s.inventory.sword=inv.at("sword");
    s.inventory.rod=inv.at("rod");s.inventory.boat=inv.at("boat");s.inventory.armor=inv.at("armor");
    s.health=j.at("health");s.coins=j.at("coins");
    s.stocks.clear();for(auto& row:j.at("stocks")) {
      std::array<int,goodTypes> stock{};if(row.size()>goodTypes) throw std::runtime_error("Invalid stock size");
      for(size_t i=0;i<row.size();
      i++) stock[i]=row[i].get<int>();
      s.stocks.push_back(stock);
    }
    s.treasury=j.at("treasury").get<decltype(s.treasury)>();
    for(auto& id:j.at("defeated")) s.defeated.insert(id.get<uint64_t>());
    for(auto& p:j.at("campfires")) {
      Vec pos{p.at(0).get<float>(),p.at(1).get<float>()};
      if(!std::isfinite(pos.x)||!std::isfinite(pos.y)||length(pos)>1e7f) throw std::runtime_error("Invalid campfire");
      s.campfires.push_back(pos);
    }
    if(s.health<1||s.health>100||s.coins<0||s.coins>10000000||s.inventory.reeds<0||s.inventory.fish<0||s.inventory.leather<0||s.inventory.potions<0||s.inventory.campfires<0)
      throw std::runtime_error("Invalid gameplay save");
    for(auto& stock:s.stocks) for(auto count:stock) if(count<0||count>10000000) throw std::runtime_error("Invalid stock");
    for(int coins:s.treasury) if(coins<0||coins>10000000) throw std::runtime_error("Invalid treasury");
  }
  if(version>=3) {
    auto& inv=j.at("inventory");s.inventory.rare=inv.at("rare").get<decltype(s.inventory.rare)>();
    s.inventory.weapons=inv.at("weapons").get<decltype(s.inventory.weapons)>();s.inventory.weaponId=inv.at("weapon_id");
    s.inventory.ship=inv.at("ship");s.inventory.submarine=inv.at("submarine");s.inventory.sonar=inv.at("sonar");s.inventory.oxygen=inv.at("oxygen_module");
    s.inventory.ammo=inv.at("ammo");s.inventory.repairs=inv.at("repairs");
    s.technologies=j.at("technologies").get<decltype(s.technologies)>();
    for(auto& item:j.at("relations")) {
      int treaty=item.at("treaty");int opinion=item.at("opinion");
      if(treaty<0||treaty>4||opinion<-100||opinion>100) throw std::runtime_error("Invalid diplomacy state");
      s.relations.push_back({opinion,Treaty(treaty),item.at("discovered")});
    }
    auto readMap=[](const nlohmann::json& array,auto& cells) {
      for(auto& item:array) {Coord c{item.at(0).template get<int>(),item.at(1).template get<int>()};
        if(std::abs(c.x)>20000||std::abs(c.y)>20000) throw std::runtime_error("Invalid explored cell");
        cells.insert(c);}
    };
    readMap(j.at("explored"),s.explored);readMap(j.at("explored_underwater"),s.exploredUnderwater);
    for(auto& wall:j.at("walls")) {
      Wall w{wall.at("city"),{wall.at("a").at(0),wall.at("a").at(1)},{wall.at("b").at(0),wall.at("b").at(1)},wall.at("hp")};
      if(w.city<0||w.city>=int(s.relations.size())||!std::isfinite(length(w.a))||!std::isfinite(length(w.b))||length(w.b-w.a)>200||length(w.b-w.a)<1||w.hp<0||w.hp>1000)
        throw std::runtime_error("Invalid wall");
      s.walls.push_back(w);
    }
    s.submerged=j.at("submerged");s.oxygen=j.at("oxygen");s.hull=j.at("hull");s.vehicle=j.at("vehicle");
    if(!std::isfinite(s.oxygen)||s.oxygen<0||s.oxygen>200||s.hull<1||s.hull>100||s.vehicle<0||s.vehicle>3)
      throw std::runtime_error("Invalid vessel state");
    for(int n:s.inventory.rare) if(n<0||n>1000000) throw std::runtime_error("Invalid rare resource");
    for(auto& id:s.inventory.weapons) (void)weapon(id);
    if(!s.inventory.weaponId.empty()&&!s.inventory.weapons.contains(s.inventory.weaponId)) throw std::runtime_error("Invalid equipped weapon");
    if(s.inventory.ammo<0||s.inventory.ammo>1000000||s.inventory.repairs<0||s.inventory.repairs>1000000) throw std::runtime_error("Invalid supplies");
  } else if(s.inventory.sword) {s.inventory.weapons.insert("wood_sword");s.inventory.weaponId="wood_sword";}
  if(version>=4) {s.campaign=j.at("campaign");s.homeCity=j.at("home_city");s.owners=j.at("owners").get<decltype(s.owners)>();s.cityOpinions=j.at("city_opinions").get<decltype(s.cityOpinions)>();s.skills=j.at("skills").get<decltype(s.skills)>();s.gathered=j.at("gathered");s.crafted=j.at("crafted");s.caught=j.at("caught");s.orders=j.at("orders");
    if(s.campaign) {if(s.homeCity<0||s.homeCity>=int(s.owners.size())||s.cityOpinions.size()!=s.owners.size()||s.owners.size()!=s.relations.size()) throw std::runtime_error("Invalid campaign");for(auto& row:s.cityOpinions) {if(row.size()!=s.owners.size()) throw std::runtime_error("Invalid city matrix");for(int value:row) if(value<-100||value>100) throw std::runtime_error("Invalid opinion");}if(s.gathered<0||s.crafted<0||s.caught<0||s.orders<0) throw std::runtime_error("Invalid progress");}}
  if(version>=5) {
    s.adventure=j.at("adventure");s.characterId=j.at("character");s.outfitId=j.at("outfit");
    s.weaponCopies=j.at("weapon_copies").get<decltype(s.weaponCopies)>();s.outfitCopies=j.at("outfit_copies").get<decltype(s.outfitCopies)>();
    s.knownRecipes=j.at("known_recipes").get<decltype(s.knownRecipes)>();s.workClock=j.at("work_clock");s.trackedQuest=j.value("tracked_quest",-1);s.wolfKills=j.value("wolf_kills",0);
    if(s.wolfKills<0||s.trackedQuest<-1||s.trackedQuest>10000) throw std::runtime_error("Invalid quest counters");
    for(auto& row:j.at("quests")) {int npc=row.at("npc");QuestProgress progress{row.at("id"),row.at("completed"),row.at("baseline")};(void)quest(progress.id);if(npc<0||npc>10000||progress.baseline<0) throw std::runtime_error("Invalid quest progress");s.quests.emplace(npc,std::move(progress));}
    std::unordered_set<int> residentIds;
    for(auto& row:j.at("citizens")) {Citizen resident{row.at("npc"),row.at("city"),row.at("job"),row.at("weapon"),row.at("outfit"),row.at("trained")};
      if(resident.npc<0||resident.npc>10000||resident.city<0||resident.city>=int(s.relations.size())||!residentIds.insert(resident.npc).second||std::find(residentJobs().begin(),residentJobs().end(),resident.job)==residentJobs().end()) throw std::runtime_error("Invalid resident");
      if(!resident.weapon.empty()) (void)weapon(resident.weapon);
      if(!resident.outfit.empty()) (void)outfit(resident.outfit);
      s.citizens.push_back(std::move(resident));}
    (void)character(s.characterId);(void)outfit(s.outfitId);
    if(!std::isfinite(s.workClock)||s.workClock<0||s.workClock>=20) throw std::runtime_error("Invalid work clock");
    for(auto& [id,count]:s.weaponCopies) { (void)weapon(id);if(count<0||count>1000000) throw std::runtime_error("Invalid weapon count");}
    for(auto& [id,count]:s.outfitCopies) { (void)outfit(id);if(count<0||count>1000000) throw std::runtime_error("Invalid outfit count");}
    if(s.adventure) {if(availableOutfit(s,s.outfitId)<1||availableWeapon(s,s.inventory.weaponId)<1) throw std::runtime_error("Invalid equipped allocation");for(auto& [id,count]:s.weaponCopies) if(availableWeapon(s,id)<0) throw std::runtime_error("Duplicate weapon allocation");for(auto& [id,count]:s.outfitCopies) if(availableOutfit(s,id)<0) throw std::runtime_error("Duplicate outfit allocation");}
  }
  return s;
}
void writeSave(const std::filesystem::path& path, const Save& s) {
  if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
  nlohmann::json inv={{"reeds",s.inventory.reeds},{"fish",s.inventory.fish},{"leather",s.inventory.leather},
    {"potions",s.inventory.potions},{"campfires",s.inventory.campfires},{"axe",s.inventory.axe},{"pickaxe",s.inventory.pickaxe},
    {"sword",s.inventory.sword},{"rod",s.inventory.rod},{"boat",s.inventory.boat},{"armor",s.inventory.armor},
    {"rare",s.inventory.rare},{"weapons",s.inventory.weapons},{"weapon_id",s.inventory.weaponId},
    {"ship",s.inventory.ship},{"submarine",s.inventory.submarine},{"sonar",s.inventory.sonar},{"oxygen_module",s.inventory.oxygen},
    {"ammo",s.inventory.ammo},{"repairs",s.inventory.repairs}};
  nlohmann::json fires=nlohmann::json::array();for(auto p:s.campfires) fires.push_back({p.x,p.y});
  nlohmann::json diplomacy=nlohmann::json::array(),map=nlohmann::json::array(),underwaterMap=nlohmann::json::array(),walls=nlohmann::json::array();
  for(auto r:s.relations) diplomacy.push_back({{"opinion",r.opinion},{"treaty",int(r.treaty)},{"discovered",r.discovered}});
  for(auto c:s.explored) map.push_back({c.x,c.y});
  for(auto c:s.exploredUnderwater) underwaterMap.push_back({c.x,c.y});
  for(auto& w:s.walls) walls.push_back({{"city",w.city},{"a",{w.a.x,w.a.y}},{"b",{w.b.x,w.b.y}},{"hp",w.hp}});
  nlohmann::json j={{"version",5},{"seed",s.seed},{"player",{s.player.x,s.player.y}},
    {"wood",s.inventory.wood},{"stone",s.inventory.stone},{"berries",s.inventory.berries},{"harvested",s.harvested},
    {"inventory",inv},{"health",s.health},{"coins",s.coins},{"defeated",s.defeated},{"campfires",fires},
    {"stocks",s.stocks},{"treasury",s.treasury},{"relations",diplomacy},{"technologies",s.technologies},
    {"explored",map},{"explored_underwater",underwaterMap},{"walls",walls},
    {"submerged",s.submerged},{"oxygen",s.oxygen},{"hull",s.hull},{"vehicle",s.vehicle}};
  j["campaign"]=s.campaign;j["home_city"]=s.homeCity;j["owners"]=s.owners;j["city_opinions"]=s.cityOpinions;j["skills"]=s.skills;j["gathered"]=s.gathered;j["crafted"]=s.crafted;j["caught"]=s.caught;j["orders"]=s.orders;
  j["adventure"]=s.adventure;j["character"]=s.characterId;j["outfit"]=s.outfitId;j["weapon_copies"]=s.weaponCopies;j["outfit_copies"]=s.outfitCopies;j["known_recipes"]=s.knownRecipes;j["work_clock"]=s.workClock;j["tracked_quest"]=s.trackedQuest;j["wolf_kills"]=s.wolfKills;
  j["quests"]=nlohmann::json::array();for(auto& [id,q]:s.quests) j["quests"].push_back({{"npc",id},{"id",q.id},{"completed",q.completed},{"baseline",q.baseline}});
  j["citizens"]=nlohmann::json::array();for(auto& resident:s.citizens) j["citizens"].push_back({{"npc",resident.npc},{"city",resident.city},{"job",resident.job},{"weapon",resident.weapon},{"outfit",resident.outfit},{"trained",resident.trained}});
  if(std::filesystem::exists(path)) {
    std::ifstream existing(path);auto previous=nlohmann::json::parse(existing,nullptr,false);
    int oldVersion=previous.is_object() ? previous.value("version",0):0;
    auto backup=path;backup+=".v"+std::to_string(oldVersion)+".bak";
    if(oldVersion>0&&oldVersion<5&&!std::filesystem::exists(backup))
      std::filesystem::copy_file(path,backup);
  }
  auto temp=path; temp += ".tmp";
  { std::ofstream f(temp); if(!f) throw std::runtime_error("Cannot open save file"); f<<j.dump(2)<<'\n'; f.flush(); if(!f) throw std::runtime_error("Cannot write save"); }
  std::filesystem::rename(temp,path);
}
World::World(uint64_t seed):seed_(seed) {configureWorld(seed);npcs=makeNpcs();buildings=makeBuildings();worker_=std::thread([this]{run();});}
World::~World() {
  {std::lock_guard lock(mutex_); stopping_=true;} wake_.notify_all(); worker_.join();
}
void World::request(Vec player,int radius) {
  radius=std::clamp(radius,1,5);
  Coord center=chunkAt(player);
  std::vector<Coord> needed;
  std::lock_guard lock(mutex_);
  desired_.clear();
  for(int y=-radius;y<=radius;y++) for(int x=-radius;x<=radius;x++) {
    Coord c{center.x+x,center.y+y}; desired_.insert(c);
    if(!chunks.contains(c)&&!pending_.contains(c)) needed.push_back(c);
  }
  // Drop stale jobs before queueing nearest-first work. Queue is bounded by view radius.
  std::erase_if(jobs_,[&](Coord c){if(desired_.contains(c)) return false; pending_.erase(c); return true;});
  std::erase_if(ready_,[&](const Chunk& c){if(desired_.contains(c.coord)) return false;pending_.erase(c.coord);return true;});
  std::sort(needed.begin(),needed.end(),[&](Coord a,Coord b){
    return (a.x-center.x)*(a.x-center.x)+(a.y-center.y)*(a.y-center.y)<(b.x-center.x)*(b.x-center.x)+(b.y-center.y)*(b.y-center.y);
  });
  for(Coord c:needed) {jobs_.push_back(c); pending_.insert(c);}
  std::erase_if(chunks,[&](const auto& pair){return !desired_.contains(pair.first);});
  wake_.notify_one();
}
void World::run() {
  for(;;) {
    Coord c;
    {std::unique_lock lock(mutex_); wake_.wait(lock,[&]{return stopping_||!jobs_.empty();});
      if(stopping_) return;
      c=jobs_.front(); jobs_.pop_front();}
    Chunk chunk=generateChunk(c,seed_);
    {std::lock_guard lock(mutex_);
      if(desired_.contains(c)) ready_.push_back(std::move(chunk)); else pending_.erase(c);
    }
  }
}
int World::integrate(int budget) {
  std::lock_guard lock(mutex_); int count=0;
  while(!ready_.empty()&&count<budget) {
    Chunk c=std::move(ready_.front()); ready_.pop_front(); pending_.erase(c.coord);
    generatedMs+=c.generationMs; maxGenerationMs=std::max(maxGenerationMs,c.generationMs); generatedCount++;
    if(desired_.contains(c.coord)) chunks.insert_or_assign(c.coord,std::move(c));
    count++;
  }
  return count;
}
size_t World::pending() const { std::lock_guard lock(mutex_); return pending_.size(); }
void World::tick(float dt,const Save& save,double time) {
  if(save.submerged) return;
  Vec player=save.player;
  for(auto& n:npcs) if(length(n.home-player)<2200) {
    n.previous=n.pos;
    if(n.ruler) continue;
    if(n.id==talkingNpc) {n.walk=0;continue;}
    Vec target=n.home+Vec{std::sin(float(time)*.16f+n.phase)*50,std::cos(float(time)*.13f+n.phase)*40};
    if(n.originCity>=0&&save.campaign&&save.cityOpinions[n.settlement][n.originCity]<-25) target=target+normalized(n.home-settlements()[n.settlement].pos)*180;
    if(auto resident=citizen(save,n.id);resident&&resident->trained&&resident->job=="guard") target=target+normalized(n.home-settlements()[n.settlement].pos)*160;
    Vec diff=target-n.pos; n.pos=move(n.pos,normalized(diff)*std::min(length(diff),dt*24),save,10);
    n.walk+=length(n.pos-n.previous)/10;
  }
  for(auto& [coord,c]:chunks) for(auto& a:c.animals) if(length(a.home-player)<1100) {
    a.previous=a.pos;
    Vec target=a.home+Vec{std::sin(float(time)*.21f+a.phase)*70,std::cos(float(time)*.17f+a.phase)*60};
    if(length(a.pos-player)<95) target=a.pos+normalized(a.pos-player)*120;
    Vec d=target-a.pos; a.pos=move(a.pos,normalized(d)*std::min(length(d),dt*48),save,6);
    if(length(a.pos-a.home)>150) a.pos=a.home+normalized(a.pos-a.home)*150;
    a.walk+=length(a.pos-a.previous)/8;
  }
}
const Resource* World::nearest(Vec p,const std::unordered_set<uint64_t>& removed,float reach) const {
  const Resource* out=nullptr; float best=reach;
  Coord center=chunkAt(p);
  for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
    auto it=chunks.find({center.x+x,center.y+y}); if(it==chunks.end()) continue;
    for(auto& r:it->second.resources) if(!removed.contains(r.id)) {
      float d=length(r.pos-p); if(d<best) {best=d; out=&r;}
    }
  } return out;
}
const Resource* World::nearest(Vec p,const Save& save,float reach) const {
  if(!save.submerged) return nearest(p,save.harvested,reach);
  const Resource* found=nullptr;float best=reach;auto center=chunkAt(p);
  for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
    auto it=chunks.find({center.x+x,center.y+y});if(it==chunks.end()) continue;
    for(auto& r:it->second.underwater) if(!save.harvested.contains(r.id)) {
      float d=length(r.pos-p);if(d<best) {best=d;found=&r;}
    }
  }return found;
}
void World::beginConversation(int id,const Save& save) {
  talkingNpc=id;if(id<0||id>=int(npcs.size())) return;
  auto& n=npcs[id];if(n.ruler) return;
  if(length(n.pos-save.player)>85) {
    for(int i=0;i<8;i++) {
      float a=i*.785398f;Vec p=save.player+Vec{std::cos(a)*62,std::sin(a)*62};
      if(clear(p,save,10)) {n.pos=p;break;}
    }
  }
  n.previous=n.pos;n.walk=0;
}
bool World::clear(Vec p,const Save& save,float radius,bool boat) const {
  for(Vec offset:std::array<Vec,5>{{{0,0},{radius,0},{-radius,0},{0,radius},{0,-radius}}}) {
    Vec sample=p+offset;Coord coord=chunkAt(sample);
    auto it=chunks.find(coord);if(it==chunks.end()) return false;
    int x=int(std::floor(sample.x/tileSize))-coord.x*chunkTiles;
    int y=int(std::floor(sample.y/tileSize))-coord.y*chunkTiles;
    auto terrain=it->second.tiles[y*chunkTiles+x].terrain;
    if(save.submerged&&terrain!=Terrain::Deep&&terrain!=Terrain::Shallow) return false;
    if(!save.submerged&&!boat&&terrain==Terrain::Deep) return false;
  }
  if(save.submerged) return true;
  auto close=[&](Vec q,float r) {Vec d=p-q;return d.x*d.x+d.y*d.y<r*r;};
  for(auto& b:buildings) if(std::abs(b.pos.x-p.x)<radius+42&&std::abs(b.pos.y-p.y)<radius+42&&close(b.pos,radius+42)) return false;
  for(auto& wall:save.walls) if(wall.hp>0) {
    Vec ab=wall.b-wall.a,ap=p-wall.a;float square=ab.x*ab.x+ab.y*ab.y;
    float t=std::clamp((ap.x*ab.x+ap.y*ab.y)/std::max(square,1.f),0.f,1.f);
    if(close(wall.a+ab*t,radius+7)) return false;
  }
  auto low=chunkAt(p-Vec{radius+18,radius+18}),high=chunkAt(p+Vec{radius+18,radius+18});
  for(int y=low.y;y<=high.y;y++) for(int x=low.x;x<=high.x;x++) {
    auto it=chunks.find({x,y});if(it==chunks.end()) continue;
    for(auto& r:it->second.resources) {
      if(r.kind!=ResourceKind::Tree&&r.kind!=ResourceKind::Stone&&r.kind!=ResourceKind::Ore) continue;
      if(save.harvested.contains(r.id)) continue;
      if(close(r.pos,radius+(r.kind==ResourceKind::Tree ? 10:14))) return false;
    }
  }
  return true;
}
Vec World::move(Vec p,Vec displacement,const Save& save,float radius,bool boat) const {
  int steps=std::clamp(int(std::ceil(length(displacement)/6)),1,256);
  Vec step=displacement*(1.f/steps);
  for(int i=0;i<steps;i++) {
    if(clear(p+step,save,radius,boat)) p=p+step;
    else {
      if(clear(p+Vec{step.x,0},save,radius,boat)) p.x+=step.x;
      if(clear(p+Vec{0,step.y},save,radius,boat)) p.y+=step.y;
    }
  }
  return p;
}
std::vector<Vec> World::path(Vec start,Vec goal,const Save& save) const {
  // Short plans keep work bounded; a distant goal is approached in successive segments.
  if(length(goal-start)>900) goal=start+normalized(goal-start)*900;
  auto cell=[](Vec p){return Coord{int(std::floor(p.x/tileSize)),int(std::floor(p.y/tileSize))};};
  auto center=[](Coord c){return Vec{(c.x+.5f)*tileSize,(c.y+.5f)*tileSize};};
  Coord from=cell(start),to=cell(goal);
  if(!clear(center(to),save,12,(save.inventory.boat||save.inventory.ship||save.inventory.submarine))) {
    float best=1e9;Coord candidate=from;
    for(int y=-5;y<=5;y++) for(int x=-5;x<=5;x++) {
      Coord c{to.x+x,to.y+y};float d=length(center(c)-goal);
      if(d<best&&clear(center(c),save,12,(save.inventory.boat||save.inventory.ship||save.inventory.submarine))) {best=d;candidate=c;}
    }to=candidate;
  }
  struct Node {float rank;Coord c;bool operator<(const Node& b) const{return rank>b.rank;}};
  struct Record {float cost;Coord parent;};
  std::priority_queue<Node> open;std::unordered_map<Coord,Record,CoordHash> records;
  open.push({0,from});records.emplace(from,Record{0,from});
  Coord reached=from;float nearest=length(center(from)-center(to));int visited=0;
  while(!open.empty()&&visited++<1800) {
    Coord c=open.top().c;open.pop();float cost=records.at(c).cost;
    float d=length(center(c)-center(to));if(d<nearest) {nearest=d;reached=c;}
    if(c==to) {reached=c;break;}
    for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
      if(!x&&!y) continue;
      Coord next{c.x+x,c.y+y};
      if(std::abs(next.x-from.x)>32||std::abs(next.y-from.y)>32) continue;
      if(!clear(center(next),save,12,(save.inventory.boat||save.inventory.ship||save.inventory.submarine))) continue;
      if(x&&y&&(!clear(center({c.x+x,c.y}),save,12,(save.inventory.boat||save.inventory.ship||save.inventory.submarine))||!clear(center({c.x,c.y+y}),save,12,(save.inventory.boat||save.inventory.ship||save.inventory.submarine)))) continue;
      float weight=(x&&y ? 1.4142f:1.f)*(terrainAt(center(next),seed_)==Terrain::Shallow ? 1.8f:1.f);
      float candidate=cost+weight;
      auto found=records.find(next);if(found!=records.end()&&found->second.cost<=candidate) continue;
      records.insert_or_assign(next,Record{candidate,c});
      open.push({candidate+length(center(next)-center(to))/tileSize,next});
    }
  }
  std::vector<Vec> out;
  while(reached!=from) {out.push_back(center(reached));reached=records.at(reached).parent;}
  std::reverse(out.begin(),out.end());return out;
}
}
