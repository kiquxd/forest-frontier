#include "gameplay.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>

using namespace frontier;
static void check(bool condition,const char* reason) {if(!condition) throw std::runtime_error(reason);}
static void flat(World& world) {
  world.chunks.clear();
  for(int y=-2;y<=3;y++) for(int x=-2;x<=3;x++) {
    Chunk c;c.coord={x,y};c.tiles.resize(256,Tile{0,false,Terrain::Forest});world.chunks.emplace(c.coord,std::move(c));
  }
  world.buildings.clear();
}
int main() {
  try {
    Save s;
    auto inventory=s.inventory;
    check(!craft(s,2).ok&&s.inventory.wood==inventory.wood,"Failed crafting cannot consume resources");
    s.inventory.wood=60;s.inventory.stone=30;s.inventory.reeds=20;s.inventory.berries=12;s.inventory.leather=8;
    for(int i=0;i<8;i++) check(craft(s,i).ok,"All recipes craft");
    int wood=s.inventory.wood;check(craft(s,2).ok&&s.inventory.wood==wood-6&&s.weaponCopies["wood_sword"]==2,"Spare weapons consume materials and create separate copies");
    check(s.inventory.sword&&s.inventory.boat&&s.inventory.armor&&s.inventory.potions==1&&s.inventory.campfires==1,"Equipment and consumables");
    int totalCoins=s.coins+s.treasury[0],totalWood=s.inventory.wood+s.stocks[0][0];
    check(trade(s,0,0,true).ok&&trade(s,0,0,false).ok,"Buy and sell");
    check(totalCoins==s.coins+s.treasury[0]&&totalWood==s.inventory.wood+s.stocks[0][0],"Trade conserves money and goods");
    s.coins=0;int stock=s.stocks[0][1];check(!trade(s,0,1,true).ok&&stock==s.stocks[0][1],"Insufficient funds is atomic");
    s.treasury[0]=0;check(!trade(s,0,0,false).ok,"Treasury limits selling");
    s.coins=20;s.stocks[0][1]=0;check(!trade(s,0,1,true).ok,"Finite merchant stock");
    s.health=100;check(!consume(s,true).ok&&s.inventory.potions==1,"Full health cannot waste potion");
    s.health=40;check(consume(s,true).ok&&s.health==85&&s.inventory.potions==0,"Potion healing");
    s.inventory.berries=3;check(consume(s,false).ok&&s.health==90&&s.inventory.berries==2,"Eating");
    Resource fish{12345,{0,0},ResourceKind::Fish,0};s.inventory.rod=false;
    check(!harvest(s,fish).ok&&!s.harvested.contains(fish.id),"Fishing requires rod");
    s.inventory.rod=true;check(harvest(s,fish).ok&&s.inventory.fish==1,"Fishing adds fish");
    check(!harvest(s,fish).ok&&s.inventory.fish==1,"No repeated harvest");
    check(terrainAt({1100,-1100},s.seed)==Terrain::Deep,"Lake center");
    check(terrainAt({0,0},s.seed)==Terrain::Forest,"Village remains dry");
    bool sand=false,shallows=false;
    for(int y=-2000;y<0;y+=24) {auto t=terrainAt({1100,float(y)},s.seed);sand|=t==Terrain::Sand;shallows|=t==Terrain::Shallow;}
    check(sand&&shallows,"Lake has sand and shallows");
    World world(s.seed);flat(world);
    world.chunks.at({0,0}).resources.push_back({777,{500,400},ResourceKind::Tree,0});
    Vec p=world.move({450,400},{200,0},s);
    check(p.x<480,"Substeps prevent tunneling through tree");
    auto path=world.path({450,400},{650,400},s);check(!path.empty(),"Navigation finds path around tree");
    p={450,400};for(auto waypoint:path) p=world.move(p,waypoint-p,s);
    check(length(p-Vec{650,400})<25,"Navigation path is physically traversable");
    s.harvested.insert(777);p=world.move({450,400},{200,0},s);check(p.x>649,"Harvest removes collision");
    world.buildings.push_back({{200,200},0});check(!world.clear({200,200},s),"Building collision");
    world.buildings.clear();world.chunks.at({0,0}).tiles[8*16+8].terrain=Terrain::Deep;
    check(!world.clear({408,408},s,12,false)&&world.clear({408,408},s,12,true),"Deep water requires boat");
    p=world.move({408,340},{0,140},s);check(p.y<=373,"Cannot tunnel into deep water");
    p=world.move({408,340},{0,140},s,12,true);check(p.y>479,"Boat crosses deep water");
    Gameplay game;s.player={560,400};s.defeated.clear();s.inventory.leather=0;s.coins=20;
    world.chunks.at({0,0}).enemies.push_back({999,{620,400},{620,400},{620,400}});
    for(int i=0;i<3;i++) {game.advance(1);check(game.attack(world,s).ok,"Sword attack");game.advance(.18f);game.combat(world,s,.18f);}
    check(s.defeated.contains(999)&&s.inventory.leather==2&&s.coins==23,"Three sword hits defeat wolf with one loot drop");
    game.advance(1);game.attack(world,s);game.advance(.18f);game.combat(world,s,.18f);check(s.inventory.leather==2,"Defeated wolf cannot drop twice");
    auto& e=world.chunks.at({0,0}).enemies[0];s.defeated.clear();s.player={1100,400};
    e={999,{1124,400},{1124,400},{1124,400}};s.health=100;s.inventory.armor=false;game.advance(2);
    game.tick(world,s,.1f,0);check(s.health==100&&e.windup>0,"Wolf telegraphs before biting");
    for(int i=0;i<5;i++) game.tick(world,s,.1f,.1);check(s.health==88,"Wolf bite");
    game.tick(world,s,.1f,.1);check(s.health==88,"Bite cooldown / invulnerability");
    e.cooldown=0;e.windup=0;s.health=1;s.coins=100;game.advance(2);for(int i=0;i<6;i++) game.tick(world,s,.1f,.2);
    check(s.health==100&&s.coins==80&&game.respawns==1,"Death respawns and preserves inventory");
    s.player={1100,400};e={999,{1124,400},{1124,400},{1124,400}};s.health=100;game.advance(4);s.inventory.armor=true;
    for(int i=0;i<6;i++) game.tick(world,s,.1f,.3);check(s.health==93,"Armor damage reduction");
    s.campfires.push_back(s.player);s.health=50;e.cooldown=0;game.advance(4);game.tick(world,s,1,.4);
    check(s.health==58,"Campfire heals and prevents attacks");
    auto temp=std::filesystem::temp_directory_path()/("frontier-gameplay-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp);auto savePath=temp/"world.json";
    s.defeated.insert(0xffffffffffffffffULL);writeSave(savePath,s);auto loaded=loadSave(savePath);
    check(loaded.defeated==s.defeated&&loaded.inventory.boat&&loaded.health==s.health&&loaded.stocks==s.stocks&&loaded.campfires.size()==1,"Version 2 save round trip");
    {std::ofstream out(savePath);out<<R"({"version":1,"seed":742019,"player":[120,140],"wood":9,"stone":5,"berries":2,"harvested":[18446744073709551615]})";}
    loaded=loadSave(savePath);check(loaded.inventory.wood==9&&loaded.harvested.contains(0xffffffffffffffffULL)&&loaded.health==100,"Old save migration");
    writeSave(savePath,loaded);check(std::filesystem::exists(savePath.string()+".v1.bak"),"Migration keeps original backup");
    std::filesystem::remove_all(temp);
    std::cout<<"Craft, trade, harvest, terrain, collision, navigation, combat, healing and migration checks passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
