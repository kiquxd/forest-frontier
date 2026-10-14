#include "gameplay.hpp"
#include "json.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <chrono>
using namespace frontier;
static void check(bool condition,const char* reason) {if(!condition) throw std::runtime_error(reason);}
static int recipe(const std::string& id) {for(int i=0;i<int(recipes().size());i++) if(recipes()[i].id==id) return i;throw std::runtime_error("Missing recipe");}
int main() {
  try {
    configureWorld(742019);Save s;initializeCampaign(s);
    check(std::count(s.owners.begin(),s.owners.end(),0)==1&&ownsCity(s,s.homeCity),"Exactly one initial player city");
    check(length(s.player-settlements()[s.homeCity].pos)<180&&s.coins==0&&s.skills.empty(),"Start near own hall without skills or money");
    auto halls=makeBuildings();check(std::count_if(halls.begin(),halls.end(),[](auto& b){return b.hall;})==int(settlements().size()),"Every city has a hall");
    auto people=makeNpcs();check(std::count_if(people.begin(),people.end(),[](auto& n){return n.ruler;})==int(settlements().size()),"Every foreign city can supply a resident ruler");
    for(int i=0;i<int(s.owners.size());i++) for(int j=0;j<int(s.owners.size());j++) check(s.cityOpinions[i][j]==s.cityOpinions[j][i],"Reciprocal city relationships");
    Resource tree{1,{},ResourceKind::Tree,0};check(!harvest(s,tree).ok&&!craft(s,recipe("campfire")).ok&&!trade(s,s.homeCity,0,false).ok,"Starting actions locked");
    check(!trainSkill(s,6).ok&&!trainSkill(s,10).ok,"Advanced lessons need practice");
    check(lessonState(s,0).ready&&!lessonState(s,0).learned,"Introductory lesson is available in a fresh campaign");
    check(diplomacy(s,s.homeCity,10).ok,"Hall teaches gathering");
    check(lessonState(s,0).learned&&lessonState(s,0).progress==lessonState(s,0).target&&lessonState(s,1).ready,"Hall lesson immediately updates learned state and dependent lesson");
    check(!diplomacy(s,s.homeCity,10).ok&&s.skills.size()==1,"Repeated hall lesson cannot change progression");
    s.gathered=4;check(!lessonState(s,2).ready&&lessonState(s,2).progress==4&&lessonState(s,2).target==5,"Craftsmanship shows its exact practice progress");s.gathered=0;
    check(trainSkill(s,1).ok,"Free hall lessons bootstrap trade");
    for(int i=0;i<60;i++) {Resource resource{uint64_t(100+i),{},ResourceKind(i%4),0};check(harvest(s,resource).ok,"Gather without tools");}
    check(trainSkill(s,2).ok,"Gathering unlocks craft lesson");
    check(craft(s,recipe("campfire")).ok&&craft(s,recipe("campfire")).ok,"Two basic crafts without tool prerequisite");
    check(trainSkill(s,3).ok&&trainSkill(s,4).ok&&trainSkill(s,5).ok,"Practice unlocks tools, combat, fishing");
    check(craft(s,recipe("rod")).ok,"Craft fishing rod");
    for(int i=0;i<3;i++) check(harvest(s,{uint64_t(1000+i),{},ResourceKind::Fish,0}).ok,"Catch fish with rod");
    check(trainSkill(s,6).ok&&craft(s,recipe("boat")).ok,"Fishing unlocks first watercraft without coins or advanced technologies");
    check(trainSkill(s,8).ok&&trainSkill(s,9).ok,"Construction and diplomacy lessons");
    check(!diplomacy(s,s.homeCity,2).ok,"Cannot declare war on own city");
    check(diplomacy(s,s.homeCity,8).ok&&trainSkill(s,10).ok,"City activity unlocks engineering");
    int foreign=(s.homeCity+1)%int(s.owners.size());s.coins=500;s.relations[foreign]={40,Treaty::Peace,true};
    check(diplomacy(s,foreign,2).ok&&s.cityOpinions[s.homeCity][foreign]==-80,"War changes intercity relationship");
    check(diplomacy(s,foreign,1).ok&&s.cityOpinions[foreign][s.homeCity]==0,"Truce changes mutual relationship");
    s.relations[foreign].opinion=75;check(diplomacy(s,foreign,3).ok&&diplomacy(s,foreign,4).ok&&ownsCity(s,foreign),"Union transfers ownership");
    auto path=std::filesystem::temp_directory_path()/("campaign-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
    writeSave(path,s);auto loaded=loadSave(path);std::filesystem::remove(path);
    check(loaded.owners==s.owners&&loaded.skills==s.skills&&loaded.cityOpinions==s.cityOpinions&&loaded.caught==3,"Campaign and activities survive save roundtrip");
    Save legacy;legacy.inventory.ship=true;legacy.technologies.insert("submarine");auto oldPosition=legacy.player;initializeCampaign(legacy,true);
    check(legacy.skills.size()==skills().size()&&legacy.inventory.ship&&legacy.technologies.contains("submarine")&&length(legacy.player-oldPosition)==0,"Legacy migration preserves progress and position");
    // A migrated v3 slot keeps capabilities and gets an untouched backup.
    auto legacyPath=std::filesystem::temp_directory_path()/("campaign-v3-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
    writeSave(legacyPath,s);nlohmann::json json;{std::ifstream in(legacyPath);in>>json;}json["version"]=3;{std::ofstream out(legacyPath);out<<json;}
    auto migrated=loadSave(legacyPath);initializeCampaign(migrated,true);writeSave(legacyPath,migrated);
    check(std::filesystem::exists(legacyPath.string()+".v3.bak")&&migrated.inventory.boat&&migrated.skills.size()==skills().size(),"Old slot backed up without relocking equipment");
    std::filesystem::remove(legacyPath);std::filesystem::remove(legacyPath.string()+".v3.bak");
    // Reception policy changes an actual visitor's position, rather than only text.
    World world(s.seed);world.buildings.clear();int host=0;auto center=chunkAt(settlements()[host].pos);
    for(int y=-2;y<=2;y++) for(int x=-2;x<=2;x++) {Chunk chunk;chunk.coord={center.x+x,center.y+y};chunk.tiles.resize(256,Tile{0,false,Terrain::Forest});world.chunks.emplace(chunk.coord,std::move(chunk));}
    Save reception=s;reception.player=settlements()[host].pos+Vec{100,100};
    int visitor=-1,ruler=-1;for(auto& n:world.npcs) if(n.settlement==host) {if(n.originCity>=0) visitor=n.id;if(n.ruler) ruler=n.id;}
    check(visitor>=0&&ruler>=0,"Resident ruler and foreign visitor exist");
    auto rulerPosition=world.npcs[ruler].pos;reception.cityOpinions[host][world.npcs[visitor].originCity]=-80;
    world.tick(20,reception,0);
    check(length(world.npcs[visitor].pos-settlements()[host].pos)>200,"Hostile foreign visitor stays outside central square");
    world.beginConversation(ruler,reception);world.tick(1,reception,1);
    check(length(world.npcs[ruler].pos-rulerPosition)==0,"Ruler stays inside the hall during conversation");
    std::cout<<"campaign ownership, halls, relations, unlock path and persistence passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
