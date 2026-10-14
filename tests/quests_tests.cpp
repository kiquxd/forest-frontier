#include "adventure.hpp"
#include "save_slots.hpp"
#include "json.hpp"
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace frontier;
static void check(bool ok,const char* reason) {if(!ok) throw std::runtime_error(reason);}
static int recipeIndex(const std::string& id) {for(int i=0;i<int(recipes().size());i++) if(recipes()[i].id==id) return i;throw std::runtime_error("Missing recipe");}
int main() {
  auto folder=std::filesystem::temp_directory_path()/("frontier-quests-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    configureWorld(742019);Save s;initializeCampaign(s);initializeAdventure(s);auto npcs=makeNpcs();
    check(s.citizens.size()>=4&&s.outfitCopies.at("traveler")==1,"New campaign has an owned resident roster and one basic outfit");
    int offers=0,giver=-1,hunter=-1;for(auto& n:npcs) {auto q=npcQuest(s,n);if(q) {offers++;if(q->id=="supplies"&&giver<0) giver=n.id;if(q->id=="wolfwatch") hunter=n.id;}check(!npcIntroduction(s,n).empty(),"Every NPC has authored lore");}
    check(offers>0&&offers<int(npcs.size())&&giver>=0,"Only some NPCs offer quests");auto& n=npcs[giver];s.relations[n.settlement]={30,Treaty::Peace,true};
    check(acceptQuest(s,n).ok&&!acceptQuest(s,n).ok,"Accept quest exactly once");int coins=s.coins;
    check(!finishQuest(s,n).ok&&s.coins==coins&&!s.knownRecipes.contains("outfit_forager"),"Incomplete quest cannot grant rewards");
    check(trainSkill(s,0).ok,"First skill available without funds");
    for(int i=0;i<2;i++) check(harvest(s,{uint64_t(100+i),{},ResourceKind::Tree,0}).ok,"Gather quest supplies");
    check(finishQuest(s,n).ok&&s.inventory.wood==0&&s.coins==10&&s.knownRecipes.contains("outfit_forager"),"Turn-in consumes requested goods and grants coins and recipe");
    check(!finishQuest(s,n).ok&&s.coins==10,"Quest rewards cannot be duplicated");
    for(int i=0;i<4;i++) harvest(s,{uint64_t(200+i),{},ResourceKind::Tree,0});
    for(int i=0;i<3;i++) harvest(s,{uint64_t(300+i),{},ResourceKind::Reed,0});
    check(trainSkill(s,2).ok&&craft(s,recipeIndex("outfit_forager")).ok,"Quest recipe makes usable clothing");
    s.player={1100,-1100};s.submerged=true;
    check(equipOutfit(s,"forager").ok&&gatherTimeMultiplier(s)<1,"Clothes swap underwater and apply gathering perk");
    check(!craft(s,recipeIndex("outfit_artisan")).ok,"Unknown quest recipes remain locked");
    s.inventory.wood=60;s.inventory.reeds=60;s.inventory.stone=60;
    check(craft(s,recipeIndex("outfit_forager")).ok&&s.outfitCopies.at("forager")==2,"Craft spare clothes");
    check(trainSkill(s,1).ok&&trainSkill(s,4).ok,"Practice supports initial training skills");
    check(craft(s,2).ok&&craft(s,2).ok&&s.weaponCopies.at("wood_sword")==2,"Spare weapons use counted copies");
    check(equipWeapon(s,"wood_sword").ok,"Weapon swaps have no location restriction");
    s.coins=100;check(research(s,0).ok,"Research combines funds, residents, skills, clothes and completed quests");
    int first=s.citizens[0].npc,second=s.citizens[1].npc;
    check(assignCitizen(s,first,"gatherer","","forager").ok,"Train and equip an owned resident");
    check(!assignCitizen(s,second,"gatherer","","forager").ok,"Player and resident cannot share the same clothing copy");
    int stock=s.stocks[s.citizens[0].city][0];tickResidents(s,20);
    check(s.stocks[s.citizens[0].city][0]==stock+1,"Trained gatherer contributes to actual town stock");
    check(!equipOutfit(s,"artisan").ok,"Cannot equip clothing that is not owned");
    check(researchRequirements(s,researchNodes()[5]).find("Avg. foreign trust")!=std::string::npos&&!research(s,5).ok,"Multi-town leadership enforces trust and settlement requirements");
    s.outfitCopies["hunter"]=1;check(research(s,1).ok&&assignCitizen(s,second,"guard","wood_sword","hunter").ok,"Militia research trains an equipped defender");
    s.submerged=false;s.player={120,140};{World world(s.seed);world.buildings.clear();Chunk flat;flat.coord={0,0};flat.tiles.resize(256,Tile{0,false,Terrain::Forest});world.chunks.emplace(flat.coord,std::move(flat));Gameplay game;
    int copies=s.weaponCopies["wood_sword"];for(int i=0;i<12;i++) {game.advance(1);check(game.attack(world,s).ok,"Repeated sword attacks");game.advance(.2f);game.combat(world,s,.2f);}
    check(s.weaponCopies["wood_sword"]==copies,"Attacks never consume weapons");
    s.characterId="warden";auto& chunk=world.chunks.at({0,0});Enemy target{99999,s.player+Vec{80,0},s.player+Vec{80,0},s.player+Vec{80,0}};chunk.enemies.push_back(target);
    game.advance(1);game.attack(world,s);game.advance(.2f);game.combat(world,s,.2f);check(chunk.enemies[0].hp==30,"Warden attack bonus applies to real hits");s.characterId="traveler";
    world.chunks.clear();auto center=chunkAt(world.npcs[second].pos);for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {Chunk c;c.coord={center.x+x,center.y+y};c.tiles.resize(256,Tile{0,false,Terrain::Forest});world.chunks.emplace(c.coord,std::move(c));}
    s.player=settlements()[s.homeCity].pos+Vec{110,110};auto enemyPos=world.npcs[second].pos+Vec{50,0};auto& defense=world.chunks.at(chunkAt(enemyPos));defense.enemies.push_back({99998,enemyPos,enemyPos,enemyPos});game.tick(world,s,1.6f,0);
    check(defense.enemies[0].hp<60&&s.weaponCopies["wood_sword"]==copies,"Equipped residents protect against nearby wolves without consuming weapons");}
    if(hunter>=0) {auto& h=npcs[hunter];s.relations[h.settlement]={30,Treaty::Peace,true};s.wolfKills=3;check(acceptQuest(s,h).ok&&questProgress(s,s.quests.at(h.id))==0,"Kill quests do not count earlier kills");s.wolfKills++;check(questProgress(s,s.quests.at(h.id))==1,"New wolf defeat counts");}
    std::filesystem::create_directories(folder);auto path=folder/"world.json";writeSave(path,s);auto loaded=loadSave(path);
    check(loaded.knownRecipes==s.knownRecipes&&loaded.weaponCopies==s.weaponCopies&&loaded.outfitId=="forager"&&loaded.citizens[0].job=="gatherer"&&loaded.quests.at(giver).completed,"New progression and equipment survive save roundtrip");
    auto legacy=folder/"legacy.json";Save old;old.inventory.sword=true;old.inventory.weapons.insert("wood_sword");old.inventory.weaponId="wood_sword";old.inventory.armor=true;initializeCampaign(old,true);writeSave(legacy,old);
    nlohmann::json json;{std::ifstream in(legacy);in>>json;}json["version"]=4;{std::ofstream out(legacy);out<<json;}
    auto migrated=loadSave(legacy);initializeAdventure(migrated);writeSave(legacy,migrated);
    check(migrated.weaponCopies.at("wood_sword")==1&&migrated.outfitCopies.at("leather_armor")==1&&std::filesystem::exists(legacy.string()+".v4.bak"),"v4 equipment migrates without loss and retains backup");
    for(auto& hero:content().characters) {auto characterPath=createSaveSlot(folder/"characters",hero.id,123,hero.id);auto characterSave=loadSave(characterPath);check(characterSave.characterId==hero.id&&characterSave.coins==hero.coins,"Selected background and start bonus persist");}
    check(character("scout").speed>1&&character("scout").taken>1&&character("artisan").gather<1&&character("artisan").damage<1,"Backgrounds have distinct positive and negative modifiers");
    std::filesystem::remove_all(folder);std::cout<<"quests, recipe rewards, backgrounds, perks, resident jobs, gear conservation and migration passed\n";
  } catch(const std::exception& e) {std::filesystem::remove_all(folder);std::cerr<<e.what()<<'\n';return 1;}
}
