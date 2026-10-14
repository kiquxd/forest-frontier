#include "adventure.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace frontier {
const CharacterDef& playerCharacter(const Save& s) {return character(s.characterId);}
const OutfitDef& playerOutfit(const Save& s) {return outfit(s.outfitId);}
float gatherTimeMultiplier(const Save& s) {return playerCharacter(s).gather*playerOutfit(s).gather;}
Citizen* citizen(Save& s,int id) {for(auto& resident:s.citizens) if(resident.npc==id) return &resident;return nullptr;}
const Citizen* citizen(const Save& s,int id) {for(auto& resident:s.citizens) if(resident.npc==id) return &resident;return nullptr;}
void syncCitizens(Save& s) {
  if(!s.campaign) return;
  for(auto& n:makeNpcs()) if(ownsCity(s,n.settlement)&&!n.ruler&&n.originCity<0&&!citizen(s,n.id)) s.citizens.push_back({n.id,n.settlement,"idle","","",false});
}
void initializeAdventure(Save& s) {
  if(!s.adventure) {
    for(auto& id:s.inventory.weapons) s.weaponCopies[id]=1;
    if(s.inventory.sword&&s.inventory.weapons.empty()) {s.inventory.weapons.insert("wood_sword");s.weaponCopies["wood_sword"]=1;}
    s.outfitCopies["traveler"]=1;s.outfitId="traveler";
    if(s.inventory.armor) {s.outfitCopies["leather_armor"]=1;s.outfitId="leather_armor";}
    s.adventure=true;
  }
  auto npcs=makeNpcs();for(auto& [id,q]:s.quests) if(id<0||id>=int(npcs.size())) throw std::runtime_error("Quest giver no longer exists");
  for(auto& resident:s.citizens) if(resident.npc<0||resident.npc>=int(npcs.size())||npcs[resident.npc].settlement!=resident.city||npcs[resident.npc].ruler||npcs[resident.npc].originCity>=0) throw std::runtime_error("Invalid saved resident");
  syncCitizens(s);
}
int availableWeapon(const Save& s,const std::string& id,int excluded) {
  if(id.empty()) return 1000000;
  auto found=s.weaponCopies.find(id);int count=found==s.weaponCopies.end() ? (s.inventory.weapons.contains(id) ? 1:0):found->second;
  for(auto& resident:s.citizens) if(resident.npc!=excluded&&resident.weapon==id) count--;
  return count;
}
int availableOutfit(const Save& s,const std::string& id,int excluded) {
  if(id.empty()) return 1000000;
  auto found=s.outfitCopies.find(id);int count=found==s.outfitCopies.end() ? 0:found->second;
  for(auto& resident:s.citizens) if(resident.npc!=excluded&&resident.outfit==id) count--;
  return count;
}
Action equipOutfit(Save& s,const std::string& id) {
  if(availableOutfit(s,id)<1) return {false,"No free copy of that outfit"};
  s.outfitId=id;s.inventory.armor=playerOutfit(s).taken<1;return {true,"Wearing: "+playerOutfit(s).name};
}
std::vector<std::string> ownedOutfits(const Save& s) {std::vector<std::string> out;for(auto& item:content().outfits) if(s.outfitCopies.contains(item.id)&&s.outfitCopies.at(item.id)>0) out.push_back(item.id);return out;}
bool recipeKnown(const Save& s,const Recipe& r) {
  if(!s.adventure) return true;
  if(r.questOnly&&!s.knownRecipes.contains(r.id)) return false;
  if(!r.result.starts_with("outfit:")) return true;
  auto id=r.result.substr(7);const auto& item=outfit(id);return !item.questOnly||s.knownRecipes.contains(r.id);
}
const QuestDef* npcQuest(const Save& s,const Npc& n) {
  if(auto found=s.quests.find(n.id);found!=s.quests.end()) return &quest(found->second.id);
  const auto& role=content().npcs[n.type].id;
  bool teacher=role==content().towns[settlements()[n.settlement].type].teacher;
  if(!teacher&&!n.ruler&&hash(s.seed^uint64_t(n.id)*0x8391ULL)%100>=45) return nullptr;
  std::string questRole=n.ruler||role=="elder"||role=="admiral" ? "diplomat":role=="archivist"||role=="navigator" ? "scholar":role=="engineer" ? "carpenter":role;
  for(auto& q:content().quests) if(q.role==questRole) return &q;
  return &quest("supplies");
}
std::string npcIntroduction(const Save& s,const Npc& n) {
  std::string out="I'm "+n.name+", "+n.role+" of "+settlements()[n.settlement].name+". "+n.personality;
  if(n.originCity>=0) out+=" I came here from "+settlements()[n.originCity].name+".";
  if(ownsCity(s,n.settlement)&&!n.ruler&&n.originCity<0) out+=" This is our town. You can train and equip me for a job.";
  if(!npcQuest(s,n)) out+=" I have no task for you today, but you can still trade or meet our town master.";
  return out;
}
static int objectiveValue(const Save& s,const QuestDef& q) {
  if(q.objective=="deliver") return goodCount(s.inventory,q.good);
  if(q.objective=="craft") return s.crafted;
  if(q.objective=="kills") return s.wolfKills;
  if(q.objective=="explore") return int(s.explored.size());
  if(q.objective=="discover") {int count=0;for(auto& r:s.relations) if(r.discovered) count++;return count;}
  return 0;
}
int questProgress(const Save& s,const QuestProgress& p) {auto& q=quest(p.id);return std::max(0,objectiveValue(s,q)-(q.objective=="deliver" ? 0:p.baseline));}
std::string questStatus(const Save& s,const Npc& n) {
  auto q=npcQuest(s,n);if(!q) return "No quest available.";
  auto found=s.quests.find(n.id);
  std::string status=found==s.quests.end() ? "Available":found->second.completed ? "Completed":std::to_string(std::min(q->target,questProgress(s,found->second)))+" / "+std::to_string(q->target);
  std::string reward=std::to_string(q->coins)+" coins";
  if(!q->recipe.empty()) {for(auto& r:recipes()) if(r.id==q->recipe) reward+=" + recipe: "+r.name;}
  if(!q->knowledge.empty()) reward+=" + "+q->knowledge;
  if(!q->weapon.empty()) reward+=" + "+weapon(q->weapon).name;
  if(!q->outfit.empty()) reward+=" + "+outfit(q->outfit).name;
  for(int i=0;i<goodTypes;i++) if(q->rewards[i]) reward+=" + "+std::to_string(q->rewards[i])+" "+goodName(i);
  return q->title+" · "+status+"\n"+q->lore+"\nReward: "+reward;
}
Action acceptQuest(Save& s,const Npc& n) {
  auto q=npcQuest(s,n);if(!q) return {false,"This resident has no quest"};
  if(s.quests.contains(n.id)) return {false,"Quest already accepted or completed"};
  if(s.relations[n.settlement].treaty==Treaty::War) return {false,"Negotiate a truce before accepting tasks"};
  s.quests[n.id]={q->id,false,q->objective=="deliver" ? 0:objectiveValue(s,*q)};s.trackedQuest=n.id;return {true,"Quest accepted: "+q->title};
}
Action finishQuest(Save& s,const Npc& n) {
  auto found=s.quests.find(n.id);if(found==s.quests.end()||found->second.completed) return {false,"No active quest to turn in"};
  auto& p=found->second;auto& q=quest(p.id);
  if(s.relations[n.settlement].treaty==Treaty::War) return {false,"This town is at war with you"};
  if(questProgress(s,p)<q.target) return {false,"Quest requirements are not met"};
  if(q.objective=="deliver") goodCount(s.inventory,q.good)-=q.target;
  s.coins=std::min(10000000,s.coins+q.coins);for(int i=0;i<goodTypes;i++) goodCount(s.inventory,i)=std::min(1000000,goodCount(s.inventory,i)+q.rewards[i]);
  if(!q.recipe.empty()) s.knownRecipes.insert(q.recipe);
  if(!q.knowledge.empty()) s.technologies.insert(q.knowledge);
  if(!q.weapon.empty()) {s.inventory.weapons.insert(q.weapon);s.weaponCopies[q.weapon]++;}
  if(!q.outfit.empty()) s.outfitCopies[q.outfit]++;
  p.completed=true;if(s.trackedQuest==n.id) s.trackedQuest=-1;s.relations[n.settlement].opinion=std::min(100,s.relations[n.settlement].opinion+5);
  if(s.campaign&&!ownsCity(s,n.settlement)) s.cityOpinions[s.homeCity][n.settlement]=s.cityOpinions[n.settlement][s.homeCity]=s.relations[n.settlement].opinion;
  return {true,"Quest completed: "+q.title+" · rewards received"};
}
int averageTrust(const Save& s) {
  int sum=0,count=0;for(int i=0;i<int(s.relations.size());i++) if(s.relations[i].discovered&&!ownsCity(s,i)) {sum+=s.relations[i].opinion;count++;}
  return count ? sum/count:0;
}
const std::vector<ProgressNode>& researchNodes() {
  static const std::vector<ProgressNode> nodes={
    {"apprenticeship","Apprenticeship","Train gatherers and craftspeople",15,4,3,1,0,1,0,1,"crafting"},
    {"militia","Militia training","Train equipped defenders; strategic war remains a placeholder",20,4,4,1,1,1,0,1,"combat"},
    {"commerce_school","Commerce school","Train traders and earn town treasury income",20,4,4,1,0,1,10,1,"trading"},
    {"navigation_school","Navigation school","Train fishers and improve town supplies",25,4,5,1,0,1,0,1,"sailing"},
    {"citizen_engineering","Engineering school","Train scholars and engineers",40,6,7,2,1,1,15,2,"engineering"},
    {"diplomatic_corps","Diplomatic corps","A foundation for multi-town leadership",50,8,7,2,2,2,30,2,"civic_charter","diplomacy"},
    {"guild_charter","Guild charter","Skilled residents produce larger work batches",35,4,6,2,1,1,20,2,"apprenticeship"}};return nodes;
}
static int countClothes(const Save& s) {int count=0;for(auto& [id,n]:s.outfitCopies) if(id!="traveler") count+=n;return count;}
static int countWeapons(const Save& s) {int count=0;for(auto& [id,n]:s.weaponCopies) count+=n;return count;}
static int completedQuests(const Save& s) {int count=0;for(auto& [id,q]:s.quests) if(q.completed) count++;return count;}
std::string researchRequirements(const Save& s,const ProgressNode& n) {
  std::ostringstream out;auto add=[&](const char* name,int have,int need){if(need) out<<name<<" "<<have<<"/"<<need<<" · ";};
  add("Coins",s.coins,n.coins);add("Residents",int(s.citizens.size()),n.residents);add("Skills",int(s.skills.size()),n.skills);
  add("Clothes",countClothes(s),n.clothes);add("Weapons",countWeapons(s),n.weapons);add("Towns",int(std::count(s.owners.begin(),s.owners.end(),0)),n.cities);
  add("Avg. foreign trust",averageTrust(s),n.trust);add("Quests",completedQuests(s),n.quests);out<<"Needs "<<(std::string(technologyName(n.prerequisite)).empty() ? skillName(n.prerequisite):technologyName(n.prerequisite));if(!n.secondPrerequisite.empty()) out<<" + "<<skillName(n.secondPrerequisite);return out.str();
}
bool researchReady(const Save& s,const ProgressNode& n) {
  return !s.technologies.contains(n.id)&&s.coins>=n.coins&&int(s.citizens.size())>=n.residents&&int(s.skills.size())>=n.skills&&countClothes(s)>=n.clothes&&countWeapons(s)>=n.weapons&&
    int(std::count(s.owners.begin(),s.owners.end(),0))>=n.cities&&averageTrust(s)>=n.trust&&completedQuests(s)>=n.quests&&(s.skills.contains(n.prerequisite)||s.technologies.contains(n.prerequisite))&&
    (n.secondPrerequisite.empty()||s.skills.contains(n.secondPrerequisite)||s.technologies.contains(n.secondPrerequisite));
}
Action research(Save& s,int index) {
  if(index<0||index>=int(researchNodes().size())) return {false,"Unknown research"};
  auto& n=researchNodes()[index];if(s.technologies.contains(n.id)) return {false,"Already researched"};
  if(!researchReady(s,n))
    return {false,researchRequirements(s,n)};
  s.coins-=n.coins;s.technologies.insert(n.id);return {true,"Researched: "+n.name};
}
const std::vector<std::string>& residentJobs() {static const std::vector<std::string> jobs={"idle","gatherer","crafter","guard","trader","fisher","scholar","engineer","ambassador"};return jobs;}
Action assignCitizen(Save& s,int id,const std::string& job,const std::string& weaponId,const std::string& outfitId) {
  auto resident=citizen(s,id);if(!resident||!ownsCity(s,resident->city)) return {false,"This is not your resident"};
  if(std::find(residentJobs().begin(),residentJobs().end(),job)==residentJobs().end()) return {false,"Unknown job"};
  if(!weaponId.empty()) (void)weapon(weaponId);
  if(!outfitId.empty()) (void)outfit(outfitId);
  if(availableWeapon(s,weaponId,id)-(s.inventory.weaponId==weaponId&&!weaponId.empty() ? 1:0)<1||availableOutfit(s,outfitId,id)-(s.outfitId==outfitId&&!outfitId.empty() ? 1:0)<1)
    return {false,"Gear is in use. Unequip it or craft another copy"};
  std::string tech=job=="gatherer"||job=="crafter" ? "apprenticeship":job=="guard" ? "militia":job=="trader" ? "commerce_school":job=="fisher" ? "navigation_school":job=="scholar"||job=="engineer" ? "citizen_engineering":job=="ambassador" ? "diplomatic_corps":"";
  if(!tech.empty()&&!s.technologies.contains(tech)) return {false,"Training required: "+tech};
  if(job!="idle"&&(outfitId.empty()||outfit(outfitId).job!=job)) return {false,"This job needs matching work clothes"};
  if(job=="guard"&&weaponId.empty()) return {false,"Defenders need a weapon"};
  if(job=="guard"&&weapon(weaponId).kind=="bow") return {false,"Local defenders need a melee weapon; ranged troops are not implemented"};
  resident->job=job;resident->weapon=weaponId;resident->outfit=outfitId;resident->trained=job!="idle";return {true,"Resident assigned: "+job};
}
void tickResidents(Save& s,float dt) {
  if(!s.adventure) return;
  s.workClock+=dt;if(s.workClock<20) return;s.workClock=std::fmod(s.workClock,20.f);
  int batch=s.technologies.contains("guild_charter") ? 2:1;
  for(auto& n:s.citizens) if(ownsCity(s,n.city)&&n.trained&&s.relations[n.city].treaty!=Treaty::War) {
    int good=n.job=="gatherer" ? 0:n.job=="crafter" ? 1:n.job=="fisher" ? 4:n.job=="scholar" ? 2:n.job=="engineer" ? 3:-1;
    if(good>=0) s.stocks[n.city][good]=std::min(1000000,s.stocks[n.city][good]+batch);
    if(n.job=="ambassador") {
      int partner=-1;for(int city=0;city<int(s.relations.size());city++) if(!ownsCity(s,city)&&s.relations[city].discovered&&s.relations[city].treaty!=Treaty::War&&s.relations[city].opinion<50&&(partner<0||s.relations[city].opinion<s.relations[partner].opinion)) partner=city;
      if(partner>=0) {s.relations[partner].opinion++;if(s.campaign) s.cityOpinions[s.homeCity][partner]=s.cityOpinions[partner][s.homeCity]=s.relations[partner].opinion;}
    }
    if(n.job=="trader") s.treasury[n.city]=std::min(10000000,s.treasury[n.city]+batch*2);
  }
}
}
