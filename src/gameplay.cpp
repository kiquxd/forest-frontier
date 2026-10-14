#include "gameplay.hpp"
#include "adventure.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace frontier {
const char* goodName(int good) {
  static const char* names[]={"Wood","Stone","Berries","Reeds","Fish","Leather","Iron","Copper","Crystal","Pearl","Steel","Core","Relic"};
  return good>=0&&good<goodTypes ? names[good]:"";
}
int& goodCount(Inventory& inv,int good) {
  switch(good) {case 0:return inv.wood;case 1:return inv.stone;case 2:return inv.berries;
    case 3:return inv.reeds;case 4:return inv.fish;case 5:return inv.leather;}
  if(good>=6&&good<goodTypes) return inv.rare[good-6];
  throw std::out_of_range("Invalid good");
}
int goodCount(const Inventory& inv,int good) {auto copy=inv;return goodCount(copy,good);}
const std::vector<Recipe>& recipes() {return content().recipes;}
const char* technologyName(const std::string& id) {
  if(id=="ironworking") return "Ironworking";
  if(id=="steelworking") return "Steel alloys";
  if(id=="shipbuilding") return "Shipbuilding";
  if(id=="submarine") return "Pressure hulls";
  if(id=="crystallurgy") return "Crystal optics";
  if(id=="diving") return "Deep-sea harvesting";
  if(id=="ancient") return "Ancient machinery";
  if(id=="fortification") return "Stone fortifications";
  for(auto& node:researchNodes()) if(node.id==id) return node.name.c_str();
  if(id=="civic_charter") return "Civic charter";
  return "";
}
static bool knows(const Save& s,const std::string& tech) {return tech.empty()||s.technologies.contains(tech);}
void ensureCityState(Save& s) {
  auto total=settlements().size();
  if(s.campaign&&(s.owners.size()!=total||s.cityOpinions.size()!=total)) throw std::runtime_error("Campaign settlement count does not match seed");
  s.stocks.resize(total);s.treasury.resize(total,150);
  while(s.relations.size()<total) {
    size_t id=s.relations.size();auto& city=settlements()[id];
    s.relations.push_back({city.attitude,city.attitude<=-50 ? Treaty::War:Treaty::Peace,false});
    if(id>=2) s.stocks[id]={40,30,25,20,12,8};
    for(int good:content().towns[city.type].goods) s.stocks[id][good]=16;
  }
}
bool ownsCity(const Save& s,int city) {return city>=0&&city<int(s.owners.size())&&s.owners[city]==0;}
const std::vector<Skill>& skills() {
  static const std::vector<Skill> list={
    {"gathering","Gathering","First lesson at any town hall"},
    {"trading","Trading","Learn gathering"},
    {"crafting","Craftsmanship","Gather from 5 resource sites"},
    {"tools","Tools","Craft 2 items"},
    {"combat","Weaponsmithing","Craft 2 items"},
    {"fishing","Fishing","Gather from 10 resource sites"},
    {"sailing","Seamanship","Catch 3 fish"},
    {"medicine","Herbalism","Gather from 10 resource sites"},
    {"construction","Construction","Craft 4 items"},
    {"diplomacy","Diplomacy","Learn trading"},
    {"engineering","Engineering","Complete a town order"}};return list;
}
std::string skillName(const std::string& id) {for(auto& skill:skills()) if(skill.id==id) return skill.name;return id;}
bool hasSkill(const Save& s,const std::string& id) {return !s.campaign||id.empty()||s.skills.contains(id);}
void initializeCampaign(Save& s,bool legacy) {
  ensureCityState(s);if(s.campaign) return;
  const int n=int(settlements().size());s.homeCity=int(hash(s.seed^0x718acULL)%n);
  if(legacy) {float nearest=1e30f;for(int i=0;i<n;i++) {float d=length(s.player-settlements()[i].pos);if(d<nearest) {nearest=d;s.homeCity=i;}}}
  s.owners.resize(n);s.cityOpinions.assign(n,std::vector<int>(n));
  for(int i=0;i<n;i++) {s.owners[i]=i==s.homeCity ? 0:i+1;
    for(int j=0;j<=i;j++) {
      const auto& a=settlements()[i];const auto& b=settlements()[j];
      int attitude=i==j ? 100:std::clamp((a.attitude+b.attitude)/2+(a.type==b.type ? 25:-5)+
        (length(a.pos-b.pos)<4500 ? 8:-8)+int(hash(s.seed^uint64_t(i*1009+j))%31)-15,-90,90);
      s.cityOpinions[i][j]=s.cityOpinions[j][i]=attitude;
    }
  }
  if(legacy) {for(auto& skill:skills()) s.skills.insert(skill.id);for(int i=0;i<n;i++) if(s.relations[i].treaty==Treaty::Union) s.owners[i]=0;}
  else {s.player=settlements()[s.homeCity].pos+Vec{110,110};s.coins=0;
    for(int i=0;i<n;i++) {int opinion=s.cityOpinions[s.homeCity][i];s.relations[i]={opinion,opinion<=-35 ? Treaty::War:Treaty::Peace,false};}}
  s.relations[s.homeCity]={100,Treaty::Peace,true};s.campaign=true;
}
LessonState lessonState(const Save& s,int index) {
  const auto& skill=skills().at(index);
  int progress=1,target=1;
  if(index==1) progress=hasSkill(s,"gathering");
  else if(index==9) progress=hasSkill(s,"trading");
  else if(index==2||index==5||index==7) {progress=s.gathered;target=index==2 ? 5:10;}
  else if(index==3||index==4||index==8) {progress=s.crafted;target=index==8 ? 4:2;}
  else if(index==6) {progress=s.caught;target=3;}
  else if(index==10) progress=s.orders;
  bool learned=s.skills.contains(skill.id),ready=progress>=target;
  std::string detail=learned ? "Learned · available now":index==0 ? "Free introductory lesson · ready to learn":
    skill.requirement+" · "+std::to_string(std::min(progress,target))+" / "+std::to_string(target)+(ready ? " · ready to learn":" · practice required");
  return {learned,ready,learned ? target:std::min(progress,target),target,detail};
}
Action trainSkill(Save& s,int index) {
  if(index<0||index>=int(skills().size())) return {false,"Unknown skill"};
  auto& skill=skills()[index];auto state=lessonState(s,index);
  if(state.learned) return {false,"Skill already learned"};
  if(!state.ready) return {false,state.detail};
  s.skills.insert(skill.id);return {true,"Learned: "+skill.name};
}
std::string recipeSkill(const Recipe& r) {
  const auto& a=r.result;
  if(a.starts_with("outfit:")) return "crafting";
  if(a=="axe"||a=="pickaxe") return "tools";
  if(a.starts_with("weapon:")||a=="ammo"||a=="armor") return "combat";
  if(a=="rod") return "fishing";
  if(a=="boat"||a=="ship") return "sailing";
  if(a=="potion") return "medicine";
  if(a=="submarine"||a=="sonar"||a=="oxygen"||a=="steel") return "engineering";
  return "crafting";
}
const char* treatyName(Treaty treaty) {
  static const char* names[]={"peace","war","truce","alliance","union"};return names[int(treaty)];
}
const WeaponDef& currentWeapon(const Save& s) {
  static const WeaponDef fists={"","Fists","fists","",10,0,1,90,.55f,{}};
  if(!s.inventory.weaponId.empty()) return weapon(s.inventory.weaponId);
  if(s.inventory.sword&&s.inventory.weapons.empty()) return weapon("wood_sword");
  return fists;
}
std::vector<std::string> ownedWeapons(const Save& s) {
  std::vector<std::string> out{""};for(auto& w:content().weapons) if(s.inventory.weapons.contains(w.id)) out.push_back(w.id);return out;
}
Action equipWeapon(Save& s,const std::string& id) {
  if(!id.empty()&&!s.inventory.weapons.contains(id)) return {false,"You do not own this weapon"};
  if(s.adventure&&availableWeapon(s,id)<1) return {false,"All copies are assigned to residents"};
  s.inventory.weaponId=id;return {true,"Equipped: "+currentWeapon(s).name};
}
bool equipped(const Inventory& inv,int recipe) {
  auto& result=recipes().at(recipe).result;
  if(result.starts_with("weapon:")||result.starts_with("outfit:")) return false;
  if(result=="axe") return inv.axe;
  if(result=="pickaxe") return inv.pickaxe;
  if(result=="rod") return inv.rod;
  if(result=="boat") return inv.boat;
  if(result=="armor") return false;
  if(result=="ship") return inv.ship;
  if(result=="submarine") return inv.submarine;
  if(result=="sonar") return inv.sonar;
  if(result=="oxygen") return inv.oxygen;
  return false;
}
Action craft(Save& s,int recipe) {
  if(recipe<0||recipe>=int(recipes().size())) return {false,"Unknown recipe"};
  auto& r=recipes()[recipe];
  if(!hasSkill(s,"crafting")||!hasSkill(s,recipeSkill(r))) return {false,"Skill locked. Learn at a town hall: "+skillName(!hasSkill(s,"crafting") ? "crafting":recipeSkill(r))};
  if(!recipeKnown(s,r)) return {false,"Complete an NPC quest to learn this recipe"};
  if(!knows(s,r.technology)) return {false,"Town technology required: "+std::string(technologyName(r.technology))};
  if(equipped(s.inventory,recipe)) return {false,"Already crafted"};
  for(int i=0;i<goodTypes;i++) if(goodCount(s.inventory,i)<r.cost[i]) return {false,"Not enough: "+std::string(goodName(i))};
  if((r.result=="potion"&&s.inventory.potions>=1000000)||(r.result=="campfire"&&s.inventory.campfires>=1000)||
    (r.result=="ammo"&&s.inventory.ammo>999990)||(r.result=="repair"&&s.inventory.repairs>=1000000)||
    (r.result=="steel"&&goodCount(s.inventory,10)>999997)) return {false,"Inventory full"};
  for(int i=0;i<goodTypes;
  i++) goodCount(s.inventory,i)-=r.cost[i];
  auto& result=r.result;
  if(result.starts_with("weapon:")) {s.inventory.weaponId=result.substr(7);s.inventory.weapons.insert(s.inventory.weaponId);s.weaponCopies[s.inventory.weaponId]++;s.inventory.sword=true;}
  else if(result.starts_with("outfit:")) s.outfitCopies[result.substr(7)]++;
  else if(result=="axe") s.inventory.axe=true;else if(result=="pickaxe") s.inventory.pickaxe=true;
  else if(result=="rod") s.inventory.rod=true;else if(result=="boat") {s.inventory.boat=true;s.vehicle=1;}
  else if(result=="armor") {s.inventory.armor=true;s.outfitCopies["leather_armor"]++;s.outfitId="leather_armor";}else if(result=="potion") s.inventory.potions++;
  else if(result=="campfire") s.inventory.campfires++;else if(result=="ammo") s.inventory.ammo+=10;
  else if(result=="ship") {s.inventory.ship=true;s.vehicle=2;}else if(result=="submarine") {s.inventory.submarine=true;s.vehicle=3;}
  else if(result=="steel") goodCount(s.inventory,10)+=3;else if(result=="sonar") s.inventory.sonar=true;
  else if(result=="oxygen") {s.inventory.oxygen=true;s.oxygen=200;}else if(result=="repair") s.inventory.repairs++;
  s.crafted++;return {true,"Crafted: "+r.name};
}
int buyPrice(int city,int good) {
  constexpr int base[goodTypes]={2,4,3,3,7,9,12,14,24,20,26,65,90};
  if(city<0||city>=int(settlements().size())||good<0||good>=goodTypes) return 0;
  if(city==1&&good<6) {constexpr int prices[]={3,3,2,2,6,10};return prices[good];}
  return base[good]+(city>=2 ? city%3:0);
}
int sellPrice(int city,int good) {return std::max(1,buyPrice(city,good)-std::max(2,buyPrice(city,good)/3));}
static std::string materialTech(int good) {
  switch(good) {case 6:return "ironworking";case 7:return "shipbuilding";case 8:return "crystallurgy";
    case 9:return "diving";case 10:return "steelworking";case 11:return "submarine";case 12:return "ancient";default:return "";}
}
Action trade(Save& s,int city,int good,bool buy) {
  if(!hasSkill(s,"trading")) return {false,"Learn trading at a town hall"};
  ensureCityState(s);
  if(city<0||city>=int(settlements().size())||good<0||good>=goodTypes) return {false,"Merchant unavailable"};
  auto relation=s.relations[city];
  if(relation.treaty==Treaty::War||relation.opinion<-20) return {false,"Town refuses to trade: negotiate a truce"};
  if(good>=6&&buy) {
    auto& type=content().towns[settlements()[city].type];
    if(std::find(type.goods.begin(),type.goods.end(),good)==type.goods.end()) return {false,"This town does not produce that material"};
    if(relation.opinion<20||!knows(s,materialTech(good))) return {false,"Trust and town technology required — P: diplomacy"};
  }
  int& count=goodCount(s.inventory,good);int& stock=s.stocks[city][good];int& money=s.treasury[city];
  int price=buy ? buyPrice(city,good):std::max(1,int(std::lround(sellPrice(city,good)*playerCharacter(s).trade*playerOutfit(s).trade)));
  if(buy) {
    if(stock<=0) return {false,"Out of stock"};
    if(s.coins<price) return {false,"Not enough coins"};
    if(count>=1000000||money>10000000-price) return {false,"Trade limit reached"};
    s.coins-=price;money+=price;stock--;count++;
  } else {
    if(count<=0) return {false,"Nothing to sell"};
    if(money<price) return {false,"Town treasury empty"};
    if(stock>=1000000||s.coins>10000000-price) return {false,"Trade limit reached"};
    s.coins+=price;money-=price;stock++;count--;
  }
  return {true,std::string(buy ? "Bought: ":"Sold: ")+goodName(good)};
}
const std::vector<std::string>& cityActions() {
  static const std::vector<std::string> actions=[] {std::vector<std::string> list={"Gift to town · 20 coins · +15 trust","Negotiate a truce · 25 coins", "Declare war · local conflict placeholder",
    "Propose alliance · trust 30 · 10 coins","Unite · alliance, trust 70 · 40 coins",
    "Learn town technology · trust 20 · 20 coins","Build wall segment · wood 8, stone 3",
    "Buy rare town weapon · 80 coins","Town order · deliver 5 wood and 3 fish"};
    list.push_back("Audience with ruler / your town");
    for(auto& skill:skills()) list.push_back("Lesson: "+skill.name+" · "+skill.requirement);
    list.push_back("Relations with other towns");return list;}();return actions;
}
Action learnTechnology(Save& s,int city) {
  ensureCityState(s);if(city<0||city>=int(settlements().size())) return {false,"Town unavailable"};
  auto& r=s.relations[city];auto& type=content().towns[settlements()[city].type];
  if(r.treaty==Treaty::War||r.opinion<20) return {false,"Masters require peace and trust 20"};
  if(s.technologies.contains(type.technology)) return {false,"Technology already learned"};
  if(!hasSkill(s,"crafting")) return {false,"Learn craftsmanship at a town hall first"};
  if(s.coins<20) return {false,"Research costs 20 coins"};
  s.coins-=20;s.treasury[city]+=20;s.technologies.insert(type.technology);return {true,"Learned: "+std::string(technologyName(type.technology))};
}
const WeaponDef& cityWeapon(int city,uint64_t seed) {
  int minimum=std::max(1,settlements().at(city).rarity),total=0;
  for(auto& item:content().weapons) if(item.rarity>=minimum) total+=item.weight;
  int roll=int(hash(seed^uint64_t(city)*0x9812)%uint64_t(total));
  for(auto& item:content().weapons) if(item.rarity>=minimum) {roll-=item.weight;if(roll<0) return item;}
  return content().weapons.back();
}
Action diplomacy(Save& s,int city,int action) {
  ensureCityState(s);if(city<0||city>=int(settlements().size())) return {false,"Town unavailable"};
  auto& r=s.relations[city];
  struct SyncOpinion {Save& save;int city;~SyncOpinion() {if(save.campaign&&save.homeCity>=0&&!ownsCity(save,city)) save.cityOpinions[city][save.homeCity]=save.cityOpinions[save.homeCity][city]=save.relations[city].opinion;}} sync{s,city};
  if(ownsCity(s,city)&&action<5) return {false,"This is your town. No treaty needed"};
  if(action>=10&&action<10+int(skills().size())) {if(r.treaty==Treaty::War) return {false,"Lessons unavailable during war"};return trainSkill(s,action-10);}
  if(action==9) return {false,ownsCity(s,city) ? "You rule this town. The town hall is vacant.":"The ruler receives visitors in the town hall"};
  if(action==10+int(skills().size())) {std::string out="Visitors: ";if(!s.campaign) return {false,"No data"};for(int i=0;i<int(settlements().size());i++) if(i!=city&&s.relations[i].discovered) out+=settlements()[i].name+" "+std::to_string(s.cityOpinions[city][i])+"; ";return {true,out};}
  if(action<5&&!hasSkill(s,"diplomacy")) return {false,"Learn diplomacy at a town hall"};
  if(action==6&&!hasSkill(s,"construction")) return {false,"Learn construction at a town hall"};
  if(action==6&&s.campaign&&!ownsCity(s,city)&&r.treaty!=Treaty::Alliance&&r.treaty!=Treaty::Union) return {false,"Build walls in your own or an allied town"};
  if(action==5) return learnTechnology(s,city);
  if(action==2) {r.treaty=Treaty::War;r.opinion=-80;return {true,"War declared. Guards are hostile; strategic warfare remains a placeholder."};}
  if(action==1) {
    if(s.coins<25) return {false,"A truce costs 25 coins"};
    s.coins-=25;s.treasury[city]+=25;
    r.treaty=Treaty::Truce;r.opinion=std::max(0,r.opinion);return {true,"Truce agreed. Guards stop attacking."};
  }
  if(r.treaty==Treaty::War) return {false,"Negotiate a truce first"};
  if(action==0) {
    if(s.coins<20) return {false,"You need 20 coins"};
    s.coins-=20;s.treasury[city]+=20;r.opinion=std::min(100,r.opinion+15);return {true,"Town accepted the gift: +15 trust"};
  }
  if(action==3) {
    if(r.opinion<30||s.coins<10) return {false,"Alliance requires trust 30 and 10 coins"};
    if(r.treaty==Treaty::Alliance||r.treaty==Treaty::Union) return {false,"Already allied"};
    s.coins-=10;s.treasury[city]+=10;r.treaty=Treaty::Alliance;r.opinion=std::max(40,r.opinion);return {true,"Alliance formed. Complete orders to build trust."};
  }
  if(action==4) {
    if(r.treaty!=Treaty::Alliance||r.opinion<70||s.coins<40) return {false,"Requires alliance, trust 70 and 40 coins"};
    s.coins-=40;s.treasury[city]+=40;r.treaty=Treaty::Union;r.opinion=95;if(s.campaign) {s.owners[city]=0;syncCitizens(s);}return {true,"Town joined your union: treasury income every 20 seconds."};
  }
  if(action==6) {
    if(r.opinion<0) return {false,"Town refuses construction"};
    int built=0;for(auto& w:s.walls) if(w.city==city) built++;
    bool damaged=false;for(auto& w:s.walls) if(w.city==city&&w.hp==0) damaged=true;
    if(built>=11&&!damaged) return {false,"Wall complete; gate left on the east"};
    if(s.inventory.wood<8||s.inventory.stone<3) return {false,"You need 8 wood and 3 stone"};
    float a=(built+1)*6.2831853f/12,b=(built+2)*6.2831853f/12;auto p=settlements()[city].pos;
    for(auto& wall:s.walls) if(wall.city==city&&wall.hp==0) {
      wall.hp=s.technologies.contains("fortification") ? 400:200;
      s.inventory.wood-=8;s.inventory.stone-=3;return {true,"Wall repaired"};
    }
    s.walls.push_back({city,p+Vec{std::cos(a)*355,std::sin(a)*355},p+Vec{std::cos(b)*355,std::sin(b)*355},s.technologies.contains("fortification") ? 400:200});
    s.inventory.wood-=8;s.inventory.stone-=3;r.opinion=std::min(100,r.opinion+2);return {true,"Wall segment built"};
  }
  if(action==7) {
    if(r.opinion<30||s.coins<80) return {false,"Requires trust 30 and 80 coins"};
    auto& item=cityWeapon(city,s.seed);
    if(s.inventory.weapons.contains(item.id)) return {false,"You already own this town's weapon"};
    s.coins-=80;s.treasury[city]+=80;s.inventory.weapons.insert(item.id);s.weaponCopies[item.id]++;s.inventory.weaponId=item.id;return {true,"Received: "+item.name+" ("+rarityName(item.rarity)+")"};
  }
  if(action==8) {
    if(s.inventory.wood<5||s.inventory.fish<3) return {false,"Order requires 5 wood and 3 fish"};
    if(s.treasury[city]<18) return {false,"Town cannot pay for the order yet"};
    s.inventory.wood-=5;s.inventory.fish-=3;s.stocks[city][0]+=5;s.stocks[city][4]+=3;
    s.treasury[city]-=18;s.coins+=18;s.orders++;r.opinion=std::min(100,r.opinion+12);return {true,"Order completed: +18 coins, +12 trust"};
  }
  return {false,"Unknown action"};
}
bool watercraft(const Save& s) {return s.inventory.boat||s.inventory.ship||s.inventory.submarine;}
int activeVehicle(const Save& s) {
  if(s.submerged) return 3;
  if(s.vehicle==3&&s.inventory.submarine) return 3;
  if(s.vehicle==2&&s.inventory.ship) return 2;
  if(s.vehicle==1&&s.inventory.boat) return 1;
  return s.inventory.submarine ? 3:s.inventory.ship ? 2:s.inventory.boat ? 1:0;
}
Action cycleVehicle(Save& s) {
  if(s.submerged) return {false,"Surface with R first"};
  for(int i=1;i<=3;i++) {int choice=(s.vehicle+i-1)%3+1;
    if((choice==1&&s.inventory.boat)||(choice==2&&s.inventory.ship)||(choice==3&&s.inventory.submarine)) {
      s.vehicle=choice;return {true,std::string("Selected: ")+(choice==1 ? "boat":choice==2 ? "ship":"submarine")};}}
  return {false,"Craft a vessel first"};
}
Action dive(Save& s) {
  if(!s.inventory.submarine||activeVehicle(s)!=3) return {false,"Craft a submarine and select it with V"};
  auto terrain=terrainAt(s.player,s.seed);
  if(terrain!=Terrain::Deep&&terrain!=Terrain::Shallow) return {false,"You can dive only on water"};
  if(!s.submerged&&s.oxygen<15) return {false,"Wait on the surface for air to refill"};
  if(!s.submerged&&s.hull<=15) return {false,"Repair the hull with J first"};
  s.submerged=!s.submerged;return {true,s.submerged ? "Diving. R: surface, G: sonar, F: harvest":"Submarine surfaced"};
}
Action repairVessel(Save& s) {
  if(s.hull>=100) return {false,"Hull intact"};
  if(s.inventory.repairs<1) return {false,"Craft a repair kit"};
  s.inventory.repairs--;s.hull=std::min(100,s.hull+45);return {true,"Hull repaired by 45"};
}
Action sonar(Save& s) {
  if(!s.submerged||!s.inventory.sonar) return {false,"Dive and craft a sonar first"};
  for(int y=-18;y<=18;y++) for(int x=-18;x<=18;x++) if(x*x+y*y<=18*18) {
    auto center=mapCell(s.player);Coord cell{center.x+x,center.y+y};Vec p{(cell.x+.5f)*mapCellSize,(cell.y+.5f)*mapCellSize};
    auto t=terrainAt(p,s.seed);if(t==Terrain::Deep||t==Terrain::Shallow) s.exploredUnderwater.insert(cell);
  }return {true,"Sonar revealed the seafloor"};
}
int resourceGood(ResourceKind kind) {
  if(int(kind)<=4) return int(kind);
  if(kind==ResourceKind::Ore) return 6;
  if(kind==ResourceKind::Crystal) return 8;
  if(kind==ResourceKind::Pearl) return 9;
  return 12;
}
Action consume(Save& s,bool potion) {
  if(s.health>=100) return {false,"Health already full"};
  if(potion) {
    if(s.inventory.potions<=0) return {false,"No healing potions — C: crafting"};
    s.inventory.potions--;s.health=std::min(100,s.health+45);return {true,"Potion: +45 health"};
  }
  if(s.inventory.fish>0) {s.inventory.fish--;s.health=std::min(100,s.health+12);return {true,"Fish: +12 health"};}
  if(s.inventory.berries>0) {s.inventory.berries--;s.health=std::min(100,s.health+5);return {true,"Berries: +5 health"};}
  return {false,"No food"};
}
float harvestDuration(const Resource& r,const Inventory& inv) {
  if(r.kind==ResourceKind::Fish) return 1.8f;
  if(r.kind==ResourceKind::Tree&&inv.axe) return .3f;
  if(r.kind==ResourceKind::Stone&&inv.pickaxe) return .35f;
  return .7f;
}
Action harvest(Save& s,const Resource& r) {
  if(!hasSkill(s,"gathering")) return {false,"First gathering lesson: P at a town hall entrance"};
  if(s.harvested.contains(r.id)) return {false,"Already gathered"};
  if(r.kind==ResourceKind::Fish&&!s.inventory.rod) return {false,"Fishing requires a rod — C: crafting"};
  if((r.kind==ResourceKind::Crystal||r.kind==ResourceKind::Pearl||r.kind==ResourceKind::Salvage)&&!s.submerged)
    return {false,"Dive in a submarine to harvest this resource"};
  int good=resourceGood(r.kind);
  if(good>=6&&!knows(s,materialTech(good))) return {false,"Town technology required: "+std::string(technologyName(materialTech(good)))};
  if(r.kind==ResourceKind::Ore&&!s.inventory.pickaxe) return {false,"Pickaxe required"};
  int amount=good==0 ? 3:good<=3 ? 2:1;
  if(goodCount(s.inventory,good)>1000000-amount) return {false,"Inventory full"};
  goodCount(s.inventory,good)+=amount;s.harvested.insert(r.id);s.gathered++;if(r.kind==ResourceKind::Fish) s.caught++;return {true,"+"+std::to_string(amount)+" "+goodName(good)};
}
void Gameplay::advance(float dt) {
  if(swing>0) {attackElapsed+=dt;swing=std::max(0.f,attackDuration-attackElapsed);}
  invulnerable=std::max(0.f,invulnerable-dt);attackCooldown=std::max(0.f,attackCooldown-dt);sonarCooldown=std::max(0.f,sonarCooldown-dt);
}
Vec Gameplay::movePlayer(World& world,Save& s,Vec displacement) {
  auto terrain=terrainAt(s.player,s.seed);bool afloat=terrain==Terrain::Shallow||terrain==Terrain::Deep;
  int vehicle=afloat ? activeVehicle(s):0;
  float factor=s.submerged ? 1.1f:vehicle==2 ? 1.65f:vehicle==3 ? 1.05f:vehicle==1 ? .85f:terrain==Terrain::Shallow ? .45f:1.f;
  if(swing>0&&vehicle==0) factor*=.45f;
  factor*=playerOutfit(s).speed*(vehicle ? 1.f:playerCharacter(s).speed);
  Vec previous=s.player;s.player=world.move(s.player,displacement*factor,s,12,watercraft(s));Vec actual=s.player-previous;
  moving=length(actual)>.001f;if(length(displacement)>.001f) facing=normalized(displacement);
  if(moving) {facing=normalized(actual);if(!vehicle&&!s.submerged) walk=std::fmod(walk+length(actual)/20,4.f);}
  if(length(actual)+.01f<length(displacement*factor)) collisions++;
  return actual;
}
void Gameplay::explore(Save& s,float width,float height,float zoom) {
  auto& cells=s.submerged ? s.exploredUnderwater:s.explored;
  auto half=unproject({width/(2*zoom),height/(2*zoom)});int radius=int(std::ceil((std::abs(half.x)+std::abs(half.y))/mapCellSize))+1;
  radius=std::min(radius,35);auto origin=mapCell(s.player);
  for(int y=-radius;y<=radius;y++) for(int x=-radius;x<=radius;x++) {
    Coord cell{origin.x+x,origin.y+y};Vec pos{(cell.x+.5f)*mapCellSize,(cell.y+.5f)*mapCellSize};auto projected=project(pos-s.player);
    if(std::abs(projected.x)>width/(2*zoom)||std::abs(projected.y)>height/(2*zoom)) continue;
    if(s.submerged) {
      if(length(pos-s.player)>650) continue;
      auto terrain=terrainAt(pos,s.seed);
      if(terrain!=Terrain::Deep&&terrain!=Terrain::Shallow) continue;
    }
    cells.insert(cell);
  }
  ensureCityState(s);
  for(size_t i=0;i<settlements().size();
  i++) if(exploredAt(s.explored,settlements()[i].pos)) s.relations[i].discovered=true;
}
bool Gameplay::repairSpawn(const World& world,Save& s) const {
  if(!world.chunks.contains(chunkAt(s.player))) return false;
  if(world.clear(s.player,s,12,watercraft(s))) return true;
  Vec origin=s.player;
  for(int r=1;r<32;r++) for(int y=-r;y<=r;y++) for(int x=-r;x<=r;x++) {
    if(std::abs(x)!=r&&std::abs(y)!=r) continue;
    Vec p=origin+Vec{float(x)*12,float(y)*12};if(world.clear(p,s,12,watercraft(s))) {s.player=p;return true;}
  }
  for(auto& village:settlements()) {
    Vec safe=village.pos+Vec{120,140};if(world.clear(safe,s,12,watercraft(s))) {s.player=safe;return true;}
  }
  return false;
}
Action Gameplay::attack(World&,Save& s) {
  if(attackCooldown>0) return {false,""};
  auto terrain=terrainAt(s.player,s.seed);
  if(s.submerged||((terrain==Terrain::Deep||terrain==Terrain::Shallow)&&watercraft(s))) return {false,"Weapons unavailable aboard vessels"};
  attackWeapon_=currentWeapon(s);attackWeapon_.damage=std::max(1,int(std::lround(attackWeapon_.damage*playerCharacter(s).damage)));
  if(attackWeapon_.kind=="bow"&&s.inventory.ammo<1) return {false,"No arrows — C: crafting"};
  if(attackWeapon_.kind=="bow") s.inventory.ammo--;
  attackDuration=attackWeapon_.duration;attackElapsed=0;swing=attackDuration;attackCooldown=attackDuration;
  attackFacing=facing;hitResolved_=false;attacks++;return {true,""};
}
void Gameplay::damage(World& world,Save& s,Enemy& e,int amount,Vec direction) {
  if(s.defeated.contains(e.id)) return;
  e.hp-=amount;e.flash=.22f;e.windup=0;e.cooldown=std::max(e.cooldown,.5f);
  e.pos=world.move(e.pos,normalized(direction)*18,s,10);e.previous=e.pos;
  if(e.hp>0) {notice="Hit: "+std::to_string(amount);return;}
  if(!e.kind) s.wolfKills++;
  s.defeated.insert(e.id);s.inventory.leather=std::min(1000000,s.inventory.leather+(e.kind ? 0:2));
  s.coins=std::min(10000000,s.coins+(e.kind ? 8:3));kills++;notice=e.kind ? "Guard defeated: +8 coins":"Wolf defeated: +2 leather, +3 coins";
  // Persistent defeated IDs make loot deterministic and prevent repeated rolls on reload.
  if(hash(e.id^s.seed)%100<18) {
    auto& found=weightedWeapon(hash(e.id));s.inventory.weapons.insert(found.id);s.weaponCopies[found.id]++;
    notice+=" · Found: "+found.name+" ("+rarityName(found.rarity)+")";
  }
}
void Gameplay::combat(World& world,Save& s,float dt) {
  if(!hitResolved_&&attackElapsed>=attackDuration*.3f) {
    hitResolved_=true;
    if(attackWeapon_.kind=="bow") projectiles.push_back({s.player+attackFacing*22,attackFacing*650,attackWeapon_.reach/650,attackWeapon_.damage});
    else {
      Coord center=chunkAt(s.player);
      for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++) {
        auto it=world.chunks.find({center.x+x,center.y+y});if(it==world.chunks.end()) continue;
        for(auto& e:it->second.enemies) {
          if(s.defeated.contains(e.id)||(e.kind&&s.relations.at(e.city).treaty!=Treaty::War)) continue;
          Vec d=e.pos-s.player;float distance=length(d);
          float cone=attackWeapon_.kind=="spear" ? .78f:.1f;
          if(distance>attackWeapon_.reach||(distance>12&&(d.x*attackFacing.x+d.y*attackFacing.y)/distance<cone)) continue;
          bool blocked=false;for(int step=1;step<6;step++) if(!world.clear(s.player+d*(step/6.f),s,3,false)) blocked=true;
          if(!blocked) damage(world,s,e,attackWeapon_.damage,d);
        }
      }
    }
  }
  for(auto& arrow:projectiles) {
    arrow.life-=dt;int steps=std::max(1,int(std::ceil(length(arrow.velocity)*dt/8)));
    for(int i=0;i<steps&&arrow.life>0;i++) {
      arrow.pos=arrow.pos+arrow.velocity*(dt/steps);
      if(!world.clear(arrow.pos,s,2,false)) {arrow.life=0;break;}
      auto coord=chunkAt(arrow.pos);
      for(int y=-1;y<=1&&arrow.life>0;y++) for(int x=-1;x<=1&&arrow.life>0;x++) {
        auto it=world.chunks.find({coord.x+x,coord.y+y});if(it==world.chunks.end()) continue;
        for(auto& e:it->second.enemies) if(!s.defeated.contains(e.id)&&(!e.kind||s.relations.at(e.city).treaty==Treaty::War)&&length(e.pos-arrow.pos)<19) {
          damage(world,s,e,arrow.damage,arrow.velocity);arrow.life=0;break;
        }
      }
    }
  }
  std::erase_if(projectiles,[](auto& p){return p.life<=0;});
}
Action Gameplay::placeCampfire(World& world,Save& s) {
  if(s.inventory.campfires<=0) return {false,"Craft a campfire with C first"};
  if(s.campfires.size()>=64) return {false,"Campfire limit reached (64)"};
  Vec p=s.player+facing*45;
  if(terrainAt(p,s.seed)==Terrain::Shallow||terrainAt(p,s.seed)==Terrain::Deep||!world.clear(p,s,18)) return {false,"Campfires need a clear, dry spot"};
  for(auto fire:s.campfires) if(length(fire-p)<100) return {false,"There is already a campfire nearby"};
  s.inventory.campfires--;s.campfires.push_back(p);return {true,"Campfire placed — rest nearby to heal"};
}
void Gameplay::tick(World& world,Save& s,float dt,double time) {
  ensureCityState(s);tickResidents(s,dt);bool sanctuary=false;
  defenderClock_+=dt;
  if(defenderClock_>=1.5f) {
    defenderClock_=std::fmod(defenderClock_,1.5f);
    for(auto& npc:world.npcs) if(length(npc.pos-s.player)<1500) {
      auto defender=citizen(s,npc.id);if(!defender||defender->job!="guard"||!defender->trained||defender->weapon.empty()||!ownsCity(s,defender->city)) continue;
      bool struck=false;for(auto& [coord,chunk]:world.chunks) {for(auto& enemy:chunk.enemies) if(!enemy.kind&&!s.defeated.contains(enemy.id)&&length(enemy.pos-npc.pos)<180) {
        damage(world,s,enemy,std::max(1,weapon(defender->weapon).damage/2),normalized(enemy.pos-npc.pos));struck=true;break;
      }if(struck) break;}
    }
  }
  for(size_t i=0;i<settlements().size();
  i++) if(s.relations[i].treaty!=Treaty::War&&length(settlements()[i].pos-s.player)<370) sanctuary=true;
  for(auto fire:s.campfires) if(length(fire-s.player)<130) sanctuary=true;
  for(auto& [coord,chunk]:world.chunks) for(auto& e:chunk.enemies) {
    if(s.defeated.contains(e.id)) continue;
    e.previous=e.pos;e.flash=std::max(0.f,e.flash-dt);e.cooldown=std::max(0.f,e.cooldown-dt);
    if(length(e.home-s.player)>1300) continue;
    bool hostile=e.kind ? s.relations.at(e.city).treaty==Treaty::War:!sanctuary;
    auto terrain=terrainAt(s.player,s.seed);if(s.submerged||terrain==Terrain::Deep||(terrain==Terrain::Shallow&&watercraft(s))) hostile=false;
    float distance=length(e.pos-s.player);bool chase=hostile&&distance<360;
    Vec target=e.home+Vec{std::sin(float(time)*.4f+float(e.id%31))*45,std::cos(float(time)*.3f+float(e.id%17))*45};
    if(chase) target=s.player;
    bool avoidFire=false;if(!e.kind) for(auto fire:s.campfires) if(length(e.pos-fire)<180) {target=e.pos+normalized(e.pos-fire)*100;chase=false;avoidFire=true;break;}
    if(!chase&&!avoidFire&&length(e.pos-e.home)>70) target=e.home;
    if(e.windup>0) {
      e.windup-=dt;
      if(e.windup<=0) {
        e.cooldown=1.3f;
        e.pos=world.move(e.pos,normalized(e.attackTarget-e.pos)*std::min(24.f,length(e.attackTarget-e.pos)),s,10);
        if(chase&&length(e.pos-s.player)<38&&invulnerable<=0) {
          int amount=s.inventory.armor ? (e.kind ? 11:7):(e.kind ? 18:12);
          if(s.adventure) amount=std::max(1,int(std::lround((e.kind ? 18:12)*playerCharacter(s).taken*playerOutfit(s).taken)));
          s.health-=amount;damageTaken+=amount;invulnerable=.65f;
          notice=e.kind ? "A guard hit you!":"Wolf bite! Sprint with SHIFT to dodge.";
          s.player=world.move(s.player,normalized(s.player-e.pos)*18,s,12,watercraft(s));
        }
      }
    } else if(chase&&distance<58&&e.cooldown<=0) {e.windup=.4f;e.attackTarget=s.player;}
    else {
      Vec delta=target-e.pos;float travel=chase ? std::max(0.f,length(delta)-42):length(delta);
      if(!s.submerged) e.pos=world.move(e.pos,normalized(delta)*std::min(travel,dt*(chase ? (e.kind ? 135.f:125.f):36.f)),s,10);
      if(e.kind&&chase&&travel>5&&length(e.pos-e.previous)<.5f&&e.cooldown<=0) {
        for(auto& wall:s.walls) if(wall.hp>0) {
          Vec line=wall.b-wall.a,relative=e.pos-wall.a;float length2=line.x*line.x+line.y*line.y;
          float fraction=std::clamp((relative.x*line.x+relative.y*line.y)/std::max(1.f,length2),0.f,1.f);
          if(length(e.pos-(wall.a+line*fraction))<35) {wall.hp=std::max(0,wall.hp-18);e.cooldown=1;notice="Guards are attacking the wall!";break;}
        }
      }
    }
    e.walk=std::fmod(e.walk+length(e.pos-e.previous)/10,4.f);
  }
  if(s.health<=0) {
    int nearest=0;float best=1e9;
    for(size_t i=0;i<settlements().size();i++) if(s.relations[i].treaty!=Treaty::War&&length(settlements()[i].pos-s.player)<best) {best=length(settlements()[i].pos-s.player);nearest=int(i);}
    s.player=settlements()[nearest].pos+Vec{120,140};s.health=100;s.coins-=s.coins/5;s.submerged=false;
    invulnerable=3;respawns++;notice="You woke up in a friendly town. Lost 20% of coins.";return;
  }
  if(s.submerged) {
    s.oxygen=std::max(0.f,s.oxygen-dt*(s.outfitId=="diver" ? .75f:1.f));auto cell=mapCell(s.player);
    if(objectId(cell.x,cell.y,s.seed)%31==0) {
      pressureClock_+=dt;if(pressureClock_>=2) {s.hull=std::max(1,s.hull-3);pressureClock_=0;notice="Dangerous reefs: hull damaged. J: repair.";}
    } else pressureClock_=0;
    if(s.oxygen<=0||s.hull<=5) {s.submerged=false;s.oxygen=5;notice="Emergency ascent. Refill air and repair the hull.";}
  } else s.oxygen=std::min(s.inventory.oxygen ? 200.f:100.f,s.oxygen+dt*6);
  bool heal=false;int rate=3;
  for(size_t i=0;i<settlements().size();
  i++) if(s.relations[i].treaty!=Treaty::War&&length(settlements()[i].pos-s.player)<160) heal=true;
  for(auto fire:s.campfires) if(length(fire-s.player)<110&&!s.submerged) {heal=true;rate=8;}
  if(heal) {regeneration_+=dt;if(regeneration_>=1) {s.health=std::min(100,s.health+rate);regeneration_-=1;}}
  else regeneration_=0;
  civicClock_+=dt;
  if(civicClock_>=20) {
    civicClock_-=20;for(size_t i=0;i<std::min(s.relations.size(),s.treasury.size());i++) if(s.relations[i].treaty==Treaty::Union&&s.treasury[i]>0&&s.coins<10000000) {s.treasury[i]--;s.coins++;}
  }
}
}
