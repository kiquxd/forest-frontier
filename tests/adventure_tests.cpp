#include "gameplay.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <stdexcept>

using namespace frontier;
static void check(bool condition,const char* reason) {if(!condition) throw std::runtime_error(reason);}
static void flat(World& world) {
  world.chunks.clear();world.buildings.clear();
  for(int y=-2;y<=3;y++) for(int x=-2;x<=3;x++) {Chunk c;c.coord={x,y};c.tiles.resize(256,Tile{0,false,Terrain::Forest});world.chunks.emplace(c.coord,std::move(c));}
}
int main() {
  try {
    check(content().npcs.size()>=20&&content().weapons.size()>=14&&recipes().size()>=28,"Content catalogs loaded");
    std::array<int,4> counts{};
    for(int i=0;i<20000;i++) counts[content().npcs[weightedNpc(hash(i))].rarity]++;
    check(counts[0]>counts[1]&&counts[1]>counts[2]&&counts[2]>counts[3]&&counts[3]>0,"Rarity weights produce rarer NPCs less frequently");
    configureWorld(742019);auto cities=settlements();configureWorld(10);auto other=settlements();configureWorld(742019);
    check(cities.size()==settlements().size()&&cities.size()>12,"Seeded towns generated");
    check(other.back().pos.x!=cities.back().pos.x,"Seed changes distant settlements");
    for(size_t i=0;i<cities.size();i++) check(cities[i].pos.x==settlements()[i].pos.x&&cities[i].type==settlements()[i].type,"Deterministic town generation");
    Save s;ensureCityState(s);s.coins=2000;s.inventory.wood=200;s.inventory.stone=200;s.inventory.fish=100;
    int gold=s.coins+s.treasury[0];
    check(learnTechnology(s,0).ok&&s.technologies.contains("ironworking"),"Learn from friendly city's master");
    check(gold==s.coins+s.treasury[0],"Research conserves money");
    check(!trade(s,2,7,true).ok,"Rare material locked by technology and reputation");
    check(diplomacy(s,2,0).ok&&learnTechnology(s,2).ok&&trade(s,2,7,true).ok,"Trust unlocks ship technology and copper");
    check(!trade(s,0,7,true).ok,"Other towns cannot supply exclusive material");
    check(diplomacy(s,0,2).ok&&s.relations[0].treaty==Treaty::War&&!trade(s,0,0,true).ok,"War closes market");
    check(!learnTechnology(s,0).ok,"War blocks technology sharing");
    check(diplomacy(s,0,1).ok&&s.relations[0].treaty==Treaty::Truce&&trade(s,0,0,true).ok,"Truce reopens market");
    for(int i=0;i<2;i++) check(diplomacy(s,0,0).ok,"Gift builds trust");
    check(diplomacy(s,0,3).ok,"Alliance requires trust");
    for(int i=0;i<3;i++) check(diplomacy(s,0,8).ok,"City order improves relations");
    check(diplomacy(s,0,4).ok&&s.relations[0].treaty==Treaty::Union,"Alliance can become union");
    int wood=s.inventory.wood,stone=s.inventory.stone;
    check(diplomacy(s,0,6).ok&&s.walls.size()==1&&s.inventory.wood==wood-8&&s.inventory.stone==stone-3,"Walls consume resources");
    for(int i=0;i<10;i++) check(diplomacy(s,0,6).ok,"Build connected ring");
    check(!diplomacy(s,0,6).ok&&s.walls.size()==11,"Wall ring leaves a gate");
    check(cityWeapon(0,s.seed).rarity>=1&&cityWeapon(8,s.seed).rarity==3,"City offers respect rarity");
    for(auto& type:content().towns) s.technologies.insert(type.technology);s.inventory.rare.fill(100);s.inventory.reeds=100;
    for(auto id:{"ship","submarine","sonar","oxygen"}) {
      int recipe=-1;for(size_t i=0;i<recipes().size();i++) if(recipes()[i].id==id) recipe=int(i);
      check(craft(s,recipe).ok,"Advanced craft chain");
    }
    s.player={1100,-1100};check(dive(s).ok&&s.submerged&&activeVehicle(s)==3,"Crafted submarine enables diving");
    check(sonar(s).ok&&!s.exploredUnderwater.empty()&&s.explored.empty(),"Sonar reveals underwater only");
    check(dive(s).ok&&!s.submerged,"Surface independently");
    World world(s.seed);flat(world);Gameplay game;
    s.submerged=false;s.vehicle=1;s.player={1100,-1100};game.walk=2;
    game.movePlayer(world,s,{8,0});check(game.walk==2,"Water vehicle movement cannot animate walking legs");
    check(!game.attack(world,s).ok,"Cannot swing a sword inside a vessel");
    s.player={120,140};s.walls.clear();s.explored.clear();s.exploredUnderwater.clear();
    game.explore(s,1000,600,1);
    check(exploredAt(s.explored,s.player)&&!exploredAt(s.explored,{1100,-1100}),"Map cannot reveal unseen lake");
    game.explore(s,1000,600,1);auto known=s.explored.size();s.player={1100,-450};game.explore(s,1000,600,1);
    check(s.explored.size()>known&&!exploredAt(s.explored,{1100,-1800}),"Exploration reveals only part of lake");
    s.player={120,140};world.beginConversation(0,s);Vec speaker=world.npcs[0].pos;
    check(length(speaker-s.player)<=85,"Speaker stands beside player");
    for(int i=0;i<30;i++) world.tick(.1f,s,i*.1);
    check(length(speaker-world.npcs[0].pos)<.001,"Speaker does not wander while speaking");
    world.talkingNpc=-1;world.tick(.1f,s,4);check(length(world.npcs[0].pos-speaker)>0,"Speaker resumes normal behavior");
    s.player={560,400};s.inventory.weaponId="wood_sword";s.inventory.weapons.insert("wood_sword");
    auto& enemies=world.chunks.at({0,0}).enemies;enemies.push_back({999,{620,400},{620,400},{620,400}});
    game.attack(world,s);check(enemies[0].hp==60,"No instant melee damage at windup");
    game.advance(.1f);game.combat(world,s,.1f);check(enemies[0].hp==60,"Windup is harmless");
    game.advance(.1f);game.combat(world,s,.1f);check(enemies[0].hp==35,"Blade deals damage in active phase");
    game.combat(world,s,.1f);check(enemies[0].hp==35,"One hit per swing");
    game.advance(1);s.inventory.weapons.insert("bow");s.inventory.weaponId="bow";s.inventory.ammo=1;
    game.attack(world,s);game.advance(.25f);game.combat(world,s,.01f);
    check(s.inventory.ammo==0&&!game.projectiles.empty(),"Bow fires projectile and consumes crafted ammo");
    for(int i=0;i<30;i++) game.combat(world,s,.02f);check(enemies[0].hp<35,"Arrow collision deals damage");
    s.player={1100,400};s.health=100;game.advance(2);
    enemies[0]={1000,{1124,400},{1124,400},{1124,400}};enemies[0].kind=1;enemies[0].city=0;enemies[0].hp=100;
    for(int i=0;i<8;i++) game.tick(world,s,.1f,i*.1);
    check(s.health==100,"Allied guards cannot attack player");
    diplomacy(s,0,2);
    for(int i=0;i<8;i++) game.tick(world,s,.1f,1+i*.1);
    check(s.health==82,"War makes guards hostile with telegraphed attacks");
    diplomacy(s,0,1);game.advance(2);int health=s.health;
    for(int i=0;i<12;i++) game.tick(world,s,.1f,2+i*.1);
    check(s.health==health,"Truce stops guard combat");
    s.relations[0].treaty=Treaty::Union;
    s.submerged=true;s.player={1100,-1100};s.oxygen=200;
    int coins=s.coins,treasury=s.treasury[0];
    for(int i=0;i<200;i++) game.tick(world,s,.1f,4+i*.1);
    check(s.coins==coins+1&&s.treasury[0]==treasury-1,"Union income continues underwater and conserves money");
    auto path=std::filesystem::temp_directory_path()/("frontier-adventure-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
    s.submerged=false;writeSave(path,s);auto restored=loadSave(path);
    check(restored.technologies==s.technologies&&restored.relations[0].treaty==Treaty::Union&&restored.explored==s.explored&&restored.inventory.submarine&&restored.inventory.weapons==s.inventory.weapons,"Advanced state persists");
    std::filesystem::remove(path);
    std::cout<<"Content rarity, diplomacy, research, walls, vessels, fog, conversation, timed combat and persistence passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
