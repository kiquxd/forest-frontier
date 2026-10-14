#include "summary.hpp"
#include <algorithm>

namespace frontier {
std::vector<SummaryEntry> inventorySummary(const Save& s) {
  std::vector<SummaryEntry> rows;
  rows.push_back({"coins","Coins","",s.coins});
  for(int g=0;g<goodTypes;g++) rows.push_back({"good_"+std::to_string(g),goodName(g),"Resource",goodCount(s.inventory,g)});
  auto add=[&](const char* id,const char* name,int count){rows.push_back({id,name,"",count});};
  add("potion","Healing potions",s.inventory.potions);add("campfire","Campfires",s.inventory.campfires);
  add("arrows","Arrows",s.inventory.ammo);add("repair","Repair kits",s.inventory.repairs);
  auto flag=[&](const char* id,const char* name,bool have){if(have) add(id,name,1);};
  flag("axe","Axe",s.inventory.axe);flag("pickaxe","Pickaxe",s.inventory.pickaxe);flag("rod","Fishing rod",s.inventory.rod);
  flag("boat","Rowboat",s.inventory.boat);flag("ship","Ship",s.inventory.ship);flag("submarine","Submarine",s.inventory.submarine);
  flag("sonar","Sonar",s.inventory.sonar);flag("oxygen","Oxygen module",s.inventory.oxygen);
  for(const auto& item:content().weapons) if(s.inventory.weapons.contains(item.id)) {
    auto found=s.weaponCopies.find(item.id);int copies=found==s.weaponCopies.end() ? 1:found->second;
    if(copies>0) rows.push_back({"weapon_"+item.id,item.name,"Damage "+std::to_string(item.damage)+" · "+rarityName(item.rarity)+(s.inventory.weaponId==item.id ? " · equipped":""),copies});
  }
  for(const auto& item:content().outfits) {
    auto found=s.outfitCopies.find(item.id);
    if(found!=s.outfitCopies.end()&&found->second>0) rows.push_back({"outfit_"+item.id,item.name,(s.outfitId==item.id ? "Worn · ":"")+item.description,found->second});
  }
  return rows;
}

std::vector<ProgressCard> progressCards(const Save& s) {
  std::vector<ProgressCard> cards;
  auto activity=[&](std::string id,std::string name,int amount,int row) {
    cards.push_back({id,name,id,"Recorded activity: "+std::to_string(amount)+". Each connected lesson has its own required count; click its cell to check progress.",{},0,row,amount>0,amount>0,true,amount});
  };
  activity("gather_sites","Gather sites",s.gathered,1);activity("craft_items","Craft items",s.crafted,2);
  activity("catch_fish","Catch fish",s.caught,3);activity("town_orders","Town orders",s.orders,4);
  const std::vector<std::string> parent={"","gathering","gather_sites","craft_items","craft_items","gather_sites","catch_fish","gather_sites","craft_items","trading","town_orders"};
  const int columns[]={0,1,1,2,2,1,2,1,2,2,1},rows[]={0,0,1,1,2,3,3,4,4,0,5};
  for(int i=0;i<int(skills().size());i++) {
    const auto& skill=skills()[i];auto state=lessonState(s,i);
    std::string detail=state.detail+". Learn at a peaceful town hall with P, or open K to learn. ";
    if(i==0) detail+="Unlocks gathering trees, stones, berries and reeds.";
    else detail+=skill.requirement+".";
    cards.push_back({skill.id,skill.name,"skill_"+skill.id,detail,parent[i].empty() ? std::vector<std::string>{}:std::vector<std::string>{parent[i]},columns[i],rows[i],state.learned,state.ready});
  }
  cards.push_back({"civic_charter","Civic charter","civic_charter","Quest reward: complete Know your neighbors (discover two new towns after accepting the quest).",{},2,5,s.technologies.contains("civic_charter"),false});
  const int researchRows[]={1,2,0,3,5,6,1};
  for(int i=0;i<int(researchNodes().size());i++) {
    const auto& n=researchNodes()[i];std::vector<std::string> parents{n.prerequisite};if(!n.secondPrerequisite.empty()) parents.push_back(n.secondPrerequisite);
    bool ready=researchReady(s,n);
    cards.push_back({n.id,n.name,"tech_"+n.id,researchRequirements(s,n)+". Open K to research.",parents,n.id=="guild_charter" ? 4:3,researchRows[i],s.technologies.contains(n.id),ready});
  }
  int row=3;std::vector<std::string> added;
  for(const auto& town:content().towns) {
    if(std::find(added.begin(),added.end(),town.technology)!=added.end()) continue;
    added.push_back(town.technology);
    bool learned=s.technologies.contains(town.technology),teacher=false;
    for(size_t city=0;city<settlements().size()&&city<s.relations.size();city++)
      if(content().towns[settlements()[city].type].technology==town.technology&&s.relations[city].discovered&&s.relations[city].treaty!=Treaty::War&&s.relations[city].opinion>=20) teacher=true;
    bool ready=!learned&&teacher&&hasSkill(s,"crafting")&&s.coins>=20;
    cards.push_back({town.technology,technologyName(town.technology),"tech_"+town.technology,"Find a "+town.name+" town master: peace, trust 20, Craftsmanship and 20 coins (you have "+std::to_string(s.coins)+"). Use F4 with the master or P at that town hall.",{"crafting"},4,row++,learned,ready});
  }
  return cards;
}
}
