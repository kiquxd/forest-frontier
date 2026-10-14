#include "summary.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <functional>
using namespace frontier;
static void check(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
int main() {
  try {
    configureWorld(742019);Save s;initializeCampaign(s);initializeAdventure(s);
    s.inventory.wood=17;s.coins=123;s.inventory.weapons.insert("wood_sword");s.weaponCopies["wood_sword"]=3;
    s.inventory.weaponId="wood_sword";s.outfitCopies["forager"]=2;
    auto rows=inventorySummary(s);
    auto find=[&](const std::string& id)->const SummaryEntry& {for(const auto& row:rows) if(row.icon==id) return row;throw std::runtime_error("Missing inventory row: "+id);};
    check(find("good_0").count==17&&find("coins").count==123,"Inventory summary reports real counts");
    check(find("weapon_wood_sword").count==3&&find("outfit_forager").count==2,"Every equipment kind gets a counted row");
    std::ifstream manifest(std::string(FRONTIER_ROOT)+"/assets/ui/icons.json");auto icons=nlohmann::json::parse(manifest).get<std::set<std::string>>();
    auto cards=progressCards(s);std::set<std::string> ids;std::set<std::pair<int,int>> cells;
    for(const auto& card:cards) {
      check(ids.insert(card.id).second,"Unique progress IDs");check(cells.insert({card.column,card.row}).second,"Tree cells do not overlap");
      check(icons.contains(card.icon),"Tree has a PNG icon for every node");
    }
    for(const auto& row:rows) check(icons.contains(row.icon),"Inventory has a PNG icon for every item");
    std::set<std::string> visiting,finished;
    std::function<void(const std::string&)> walk=[&](const auto& id) {
      check(ids.contains(id),"Dependency resolves to a visible node");if(finished.contains(id)) return;
      check(visiting.insert(id).second,"Dependency graph is acyclic");
      for(const auto& card:cards) if(card.id==id) for(const auto& parent:card.dependencies) walk(parent);
      visiting.erase(id);finished.insert(id);
    };
    for(const auto& card:cards) walk(card.id);
    auto node=[&](const std::string& id)->const ProgressCard& {for(const auto& card:cards) if(card.id==id) return card;throw std::runtime_error("Missing progress node");};
    check(node("gathering").ready&&!node("gathering").learned&&!node("trading").ready,"Fresh tree reflects locked skills");
    trainSkill(s,0);cards=progressCards(s);
    check(node("gathering").learned&&node("trading").ready,"Learning refreshes completion and dependent readiness");
    s.gathered=4;cards=progressCards(s);check(node("crafting").detail.find("4 / 5")!=std::string::npos&&!node("crafting").ready,"Activity requirement remains a real counter");
    s.gathered=5;cards=progressCards(s);check(node("crafting").ready,"Tree readiness matches the lesson operation");
    trainSkill(s,2);cards=progressCards(s);
    const auto& townTech=content().towns[settlements()[s.homeCity].type].technology;
    check(node(townTech).ready,"Known peaceful teacher shows town knowledge ready when its actual gates pass");
    Save probe=s;check(learnTechnology(probe,s.homeCity).ok,"Town-node readiness matches authoritative teaching");
    s.relations[s.homeCity].treaty=Treaty::War;cards=progressCards(s);check(!node(townTech).ready,"War blocks town teaching in the tree");
    check(s.coins==123&&s.weaponCopies.at("wood_sword")==3&&s.outfitCopies.at("forager")==2,"Summary and research readiness cannot spend resources");
    std::cout<<"Summary inventory, PNG coverage, graph validity and live prerequisites passed\n";
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
