#include "content.hpp"
#include "json.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

namespace frontier {
static std::vector<nlohmann::json> files(const std::string& folder) {
  std::vector<std::filesystem::path> paths;
  for(auto& file:std::filesystem::directory_iterator(std::filesystem::path(FRONTIER_ROOT)/"content"/folder))
    if(file.path().extension()==".json") paths.push_back(file.path());
  std::sort(paths.begin(),paths.end());std::vector<nlohmann::json> out;std::unordered_set<std::string> ids;
  for(auto& p:paths) {
    std::ifstream f(p);nlohmann::json j;f>>j;
    if(!ids.insert(j.at("id").get<std::string>()).second) throw std::runtime_error("Duplicate content ID: "+p.string());
    out.push_back(std::move(j));
  }
  if(out.empty()) throw std::runtime_error("Empty content directory: "+folder);
  return out;
}
static std::array<int,goodTypes> costs(const nlohmann::json& j) {
  std::array<int,goodTypes> out{};
  for(auto it=j.begin();it!=j.end();++it) {
    int index=std::stoi(it.key()),amount=it.value().get<int>();
    if(index<0||index>=goodTypes||amount<0||amount>10000) throw std::runtime_error("Invalid recipe cost");
    out[index]=amount;
  }return out;
}
static Content load() {
  Content c;
  for(auto& j:files("npcs")) c.npcs.push_back({j.at("id"),j.at("role"),j.at("personality"),j.at("weight"),j.at("rarity"),j.at("color").get<std::array<int,3>>(),j.at("names").get<std::vector<std::string>>()});
  for(auto& j:files("weapons")) c.weapons.push_back({j.at("id"),j.at("name"),j.at("kind"),j.at("technology"),j.at("damage"),j.at("rarity"),j.at("weight"),j.at("reach"),j.at("duration"),costs(j.at("cost"))});
  for(auto& j:files("settlements")) c.towns.push_back({j.at("id"),j.at("name"),j.at("teacher"),j.at("technology"),j.at("weight"),j.at("rarity"),j.at("attitude"),j.at("color").get<std::array<int,3>>(),j.at("goods").get<std::vector<int>>()});
  auto all=files("recipes");
  const std::vector<std::string> order={"axe","pickaxe","sword","rod","boat","armor","potion","campfire","arrows","ship","steel","submarine","sonar","oxygen","repair"};
  for(auto& id:order) {
    auto it=std::find_if(all.begin(),all.end(),[&](auto& j){return j.at("id")==id;});
    if(it==all.end()) throw std::runtime_error("Missing core recipe: "+id);
    auto& j=*it;c.recipes.push_back({j.at("name"),costs(j.at("cost")),j.at("effect"),j.at("id"),j.at("result"),j.at("technology")});
  }
  for(auto& w:c.weapons) if(w.id!="wood_sword") c.recipes.push_back({w.name,w.cost,"Weapon: damage "+std::to_string(w.damage)+", reach "+std::to_string(int(w.reach)),"weapon_"+w.id,"weapon:"+w.id,w.technology});
  for(auto& j:all) if(std::find(order.begin(),order.end(),j.at("id").get<std::string>())==order.end()) c.recipes.push_back({j.at("name"),costs(j.at("cost")),j.at("effect"),j.at("id"),j.at("result"),j.at("technology"),j.value("quest_only",false)});
  for(auto& j:files("characters")) c.characters.push_back({j.at("id"),j.at("name"),j.at("lore"),j.at("pros"),j.at("cons"),j.at("speed"),j.at("gather"),j.at("damage"),j.at("taken"),j.at("trade"),j.at("coins")});
  for(auto& j:files("outfits")) c.outfits.push_back({j.at("id"),j.at("name"),j.at("description"),j.at("technology"),j.at("job"),j.at("speed"),j.at("gather"),j.at("taken"),j.at("trade"),costs(j.at("cost")),j.at("color").get<std::array<int,3>>(),j.at("quest_only")});
  for(auto& j:files("quests")) c.quests.push_back({j.at("id"),j.at("title"),j.at("lore"),j.at("objective"),j.at("knowledge"),j.at("recipe"),j.at("weapon"),j.at("outfit"),j.at("role"),j.at("good"),j.at("target"),j.at("coins"),costs(j.at("rewards"))});
  for(auto& o:c.outfits) if(o.id!="traveler") c.recipes.push_back({o.name,o.cost,o.description,"outfit_"+o.id,"outfit:"+o.id,o.technology});
  for(auto& n:c.npcs) if(n.weight<1||n.rarity<0||n.rarity>3||n.names.empty()) throw std::runtime_error("Invalid NPC: "+n.id);
  for(auto& w:c.weapons) if(w.weight<1||w.damage<1||w.reach<20||w.duration<.2f||w.rarity<0||w.rarity>3) throw std::runtime_error("Invalid weapon: "+w.id);
  for(auto& t:c.towns) if(t.weight<1||t.rarity<0||t.rarity>3||t.attitude<-100||t.attitude>100) throw std::runtime_error("Invalid town: "+t.id);
  return c;
}
const Content& content() {static const Content c=load();return c;}
const WeaponDef& weapon(const std::string& id) {
  auto& list=content().weapons;auto it=std::find_if(list.begin(),list.end(),[&](auto& w){return w.id==id;});
  if(it==list.end()) throw std::runtime_error("Unknown weapon: "+id);
  return *it;
}
int npcType(const std::string& id) {auto& v=content().npcs;for(size_t i=0;i<v.size();i++) if(v[i].id==id) return int(i);throw std::runtime_error("Unknown NPC type: "+id);}
int townType(const std::string& id) {auto& v=content().towns;for(size_t i=0;i<v.size();i++) if(v[i].id==id) return int(i);throw std::runtime_error("Unknown town type: "+id);}
template<class T> static int choose(const std::vector<T>& v,uint64_t n) {
  int total=0;for(auto& item:v) total+=item.weight;int roll=int(n%uint64_t(total));
  for(size_t i=0;i<v.size();i++) {roll-=v[i].weight;if(roll<0) return int(i);}return 0;
}
int weightedNpc(uint64_t n) {return choose(content().npcs,n);}
int weightedTown(uint64_t n) {return choose(content().towns,n);}
const WeaponDef& weightedWeapon(uint64_t n) {return content().weapons[choose(content().weapons,n)];}
template<class T> static const T& findDef(const std::vector<T>& list,const std::string& id) {for(auto& item:list) if(item.id==id) return item;throw std::runtime_error("Unknown content ID: "+id);}
const CharacterDef& character(const std::string& id) {return findDef(content().characters,id);}
const OutfitDef& outfit(const std::string& id) {return findDef(content().outfits,id);}
const QuestDef& quest(const std::string& id) {return findDef(content().quests,id);}
const char* rarityName(int r) {static const char* names[]={"common","rare","epic","legendary"};return r>=0&&r<4 ? names[r]:"";}
}
