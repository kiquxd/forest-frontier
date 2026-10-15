#include "gameplay.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "adventure.hpp"
#include "atlas_notes.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "settlement_system.hpp"
#include "vessels.hpp"

namespace frontier {
const char* GoodName(int good) {
  static const char* kNames[] = {
      "Wood",   "Stone",   "Berries", "Reeds", "Fish", "Leather", "Iron",
      "Copper", "Crystal", "Pearl",   "Steel", "Core", "Relic"};
  return good >= 0 && good < kGoodTypes ? kNames[good] : "";
}
int& GoodCount(Inventory& inventory, int good) {
  switch (good) {
    case 0:
      return inventory.wood;
    case 1:
      return inventory.stone;
    case 2:
      return inventory.berries;
    case 3:
      return inventory.reeds;
    case 4:
      return inventory.fish;
    case 5:
      return inventory.leather;
  }
  if (good >= 6 && good < kGoodTypes) {
    return inventory.rare[good - 6];
  }
  throw std::out_of_range("Invalid good");
}
int GoodCount(const Inventory& inventory, int good) {
  auto copy = inventory;
  return GoodCount(copy, good);
}
const std::vector<Recipe>& Recipes() {
  return GetContent().recipes;
}
const char* TechnologyName(const std::string& id) {
  if (id == "ironworking") {
    return "Ironworking";
  }
  if (id == "steelworking") {
    return "Steel alloys";
  }
  if (id == "shipbuilding") {
    return "Shipbuilding";
  }
  if (id == "submarine") {
    return "Pressure hulls";
  }
  if (id == "crystallurgy") {
    return "Crystal optics";
  }
  if (id == "diving") {
    return "Deep-sea harvesting";
  }
  if (id == "ancient") {
    return "Ancient machinery";
  }
  if (id == "fortification") {
    return "Stone fortifications";
  }
  for (auto& node : ResearchNodes()) {
    if (node.id == id) {
      return node.name.c_str();
    }
  }
  if (id == "medical_care") {
    return "Medical care";
  }
  if (id == "hospitals") {
    return "Hospitals";
  }
  if (id == "civic_charter") {
    return "Civic charter";
  }
  return "";
}
namespace {
bool Knows(const Save& save, const std::string& tech) {
  return tech.empty() || save.technologies.contains(tech);
}
}  // namespace

void EnsureCityState(Save& save) {
  auto total = Settlements().size();
  if (save.campaign &&
      (save.owners.size() != total || save.city_opinions.size() != total)) {
    throw std::runtime_error("Campaign settlement count does not match seed");
  }
  save.stocks.resize(total);
  save.treasury.resize(total, 150);
  while (save.relations.size() < total) {
    size_t id = save.relations.size();
    auto& city = Settlements()[id];
    save.relations.push_back(
        {city.attitude, city.attitude <= -50 ? Treaty::kWar : Treaty::kPeace,
         false});
    if (id >= 2) {
      save.stocks[id] = {40, 30, 25, 20, 12, 8};
    }
    for (int good : GetContent().towns[city.type].goods) {
      save.stocks[id][good] = 16;
    }
  }
}
bool OwnsCity(const Save& save, int city) {
  return city >= 0 && city < static_cast<int>(save.owners.size()) &&
         save.owners[city] == 0;
}
const std::vector<Skill>& Skills() {
  static const std::vector<Skill> kList = {
      {"gathering", "Gathering", "First lesson at any town hall"},
      {"trading", "Trading", "Learn gathering"},
      {"crafting", "Craftsmanship", "Gather from 5 resource sites"},
      {"tools", "Tools", "Craft 2 items"},
      {"combat", "Weaponsmithing", "Craft 2 items"},
      {"fishing", "Fishing", "Gather from 10 resource sites"},
      {"sailing", "Seamanship", "Catch 3 fish"},
      {"medicine", "Herbalism", "Gather from 10 resource sites"},
      {"construction", "Construction", "Craft 4 items"},
      {"diplomacy", "Diplomacy", "Learn trading"},
      {"engineering", "Engineering", "Complete a town order"}};
  return kList;
}
std::string SkillName(const std::string& id) {
  for (auto& skill : Skills()) {
    if (skill.id == id) {
      return skill.name;
    }
  }
  return id;
}
bool HasSkill(const Save& save, const std::string& id) {
  return !save.campaign || id.empty() || save.skills.contains(id);
}
void InitializeCampaign(Save& save, bool legacy) {
  EnsureCityState(save);
  if (save.campaign) {
    return;
  }
  const int n = static_cast<int>(Settlements().size());
  save.home_city = static_cast<int>(Hash(save.seed ^ 0x718acULL) % n);
  if (legacy) {
    float nearest = 1e30f;
    for (int i = 0; i < n; i++) {
      float d = Length(save.player - Settlements()[i].pos);
      if (d < nearest) {
        nearest = d;
        save.home_city = i;
      }
    }
  }
  save.owners.resize(n);
  save.city_opinions.assign(n, std::vector<int>(n));
  for (int i = 0; i < n; i++) {
    save.owners[i] = i == save.home_city ? 0 : i + 1;
    for (int j = 0; j <= i; j++) {
      const auto& a = Settlements()[i];
      const auto& b = Settlements()[j];
      int attitude =
          i == j
              ? 100
              : std::clamp(
                    (a.attitude + b.attitude) / 2 +
                        (a.type == b.type ? 25 : -5) +
                        (Length(a.pos - b.pos) < 4500 ? 8 : -8) +
                        static_cast<int>(Hash(save.seed ^ static_cast<uint64_t>(
                                                              i * 1009 + j)) %
                                         31) -
                        15,
                    -90, 90);
      save.city_opinions[i][j] = save.city_opinions[j][i] = attitude;
    }
  }
  if (legacy) {
    for (auto& skill : Skills()) {
      save.skills.insert(skill.id);
      RecordHistory(save, "skill", "Learned " + skill.name);
    }
    for (int i = 0; i < n; i++) {
      if (save.relations[i].treaty == Treaty::kUnion) {
        save.owners[i] = 0;
      }
    }
  } else {
    save.player = Settlements()[save.home_city].pos + Vec{110, 110};
    save.coins = 0;
    for (int i = 0; i < n; i++) {
      int opinion = save.city_opinions[save.home_city][i];
      save.relations[i] = {
          opinion, opinion <= -35 ? Treaty::kWar : Treaty::kPeace, false};
    }
  }
  save.relations[save.home_city] = {100, Treaty::kPeace, true};
  save.campaign = true;
}
LessonState GetLessonState(const Save& save, int index) {
  const auto& skill = Skills().at(index);
  int progress = 1, target = 1;
  if (index == 1) {
    progress = HasSkill(save, "gathering");
  } else if (index == 9) {
    progress = HasSkill(save, "trading");
  } else if (index == 2 || index == 5 || index == 7) {
    progress = save.gathered;
    target = index == 2 ? 5 : 10;
  } else if (index == 3 || index == 4 || index == 8) {
    progress = save.crafted;
    target = index == 8 ? 4 : 2;
  } else if (index == 6) {
    progress = save.caught;
    target = 3;
  } else if (index == 10) {
    progress = save.orders;
  }
  bool learned = save.skills.contains(skill.id), ready = progress >= target;
  std::string detail =
      learned      ? "Learned · available now"
      : index == 0 ? "Free introductory lesson · ready to learn"
                   : skill.requirement + " · " +
                         std::to_string(std::min(progress, target)) + " / " +
                         std::to_string(target) +
                         (ready ? " · ready to learn" : " · practice required");
  return {learned, ready, learned ? target : std::min(progress, target), target,
          detail};
}
Action TrainSkill(Save& save, int index) {
  if (index < 0 || index >= static_cast<int>(Skills().size())) {
    return {false, "Unknown skill"};
  }
  auto& skill = Skills()[index];
  auto state = GetLessonState(save, index);
  if (state.learned) {
    return {false, "Skill already learned"};
  }
  if (!state.ready) {
    return {false, state.detail};
  }
  save.skills.insert(skill.id);
  RecordHistory(save, "skill", "Learned " + skill.name);
  return {true, "Learned: " + skill.name};
}
std::string RecipeSkill(const Recipe& r) {
  const auto& a = r.result;
  if (a.starts_with("outfit:")) {
    return "crafting";
  }
  if (a == "axe" || a == "pickaxe") {
    return "tools";
  }
  if (a.starts_with("weapon:") || a == "ammo" || a == "armor") {
    return "combat";
  }
  if (a == "rod") {
    return "fishing";
  }
  if (a == "vessel:raft") {
    return "crafting";
  }
  if (a == "vessel:canoe") {
    return "tools";
  }
  if (a == "boat" || a == "ship" || a.starts_with("vessel:")) {
    return "sailing";
  }
  if (a == "potion") {
    return "medicine";
  }
  if (a == "submarine" || a == "sonar" || a == "oxygen" || a == "steel") {
    return "engineering";
  }
  return "crafting";
}
const char* TreatyName(Treaty treaty) {
  static const char* kNames[] = {"peace", "war", "truce", "alliance", "union"};
  return kNames[static_cast<int>(treaty)];
}
const WeaponDef& CurrentWeapon(const Save& save) {
  static const WeaponDef kFists = {"", "Fists", "fists", "",   10,
                                   0,  1,       90,      .55f, {}};
  if (!save.inventory.weapon_id.empty()) {
    return Weapon(save.inventory.weapon_id);
  }
  if (save.inventory.sword && save.inventory.weapons.empty()) {
    return Weapon("wood_sword");
  }
  return kFists;
}
std::vector<std::string> OwnedWeapons(const Save& save) {
  std::vector<std::string> out{""};
  for (auto& weapon : GetContent().weapons) {
    if (save.inventory.weapons.contains(weapon.id)) {
      out.push_back(weapon.id);
    }
  }
  return out;
}
Action EquipWeapon(Save& save, const std::string& id) {
  if (!id.empty() && !save.inventory.weapons.contains(id)) {
    return {false, "You do not own this weapon"};
  }
  if (save.adventure && AvailableWeapon(save, id) < 1) {
    return {false, "All copies are assigned to residents"};
  }
  save.inventory.weapon_id = id;
  return {true, "Equipped: " + CurrentWeapon(save).name};
}
bool Equipped(const Inventory& inventory, int recipe) {
  auto& result = Recipes().at(recipe).result;
  if (result.starts_with("weapon:") || result.starts_with("outfit:")) {
    return false;
  }
  if (result.starts_with("vessel:")) {
    return false;
  }
  if (result == "axe") {
    return inventory.axe;
  }
  if (result == "pickaxe") {
    return inventory.pickaxe;
  }
  if (result == "rod") {
    return inventory.rod;
  }
  if (result == "boat") {
    return inventory.boat;
  }
  if (result == "armor") {
    return false;
  }
  if (result == "ship") {
    return inventory.ship;
  }
  if (result == "submarine") {
    return inventory.submarine;
  }
  if (result == "sonar") {
    return inventory.sonar;
  }
  if (result == "oxygen") {
    return inventory.oxygen;
  }
  return false;
}
bool Equipped(const Save& save, int recipe) {
  const auto& result = Recipes().at(recipe).result;
  return result.starts_with("vessel:") ? save.vessels.contains(result.substr(7))
                                       : Equipped(save.inventory, recipe);
}
Action Craft(Save& save, int recipe) {
  if (save.nation) {
    return QueueProduction(save, recipe);
  }
  if (recipe < 0 || recipe >= static_cast<int>(Recipes().size())) {
    return {false, "Unknown recipe"};
  }
  InitializeTowns(save);
  auto& r = Recipes()[recipe];
  if (CraftWait(save) > 0) {
    return {false,
            "Crafting cooldown: " +
                std::to_string(static_cast<int>(std::ceil(CraftWait(save)))) +
                " game seconds"};
  }
  if (!FacilityAccess(save, CityAt(save, save.player), RecipeFacility(r))) {
    return {false, "Craft on town territory with a local or road-connected " +
                       RecipeFacility(r)};
  }
  if (!HasSkill(save, "crafting") || !HasSkill(save, RecipeSkill(r))) {
    return {false, "Skill locked. Learn at a town hall: " +
                       SkillName(!HasSkill(save, "crafting") ? "crafting"
                                                             : RecipeSkill(r))};
  }
  if (!RecipeKnown(save, r)) {
    return {false, "Complete an NPC quest to learn this recipe"};
  }
  if (!Knows(save, r.technology)) {
    return {false, "Town technology required: " +
                       std::string(TechnologyName(r.technology))};
  }
  if (Equipped(save, recipe)) {
    return {false, "Already crafted"};
  }
  int vessel_coins = 0;
  if (r.result.starts_with("vessel:")) {
    const auto v = Vessel(r.result.substr(7));
    if (save.vessels.contains(v.id)) {
      return {false, "You already own this vessel"};
    }
    if (v.prestige >= 6 && !Knows(save, "shipbuilding")) {
      return {false, "Learn Shipbuilding from a specialist town first"};
    }
    if (v.id == "flagship" && !Knows(save, "navigation_school")) {
      return {false, "Research Navigation school first"};
    }
    vessel_coins = v.coins;
    if (save.coins < vessel_coins) {
      return {false, "Vessel commissioning requires " +
                         std::to_string(vessel_coins) + " coins"};
    }
  }
  for (int i = 0; i < kGoodTypes; i++) {
    if (GoodCount(save.inventory, i) < r.cost[i]) {
      return {false, "Not enough: " + std::string(GoodName(i))};
    }
  }
  if ((r.result == "potion" && save.inventory.potions >= 1000000) ||
      (r.result == "campfire" && save.inventory.campfires >= 1000) ||
      (r.result == "ammo" && save.inventory.ammo > 999990) ||
      (r.result == "repair" && save.inventory.repairs >= 1000000) ||
      (r.result == "steel" && GoodCount(save.inventory, 10) > 999997)) {
    return {false, "Inventory full"};
  }
  if ((r.result.starts_with("weapon:") &&
       save.weapon_copies[r.result.substr(7)] >= 1000000) ||
      (r.result.starts_with("outfit:") &&
       save.outfit_copies[r.result.substr(7)] >= 1000000)) {
    return {false, "Inventory full"};
  }
  for (int i = 0; i < kGoodTypes; i++) {
    GoodCount(save.inventory, i) -= r.cost[i];
  }
  save.coins -= vessel_coins;
  auto& result = r.result;
  if (result.starts_with("vessel:")) {
    const auto v = Vessel(result.substr(7));
    save.vessels.insert(v.id);
    save.vehicle = v.number;
    RecordHistory(save, "vessel", "Commissioned " + v.name);
  } else if (result.starts_with("weapon:")) {
    save.inventory.weapon_id = result.substr(7);
    save.inventory.weapons.insert(save.inventory.weapon_id);
    save.weapon_copies[save.inventory.weapon_id]++;
    save.inventory.sword = true;
  } else if (result.starts_with("outfit:")) {
    save.outfit_copies[result.substr(7)]++;
  } else if (result == "axe") {
    save.inventory.axe = true;
  } else if (result == "pickaxe") {
    save.inventory.pickaxe = true;
  } else if (result == "rod") {
    save.inventory.rod = true;
  } else if (result == "boat") {
    save.inventory.boat = true;
    save.vehicle = 1;
  } else if (result == "armor") {
    save.inventory.armor = true;
    save.outfit_copies["leather_armor"]++;
    save.outfit_id = "leather_armor";
  } else if (result == "potion") {
    save.inventory.potions++;
  } else if (result == "campfire") {
    save.inventory.campfires++;
  } else if (result == "ammo") {
    save.inventory.ammo += 10;
  } else if (result == "ship") {
    save.inventory.ship = true;
    save.vehicle = 2;
  } else if (result == "submarine") {
    save.inventory.submarine = true;
    save.vehicle = 3;
  } else if (result == "steel") {
    GoodCount(save.inventory, 10) += 3;
  } else if (result == "sonar") {
    save.inventory.sonar = true;
  } else if (result == "oxygen") {
    save.inventory.oxygen = true;
    save.oxygen = 200;
  } else if (result == "repair") {
    save.inventory.repairs++;
  }
  save.crafted++;
  save.craft_ready_at = save.game_time + CraftDelay(r);
  RememberResources(save);
  return {true, "Crafted: " + r.name};
}
int BuyPrice(int city, int good) {
  constexpr int kBase[kGoodTypes] = {2,  4,  3,  3,  7,  9, 12,
                                     14, 24, 20, 26, 65, 90};
  if (city < 0 || city >= static_cast<int>(Settlements().size()) || good < 0 ||
      good >= kGoodTypes) {
    return 0;
  }
  if (city == 1 && good < 6) {
    constexpr int kPrices[] = {3, 3, 2, 2, 6, 10};
    return kPrices[good];
  }
  return kBase[good] + (city >= 2 ? city % 3 : 0);
}
int SellPrice(int city, int good) {
  return std::max(1,
                  BuyPrice(city, good) - std::max(2, BuyPrice(city, good) / 3));
}
namespace {
std::string MaterialTech(int good) {
  switch (good) {
    case 6:
      return "ironworking";
    case 7:
      return "shipbuilding";
    case 8:
      return "crystallurgy";
    case 9:
      return "diving";
    case 10:
      return "steelworking";
    case 11:
      return "submarine";
    case 12:
      return "ancient";
    default:
      return "";
  }
}
}  // namespace

Action Trade(Save& save, int city, int good, bool buy) {
  if (save.nation && OwnsCity(save, city)) {
    return {false, "Use the town warehouse; internal transfers have no price"};
  }
  if (!HasSkill(save, "trading")) {
    return {false, "Learn trading at a town hall"};
  }
  EnsureCityState(save);
  if (city < 0 || city >= static_cast<int>(Settlements().size()) || good < 0 ||
      good >= kGoodTypes) {
    return {false, "Merchant unavailable"};
  }
  auto relation = save.relations[city];
  if (relation.treaty == Treaty::kWar || relation.opinion < -20) {
    return {false, "Town refuses to trade: negotiate a truce"};
  }
  if (good >= 6 && buy) {
    auto& type = GetContent().towns[Settlements()[city].type];
    if (std::find(type.goods.begin(), type.goods.end(), good) ==
        type.goods.end()) {
      return {false, "This town does not produce that material"};
    }
    if (relation.opinion < 20 || !Knows(save, MaterialTech(good))) {
      return {false, "Trust and town technology required; visit a town hall"};
    }
  }
  int& count = GoodCount(save.inventory, good);
  int& stock = save.stocks[city][good];
  int& money = save.treasury[city];
  int price =
      buy ? BuyPrice(city, good)
          : std::max(1,
                     static_cast<int>(std::lround(SellPrice(city, good) *
                                                  PlayerCharacter(save).trade *
                                                  PlayerOutfit(save).trade)));
  if (buy) {
    if (stock <= 0) {
      return {false, "Out of stock"};
    }
    if (save.coins < price) {
      return {false, "Not enough coins"};
    }
    if (count >= 1000000 || money > 10000000 - price) {
      return {false, "Trade limit reached"};
    }
    save.coins -= price;
    money += price;
    stock--;
    count++;
  } else {
    if (count <= 0) {
      return {false, "Nothing to sell"};
    }
    if (money < price + (save.nation ? 40 : 0)) {
      return {false, "Town treasury empty"};
    }
    if (stock >= 1000000 || save.coins > 10000000 - price) {
      return {false, "Trade limit reached"};
    }
    save.coins += price;
    money -= price;
    stock++;
    count--;
  }
  RememberResources(save);
  return {true, std::string(buy ? "Bought: " : "Sold: ") + GoodName(good)};
}
const std::vector<std::string>& CityActions() {
  static const std::vector<std::string> kActions = [] {
    std::vector<std::string> list = {
        "Gift to town · 20 coins · +15 trust",
        "Negotiate a truce · 25 coins",
        "Declare war · local conflict placeholder",
        "Propose alliance · trust 30 · 10 coins",
        "Unite · alliance, trust 70 · 40 coins",
        "Learn town technology · trust 20 · 20 coins",
        "Build wall segment · wood 8, stone 3",
        "Buy rare town weapon · 80 coins",
        "Town order · deliver 5 wood and 3 fish"};
    list.push_back("Audience with ruler / your town");
    for (auto& skill : Skills()) {
      list.push_back("Lesson: " + skill.name + " · " + skill.requirement);
    }
    list.push_back("Relations with other towns");
    list.push_back("Expand territory · increasing wood / stone / coin cost");
    for (const auto& f : FacilityTypes()) {
      list.push_back("Build " + f.name + " · wood " + std::to_string(f.wood) +
                     ", stone " + std::to_string(f.stone) + ", coins " +
                     std::to_string(f.coins));
    }
    list.push_back("Surrender to this ruler · 30 now, tribute 10 / 120s");
    list.push_back("Buy independence · 300 coins");
    list.push_back("Restore previous allegiance · 300 coins");
    list.push_back("Ransom first captive held here · price depends on weapon");
    return list;
  }();
  return kActions;
}
Action LearnTechnology(Save& save, int city) {
  EnsureCityState(save);
  if (city < 0 || city >= static_cast<int>(Settlements().size())) {
    return {false, "Town unavailable"};
  }
  auto& r = save.relations[city];
  auto& type = GetContent().towns[Settlements()[city].type];
  if (r.treaty == Treaty::kWar || r.opinion < 20) {
    return {false, "Masters require peace and trust 20"};
  }
  if (save.technologies.contains(type.technology)) {
    return {false, "Technology already learned"};
  }
  if (!HasSkill(save, "crafting")) {
    return {false, "Learn craftsmanship at a town hall first"};
  }
  if (save.coins < 20) {
    return {false, "Research costs 20 coins"};
  }
  save.coins -= 20;
  save.treasury[city] += 20;
  save.technologies.insert(type.technology);
  RecordHistory(save, "technology",
                "Learned " + std::string(TechnologyName(type.technology)),
                city);
  return {true, "Learned: " + std::string(TechnologyName(type.technology))};
}
const WeaponDef& CityWeapon(int city, uint64_t seed) {
  int minimum = std::max(1, Settlements().at(city).rarity), total = 0;
  for (auto& item : GetContent().weapons) {
    if (item.rarity >= minimum) {
      total += item.weight;
    }
  }
  int roll =
      static_cast<int>(Hash(seed ^ static_cast<uint64_t>(city) * 0x9812) %
                       static_cast<uint64_t>(total));
  for (auto& item : GetContent().weapons) {
    if (item.rarity >= minimum) {
      roll -= item.weight;
      if (roll < 0) {
        return item;
      }
    }
  }
  return GetContent().weapons.back();
}
Action Diplomacy(Save& save, int city, int action) {
  EnsureCityState(save);
  if (city < 0 || city >= static_cast<int>(Settlements().size())) {
    return {false, "Town unavailable"};
  }
  int extra = 11 + static_cast<int>(Skills().size());
  if (action == extra) {
    return ExpandCity(save, city);
  }
  if (action > extra &&
      action <= extra + static_cast<int>(FacilityTypes().size())) {
    return BuildFacility(save, city, FacilityTypes()[action - extra - 1].id);
  }
  int politics = extra + 1 + static_cast<int>(FacilityTypes().size());
  if (action == politics) {
    int local = CityAt(save, save.player);
    return SurrenderTown(save, OwnsCity(save, local) ? local : save.home_city,
                         city);
  }
  if (action == politics + 1 || action == politics + 2) {
    return BuyIndependence(save, city, action == politics + 2);
  }
  if (action == politics + 3) {
    for (const auto& resident : save.citizens) {
      if (resident.state == "captive" && resident.captor == city) {
        return RansomResident(save, resident.npc);
      }
    }
    return {false, "This ruler holds none of your residents"};
  }
  auto& r = save.relations[city];
  struct SyncOpinion {
    Save& save;
    int city;
    Treaty previous;
    ~SyncOpinion() {
      if (previous != save.relations[city].treaty) {
        RecordHistory(save, "treaty",
                      Settlements()[city].name + ": " +
                          TreatyName(save.relations[city].treaty),
                      city);
      }
      if (save.campaign && save.home_city >= 0 && !OwnsCity(save, city)) {
        save.city_opinions[city][save.home_city] =
            save.city_opinions[save.home_city][city] =
                save.relations[city].opinion;
      }
    }
  } sync{save, city, r.treaty};
  if (OwnsCity(save, city) && action < 5) {
    return {false, "This is your town. No treaty needed"};
  }
  if (action >= 10 && action < 10 + static_cast<int>(Skills().size())) {
    if (r.treaty == Treaty::kWar) {
      return {false, "Lessons unavailable during war"};
    }
    return TrainSkill(save, action - 10);
  }
  if (action == 9) {
    return {false, OwnsCity(save, city)
                       ? "You rule this town. The town hall is vacant."
                       : "The ruler receives visitors in the town hall"};
  }
  if (action == 10 + static_cast<int>(Skills().size())) {
    std::string out = "Visitors: ";
    if (!save.campaign) {
      return {false, "No data"};
    }
    for (int i = 0; i < static_cast<int>(Settlements().size()); i++) {
      if (i != city && save.relations[i].discovered) {
        out += Settlements()[i].name + " " +
               std::to_string(save.city_opinions[city][i]) + "; ";
      }
    }
    return {true, out};
  }
  if (action < 5 && !HasSkill(save, "diplomacy")) {
    return {false, "Learn diplomacy at a town hall"};
  }
  if (action == 6 && !HasSkill(save, "construction")) {
    return {false, "Learn construction at a town hall"};
  }
  if (action == 6 && save.campaign && !OwnsCity(save, city) &&
      r.treaty != Treaty::kAlliance && r.treaty != Treaty::kUnion) {
    return {false, "Build walls in your own or an allied town"};
  }
  if (action == 5) {
    return LearnTechnology(save, city);
  }
  if (action == 2) {
    r.treaty = Treaty::kWar;
    r.opinion = -80;
    return {true,
            "War declared. Guards are hostile; strategic warfare remains a "
            "placeholder."};
  }
  if (action == 1) {
    if (save.coins < 25) {
      return {false, "A truce costs 25 coins"};
    }
    save.coins -= 25;
    save.treasury[city] += 25;
    r.treaty = Treaty::kTruce;
    r.opinion = std::max(0, r.opinion);
    return {true, "Truce agreed. Guards stop attacking."};
  }
  if (r.treaty == Treaty::kWar) {
    return {false, "Negotiate a truce first"};
  }
  if (action == 0) {
    if (save.coins < 20) {
      return {false, "You need 20 coins"};
    }
    save.coins -= 20;
    save.treasury[city] += 20;
    r.opinion = std::min(100, r.opinion + 15);
    return {true, "Town accepted the gift: +15 trust"};
  }
  if (action == 3) {
    if (r.opinion < 30 || save.coins < 10) {
      return {false, "Alliance requires trust 30 and 10 coins"};
    }
    if (r.treaty == Treaty::kAlliance || r.treaty == Treaty::kUnion) {
      return {false, "Already allied"};
    }
    save.coins -= 10;
    save.treasury[city] += 10;
    r.treaty = Treaty::kAlliance;
    r.opinion = std::max(40, r.opinion);
    return {true, "Alliance formed. Complete orders to build trust."};
  }
  if (action == 4) {
    if (r.treaty != Treaty::kAlliance || r.opinion < 70 || save.coins < 40) {
      return {false, "Requires alliance, trust 70 and 40 coins"};
    }
    save.coins -= 40;
    save.treasury[city] += 40;
    r.treaty = Treaty::kUnion;
    r.opinion = 95;
    if (save.campaign) {
      save.owners[city] = 0;
      SyncCitizens(save);
    }
    return {true, "Town joined your union: treasury income every 20 seconds."};
  }
  if (action == 6) {
    int& wood = save.nation ? save.stocks[city][0] : save.inventory.wood;
    int& stone = save.nation ? save.stocks[city][1] : save.inventory.stone;
    if (r.opinion < 0) {
      return {false, "Town refuses construction"};
    }
    int built = 0;
    for (auto& w : save.walls) {
      if (w.city == city) {
        built++;
      }
    }
    bool damaged = false;
    for (auto& w : save.walls) {
      if (w.city == city && w.hp == 0) {
        damaged = true;
      }
    }
    if (built >= 11 && !damaged) {
      return {false, "Wall complete; gate left on the east"};
    }
    if (wood < 8 || stone < 3) {
      return {false, "You need 8 wood and 3 stone"};
    }
    float a = (built + 1) * 6.2831853f / 12, b = (built + 2) * 6.2831853f / 12;
    auto position = Settlements()[city].pos;
    for (auto& wall : save.walls) {
      if (wall.city == city && wall.hp == 0) {
        wall.hp = save.technologies.contains("fortification") ? 400 : 200;
        wood -= 8;
        stone -= 3;
        return {true, "Wall repaired"};
      }
    }
    save.walls.push_back(
        {city, position + Vec{std::cos(a) * 355, std::sin(a) * 355},
         position + Vec{std::cos(b) * 355, std::sin(b) * 355},
         save.technologies.contains("fortification") ? 400 : 200});
    wood -= 8;
    stone -= 3;
    r.opinion = std::min(100, r.opinion + 2);
    return {true, "Wall segment built"};
  }
  if (action == 7) {
    if (r.opinion < 30 || save.coins < 80) {
      return {false, "Requires trust 30 and 80 coins"};
    }
    auto& item = CityWeapon(city, save.seed);
    if (save.inventory.weapons.contains(item.id)) {
      return {false, "You already own this town's weapon"};
    }
    save.coins -= 80;
    save.treasury[city] += 80;
    save.inventory.weapons.insert(item.id);
    if (save.nation) {
      ++GetCitizen(save, save.controlled_npc)->weapons[item.id];
    }
    save.weapon_copies[item.id]++;
    save.inventory.weapon_id = item.id;
    return {true,
            "Received: " + item.name + " (" + RarityName(item.rarity) + ")"};
  }
  if (action == 8 && save.nation) {
    return {false, "Use Town projects or Trade orders in the town hall"};
  }
  if (action == 8) {
    if (save.inventory.wood < 5 || save.inventory.fish < 3) {
      return {false, "Order requires 5 wood and 3 fish"};
    }
    if (save.treasury[city] < 18) {
      return {false, "Town cannot pay for the order yet"};
    }
    save.inventory.wood -= 5;
    save.inventory.fish -= 3;
    save.stocks[city][0] += 5;
    save.stocks[city][4] += 3;
    save.treasury[city] -= 18;
    save.coins += 18;
    save.orders++;
    r.opinion = std::min(100, r.opinion + 12);
    return {true, "Order completed: +18 coins, +12 trust"};
  }
  return {false, "Unknown action"};
}
bool Watercraft(const Save& save) {
  return save.inventory.boat || save.inventory.ship ||
         save.inventory.submarine || !save.vessels.empty();
}
int ActiveVehicle(const Save& save) {
  if (save.submerged) {
    return 3;
  }
  if (save.vehicle >= 1 && save.vehicle <= 9 &&
      OwnsVessel(save, save.vehicle)) {
    return save.vehicle;
  }
  for (int id : {3, 2, 1, 9, 8, 7, 6, 5, 4}) {
    if (OwnsVessel(save, id)) {
      return id;
    }
  }
  return 0;
}
Action CycleVehicle(Save& save) {
  if (save.submerged) {
    return {false, "Surface before selecting another vessel"};
  }
  for (int i = 1; i <= 9; i++) {
    int id = (save.vehicle + i - 1) % 9 + 1;
    if (OwnsVessel(save, id)) {
      save.vehicle = id;
      return {true, "Selected: " + Vessel(id).name + " · prestige " +
                        std::to_string(Vessel(id).prestige)};
    }
  }
  return {false, "Craft a vessel first"};
}
Action Dive(Save& save) {
  if (!save.inventory.submarine || ActiveVehicle(save) != 3) {
    return {false, "Craft a submarine and select it with V"};
  }
  auto terrain = TerrainAt(save.player, save.seed);
  if (terrain != Terrain::kDeep && terrain != Terrain::kShallow) {
    return {false, "You can dive only on water"};
  }
  if (!save.submerged && save.oxygen < 15) {
    return {false, "Wait on the surface for air to refill"};
  }
  if (!save.submerged && save.hull <= 15) {
    return {false, "Repair the hull with J first"};
  }
  save.submerged = !save.submerged;
  return {true,
          save.submerged
              ? "Diving. Surface, sonar and gathering actions are available."
              : "Submarine surfaced"};
}
Action RepairVessel(Save& save) {
  if (save.hull >= 100) {
    return {false, "Hull intact"};
  }
  if (save.inventory.repairs < 1) {
    return {false, "Craft a repair kit"};
  }
  save.inventory.repairs--;
  save.hull = std::min(100, save.hull + 45);
  return {true, "Hull repaired by 45"};
}
Action Sonar(Save& save) {
  if (!save.submerged || !save.inventory.sonar) {
    return {false, "Dive and craft a sonar first"};
  }
  for (int y = -18; y <= 18; y++) {
    for (int x = -18; x <= 18; x++) {
      if (x * x + y * y <= 18 * 18) {
        auto center = MapCell(save.player);
        Coord cell{center.x + x, center.y + y};
        Vec position{(cell.x + .5f) * kMapCellSize,
                     (cell.y + .5f) * kMapCellSize};
        auto t = TerrainAt(position, save.seed);
        if (t == Terrain::kDeep || t == Terrain::kShallow) {
          save.explored_underwater.insert(cell);
        }
      }
    }
  }
  return {true, "Sonar revealed the seafloor"};
}
int ResourceGood(ResourceKind kind) {
  if (static_cast<int>(kind) <= 4) {
    return static_cast<int>(kind);
  }
  if (kind == ResourceKind::kOre) {
    return 6;
  }
  if (kind == ResourceKind::kCrystal) {
    return 8;
  }
  if (kind == ResourceKind::kPearl) {
    return 9;
  }
  return 12;
}
Action Consume(Save& save, bool potion) {
  if (save.health >= 100) {
    return {false, "Health already full"};
  }
  if (potion) {
    if (save.inventory.potions <= 0) {
      return {false, "No healing potions; craft more"};
    }
    save.inventory.potions--;
    save.health = std::min(100, save.health + 45);
    return {true, "Potion: +45 health"};
  }
  if (save.inventory.fish > 0) {
    save.inventory.fish--;
    save.health = std::min(100, save.health + 12);
    return {true, "Fish: +12 health"};
  }
  if (save.inventory.berries > 0) {
    save.inventory.berries--;
    save.health = std::min(100, save.health + 5);
    return {true, "Berries: +5 health"};
  }
  return {false, "No food"};
}
float HarvestDuration(const Resource& resource, const Inventory& inventory) {
  if (resource.kind == ResourceKind::kFish) {
    return 1.8f;
  }
  if (resource.kind == ResourceKind::kTree && inventory.axe) {
    return .3f;
  }
  if (resource.kind == ResourceKind::kStone && inventory.pickaxe) {
    return .35f;
  }
  return .7f;
}
Action Harvest(Save& save, const Resource& resource) {
  if (!HasSkill(save, "gathering")) {
    return {false, "First gathering lesson: P at a town hall entrance"};
  }
  if (save.harvested.contains(resource.id)) {
    return {false, "Already gathered"};
  }
  if (resource.kind == ResourceKind::kFish && !save.inventory.rod) {
    return {false, "Fishing requires a crafted rod"};
  }
  if ((resource.kind == ResourceKind::kCrystal ||
       resource.kind == ResourceKind::kPearl ||
       resource.kind == ResourceKind::kSalvage) &&
      !save.submerged) {
    return {false, "Dive in a submarine to harvest this resource"};
  }
  int good = ResourceGood(resource.kind);
  if (good >= 6 && !Knows(save, MaterialTech(good))) {
    return {false, "Town technology required: " +
                       std::string(TechnologyName(MaterialTech(good)))};
  }
  if (resource.kind == ResourceKind::kOre && !save.inventory.pickaxe) {
    return {false, "Pickaxe required"};
  }
  int amount = good == 0 ? 3 : good <= 3 ? 2 : 1;
  if (GoodCount(save.inventory, good) > 1000000 - amount) {
    return {false, "Inventory full"};
  }
  GoodCount(save.inventory, good) += amount;
  save.known_goods[good] = true;
  save.harvested.insert(resource.id);
  save.gathered++;
  if (resource.kind == ResourceKind::kFish) {
    save.caught++;
  }
  return {true, "+" + std::to_string(amount) + " " + GoodName(good)};
}
void Gameplay::Advance(float dt) {
  for (auto& effect : hit_effects) {
    effect.age += dt;
  }
  std::erase_if(hit_effects, [](const auto& effect) {
    return effect.age > (effect.defeated ? 1.5f : .3f);
  });
  if (swing > 0) {
    attack_elapsed += dt;
    swing = std::max(0.f, attack_duration - attack_elapsed);
  }
  invulnerable = std::max(0.f, invulnerable - dt);
  attack_cooldown = std::max(0.f, attack_cooldown - dt);
  sonar_cooldown = std::max(0.f, sonar_cooldown - dt);
}
Vec Gameplay::MovePlayer(World& world, Save& save, Vec displacement) {
  auto terrain = TerrainAt(save.player, save.seed);
  bool on_bridge = !save.submerged && BridgeAt(save, save.player);
  bool afloat =
      !on_bridge && (terrain == Terrain::kShallow || terrain == Terrain::kDeep);
  int vehicle = afloat ? ActiveVehicle(save) : 0;
  float factor = save.submerged ? 1.1f
                 : vehicle      ? Vessel(vehicle).speed
                 : terrain == Terrain::kShallow && !on_bridge ? .45f
                                                              : 1.f;
  if (swing > 0 && vehicle == 0) {
    factor *= .45f;
  }
  factor *=
      PlayerOutfit(save).speed * (vehicle ? 1.f : PlayerCharacter(save).speed);
  Vec previous = save.player;
  save.player = world.Move(
      save.player, displacement * factor, save,
      vehicle ? std::clamp(Vessel(vehicle).width * .6f, 10.f, 25.f) : 12.f,
      Watercraft(save));
  world.ResolvePlayerContacts(save);
  Vec actual = save.player - previous;
  moving = Length(actual) > .001f;
  if (Length(displacement) > .001f) {
    facing = Normalized(displacement);
  }
  if (moving) {
    facing = Normalized(actual);
    if (!vehicle && !save.submerged) {
      walk = std::fmod(walk + Length(actual) / 20, 4.f);
    }
  }
  if (Length(actual) + .01f < Length(displacement * factor)) {
    collisions++;
  }
  return actual;
}
void Gameplay::Explore(Save& save, float, float, float, const World* world) {
  auto& cells = save.submerged ? save.explored_underwater : save.explored;
  auto& visible =
      save.submerged ? save.visible_underwater : save.visible_surface;
  visible.clear();
  const float range = SightRange(save);
  int radius = static_cast<int>(std::ceil(range / kMapCellSize));
  auto origin = MapCell(save.player);
  cells.insert(origin);
  visible.insert(origin);
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      Coord cell{origin.x + x, origin.y + y};
      Vec p{(cell.x + .5f) * kMapCellSize, (cell.y + .5f) * kMapCellSize};
      if (Length(p - save.player) > range) {
        continue;
      }
      if (save.submerged) {
        auto t = TerrainAt(p, save.seed);
        if (t != Terrain::kDeep && t != Terrain::kShallow) {
          continue;
        }
      }
      if (world && world->SightBlocked(save.player, p, save)) {
        continue;
      }
      cells.insert(cell);
      visible.insert(cell);
    }
  }
  if (world) {
    for (const auto& [coord, chunk] : world->chunks) {
      for (const auto& resource :
           save.submerged ? chunk.underwater : chunk.resources) {
        int good = ResourceGood(resource.kind);
        if (good >= 6 && visible.contains(MapCell(resource.pos)) &&
            !save.harvested.contains(resource.id) &&
            save.known_deposits.size() < 4096) {
          save.known_deposits.emplace(
              resource.id, KnownDeposit{resource.pos, good, save.submerged});
        }
      }
    }
  }
  EnsureCityState(save);
  for (size_t i = 0; i < Settlements().size(); i++) {
    // Approaching a hall identifies its centre even though the foundation
    // blocks the ordinary sight ray. Reveal only that cell, not the town.
    if (!save.submerged && Length(save.player - Settlements()[i].pos) <= 180) {
      save.explored.insert(MapCell(Settlements()[i].pos));
      save.visible_surface.insert(MapCell(Settlements()[i].pos));
    }
    if (!save.relations[i].discovered &&
        ExploredAt(save.explored, Settlements()[i].pos)) {
      save.relations[i].discovered = true;
      RecordHistory(save, "discovery", "Discovered " + Settlements()[i].name,
                    static_cast<int>(i));
    }
  }
}
bool Gameplay::RepairSpawn(const World& world, Save& save) const {
  if (!world.chunks.contains(ChunkAt(save.player))) {
    return false;
  }
  if (world.Clear(save.player, save, 12, Watercraft(save))) {
    return true;
  }
  Vec origin = save.player;
  for (int r = 1; r < 32; r++) {
    for (int y = -r; y <= r; y++) {
      for (int x = -r; x <= r; x++) {
        if (std::abs(x) != r && std::abs(y) != r) {
          continue;
        }
        Vec position = origin + Vec{static_cast<float>(x) * 12,
                                    static_cast<float>(y) * 12};
        if (world.Clear(position, save, 12, Watercraft(save))) {
          save.player = position;
          return true;
        }
      }
    }
  }
  for (auto& village : Settlements()) {
    Vec safe = village.pos + Vec{120, 140};
    if (world.Clear(safe, save, 12, Watercraft(save))) {
      save.player = safe;
      return true;
    }
  }
  return false;
}
Action Gameplay::Attack(World&, Save& save) {
  if (attack_cooldown > 0) {
    return {false, ""};
  }
  auto terrain = TerrainAt(save.player, save.seed);
  if (save.submerged ||
      ((terrain == Terrain::kDeep || terrain == Terrain::kShallow) &&
       Watercraft(save) && !BridgeAt(save, save.player))) {
    return {false, "Weapons unavailable aboard vessels"};
  }
  attack_weapon_ = CurrentWeapon(save);
  attack_weapon_.damage =
      std::max(1, static_cast<int>(std::lround(attack_weapon_.damage *
                                               PlayerCharacter(save).damage)));
  if (attack_weapon_.kind == "bow" && save.inventory.ammo < 1) {
    return {false, "No arrows; craft more"};
  }
  if (attack_weapon_.kind == "bow") {
    save.inventory.ammo--;
  }
  attack_duration = attack_weapon_.duration;
  attack_elapsed = 0;
  swing = attack_duration;
  attack_cooldown = attack_duration;
  attack_facing = facing;
  hit_resolved_ = false;
  attacks++;
  return {true, ""};
}
void Gameplay::Damage(World& world, Save& save, Enemy& enemy, int amount,
                      Vec direction) {
  if (save.defeated.contains(enemy.id)) {
    return;
  }
  enemy.hp -= amount;
  hits++;
  if (hit_effects.size() >= 64) {
    hit_effects.erase(hit_effects.begin());
  }
  hit_effects.push_back({enemy.pos, 0, enemy.hp <= 0, enemy.kind});
  enemy.flash = .22f;
  enemy.windup = 0;
  enemy.cooldown = std::max(enemy.cooldown, .5f);
  enemy.pos = world.Move(enemy.pos, Normalized(direction) * 18, save, 10);
  enemy.previous = enemy.pos;
  if (enemy.hp > 0) {
    notice = "Hit: " + std::to_string(amount);
    return;
  }
  if (!enemy.kind) {
    save.wolf_kills++;
  }
  save.defeated.insert(enemy.id);
  if (!enemy.kind) {
    save.known_goods[5] = true;
  }
  save.inventory.leather =
      std::min(1000000, save.inventory.leather + (enemy.kind ? 0 : 2));
  if (!save.nation) {
    save.coins = std::min(10000000, save.coins + (enemy.kind ? 8 : 3));
  }
  kills++;
  notice = save.nation
               ? (enemy.kind ? "Guard defeated; equipment is not dropped"
                             : "Wolf defeated: +2 leather")
           : enemy.kind ? "Guard defeated: +8 coins"
                        : "Wolf defeated: +2 leather, +3 coins";
  // Persistent defeated IDs make loot deterministic and prevent repeated rolls
  // on reload.
  if (!save.nation && Hash(enemy.id ^ save.seed) % 100 < 18) {
    auto& found = WeightedWeapon(Hash(enemy.id));
    save.inventory.weapons.insert(found.id);
    save.weapon_copies[found.id]++;
    notice += " · Found: " + found.name + " (" + RarityName(found.rarity) + ")";
  }
}
void Gameplay::Combat(World& world, Save& save, float dt) {
  if (!hit_resolved_ &&
      attack_elapsed >=
          attack_duration * (attack_weapon_.kind == "bow" ? .3f
                             : attack_weapon_.kind == "spear" ||
                                     attack_weapon_.kind == "fists"
                                 ? .35f
                                 : .455f)) {
    hit_resolved_ = true;
    if (attack_weapon_.kind == "bow") {
      projectiles.push_back({save.player + attack_facing * 22,
                             attack_facing * 650, attack_weapon_.reach / 650,
                             attack_weapon_.damage});
    } else {
      Coord center = ChunkAt(save.player);
      for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
          auto it = world.chunks.find({center.x + x, center.y + y});
          if (it == world.chunks.end()) {
            continue;
          }
          for (auto& enemy : it->second.enemies) {
            if (save.defeated.contains(enemy.id) ||
                (enemy.kind &&
                 save.relations.at(enemy.city).treaty != Treaty::kWar)) {
              continue;
            }
            Vec d = enemy.pos - save.player;
            float distance = Length(d);
            float cone = attack_weapon_.kind == "spear" ? .78f : .1f;
            if (distance > attack_weapon_.reach ||
                (distance > 12 &&
                 (d.x * attack_facing.x + d.y * attack_facing.y) / distance <
                     cone)) {
              continue;
            }
            bool blocked = false;
            for (int step = 1; step < 6; step++) {
              if (!world.Clear(save.player + d * (step / 6.f), save, 3,
                               false)) {
                blocked = true;
              }
            }
            if (!blocked) {
              Damage(world, save, enemy, attack_weapon_.damage, d);
            }
          }
        }
      }
    }
  }
  for (auto& arrow : projectiles) {
    arrow.life -= dt;
    int steps = std::max(
        1, static_cast<int>(std::ceil(Length(arrow.velocity) * dt / 8)));
    for (int i = 0; i < steps && arrow.life > 0; i++) {
      arrow.pos = arrow.pos + arrow.velocity * (dt / steps);
      if (!world.Clear(arrow.pos, save, 2, false)) {
        arrow.life = 0;
        break;
      }
      auto coord = ChunkAt(arrow.pos);
      for (int y = -1; y <= 1 && arrow.life > 0; y++) {
        for (int x = -1; x <= 1 && arrow.life > 0; x++) {
          auto it = world.chunks.find({coord.x + x, coord.y + y});
          if (it == world.chunks.end()) {
            continue;
          }
          for (auto& enemy : it->second.enemies) {
            if (!save.defeated.contains(enemy.id) &&
                (!enemy.kind ||
                 save.relations.at(enemy.city).treaty == Treaty::kWar) &&
                Length(enemy.pos - arrow.pos) < 19) {
              Damage(world, save, enemy, arrow.damage, arrow.velocity);
              arrow.life = 0;
              break;
            }
          }
        }
      }
    }
  }
  std::erase_if(projectiles, [](auto& p) { return p.life <= 0; });
}
Action Gameplay::PlaceCampfire(World& world, Save& save) {
  if (save.inventory.campfires <= 0) {
    return {false, "Craft a campfire with C first"};
  }
  if (save.campfires.size() >= 64) {
    return {false, "Campfire limit reached (64)"};
  }
  Vec position = save.player + facing * 45;
  if (TerrainAt(position, save.seed) == Terrain::kShallow ||
      TerrainAt(position, save.seed) == Terrain::kDeep ||
      !world.Clear(position, save, 18)) {
    return {false, "Campfires need a clear, dry spot"};
  }
  for (auto fire : save.campfires) {
    if (Length(fire - position) < 100) {
      return {false, "There is already a campfire nearby"};
    }
  }
  save.inventory.campfires--;
  save.campfires.push_back(position);
  return {true, "Campfire placed — rest nearby to heal"};
}
void Gameplay::Tick(World& world, Save& save, float dt, double time) {
  int defeat_captor = -1;
  EnsureCityState(save);
  InitializeTowns(save);
  TickTowns(save, dt);
  TickNation(save, dt);
  SyncNationActors(world, save);
  world.SyncInfrastructure(save);
  TickResidents(save, dt);
  bool sanctuary = false;
  defender_clock_ += dt;
  if (defender_clock_ >= 1.5f) {
    defender_clock_ = std::fmod(defender_clock_, 1.5f);
    for (auto& npc : world.npcs) {
      if (Length(npc.pos - save.player) < 1500) {
        auto defender = GetCitizen(save, npc.id);
        if (!defender || defender->state != "active" ||
            defender->job != "guard" || !defender->trained ||
            defender->weapon.empty() || !OwnsCity(save, defender->city)) {
          continue;
        }
        bool struck = false;
        for (auto& [coord, chunk] : world.chunks) {
          for (auto& enemy : chunk.enemies) {
            if ((!enemy.kind ||
                 save.relations[enemy.city].treaty == Treaty::kWar) &&
                !save.defeated.contains(enemy.id) &&
                Length(enemy.pos - npc.pos) < 180) {
              Damage(world, save, enemy,
                     std::max(1, Weapon(defender->weapon).damage / 2),
                     Normalized(enemy.pos - npc.pos));
              struck = true;
              break;
            }
          }
          if (struck) {
            break;
          }
        }
      }
    }
  }
  for (size_t i = 0; i < Settlements().size(); i++) {
    if (save.relations[i].treaty != Treaty::kWar &&
        Length(Settlements()[i].pos - save.player) < 370) {
      sanctuary = true;
    }
  }
  for (auto fire : save.campfires) {
    if (Length(fire - save.player) < 130) {
      sanctuary = true;
    }
  }
  for (auto& [coord, chunk] : world.chunks) {
    for (auto& e : chunk.enemies) {
      if (save.defeated.contains(e.id)) {
        continue;
      }
      e.previous = e.pos;
      e.flash = std::max(0.f, e.flash - dt);
      e.cooldown = std::max(0.f, e.cooldown - dt);
      if (Length(e.home - save.player) > 1300) {
        continue;
      }
      bool hostile = e.kind ? save.relations.at(e.city).treaty == Treaty::kWar
                            : !sanctuary;
      auto terrain = TerrainAt(save.player, save.seed);
      if ((save.nation && !HasControlled(save)) || save.submerged ||
          (!BridgeAt(save, save.player) &&
           (terrain == Terrain::kDeep ||
            (terrain == Terrain::kShallow && Watercraft(save))))) {
        hostile = false;
      }
      if (e.windup <= 0) {
        e.target_npc = -1;
        float nearest = Length(e.pos - save.player);
        for (const auto& npc : world.npcs) {
          auto c = GetCitizen(save, npc.id);
          float d = Length(npc.pos - e.pos);
          if (c && (!save.nation || c->npc != save.controlled_npc) &&
              c->state == "active" && c->job == "guard" && c->trained &&
              OwnsCity(save, c->city) && d < std::min(180.f, nearest) &&
              (!e.kind || save.relations[e.city].treaty == Treaty::kWar)) {
            nearest = d;
            e.target_npc = npc.id;
          }
        }
      }
      Vec victim = save.player;
      if (e.target_npc >= 0) {
        auto c = GetCitizen(save, e.target_npc);
        if (c && c->state == "active") {
          auto npc =
              std::find_if(world.npcs.begin(), world.npcs.end(),
                           [&](const auto& n) { return n.id == e.target_npc; });
          if (npc != world.npcs.end()) {
            victim = npc->pos;
            hostile = !e.kind || save.relations[e.city].treaty == Treaty::kWar;
          }
        } else {
          e.target_npc = -1;
          e.windup = 0;
        }
      }
      float distance = Length(e.pos - victim);
      bool chase = hostile && distance < 360;
      Vec target = e.home + Vec{std::sin(static_cast<float>(time) * .4f +
                                         static_cast<float>(e.id % 31)) *
                                    45,
                                std::cos(static_cast<float>(time) * .3f +
                                         static_cast<float>(e.id % 17)) *
                                    45};
      if (chase) {
        target = victim;
      }
      bool avoid_fire = false;
      if (!e.kind) {
        for (auto fire : save.campfires) {
          if (Length(e.pos - fire) < 180) {
            target = e.pos + Normalized(e.pos - fire) * 100;
            chase = false;
            avoid_fire = true;
            break;
          }
        }
      }
      if (!chase && !avoid_fire && Length(e.pos - e.home) > 70) {
        target = e.home;
      }
      if (e.windup > 0) {
        e.windup -= dt;
        if (e.windup <= 0) {
          e.cooldown = 1.3f;
          e.pos =
              world.Move(e.pos,
                         Normalized(e.attack_target - e.pos) *
                             std::min(24.f, Length(e.attack_target - e.pos)),
                         save, 10);
          if (chase && e.target_npc >= 0 && Length(e.pos - victim) < 38) {
            auto c = GetCitizen(save, e.target_npc);
            if (c && c->state == "active") {
              float taken = c->outfit.empty() ? 1 : Outfit(c->outfit).taken;
              c->health -= std::max(
                  1, static_cast<int>(std::lround((e.kind ? 18 : 12) * taken)));
              if (c->health <= 0) {
                bool captured = e.kind && Hash(save.seed ^ e.id ^
                                               static_cast<uint64_t>(c->npc)) %
                                                  100 <
                                              40;
                notice = LoseResident(save, c->npc, captured, e.city).message;
              }
            }
          }
          if (chase && e.target_npc < 0 && Length(e.pos - save.player) < 38 &&
              invulnerable <= 0) {
            int amount =
                save.inventory.armor ? (e.kind ? 11 : 7) : (e.kind ? 18 : 12);
            if (save.adventure) {
              amount = std::max(
                  1, static_cast<int>(std::lround((e.kind ? 18 : 12) *
                                                  PlayerCharacter(save).taken *
                                                  PlayerOutfit(save).taken)));
            }
            save.health -= amount;
            if (save.nation && save.health <= 0 && e.kind &&
                Hash(save.seed ^ e.id ^
                     static_cast<uint64_t>(save.controlled_npc)) %
                        100 <
                    40) {
              defeat_captor = e.city;
            }
            damage_taken += amount;
            invulnerable = .65f;
            notice = e.kind ? "A guard hit you!"
                            : "Wolf bite! Sprint with SHIFT to dodge.";
            save.player =
                world.Move(save.player, Normalized(save.player - e.pos) * 18,
                           save, 12, Watercraft(save));
          }
        }
      } else if (chase && distance < 58 && e.cooldown <= 0) {
        e.windup = .4f;
        e.attack_target = victim;
      } else {
        Vec delta = target - e.pos;
        float travel =
            chase ? std::max(0.f, Length(delta) - 42) : Length(delta);
        if (!save.submerged) {
          e.pos = world.Move(
              e.pos,
              Normalized(delta) *
                  std::min(travel,
                           dt * (chase ? (e.kind ? 135.f : 125.f) : 36.f)),
              save, 10);
        }
        if (e.kind && chase && travel > 5 && Length(e.pos - e.previous) < .5f &&
            e.cooldown <= 0) {
          for (auto& wall : save.walls) {
            if (wall.hp > 0) {
              Vec line = wall.b - wall.a, relative = e.pos - wall.a;
              float length2 = line.x * line.x + line.y * line.y;
              float fraction =
                  std::clamp((relative.x * line.x + relative.y * line.y) /
                                 std::max(1.f, length2),
                             0.f, 1.f);
              if (Length(e.pos - (wall.a + line * fraction)) < 35) {
                wall.hp = std::max(0, wall.hp - 18);
                e.cooldown = 1;
                notice = "Guards are attacking the wall!";
                break;
              }
            }
          }
        }
      }
      e.walk = std::fmod(e.walk + Length(e.pos - e.previous) / 10, 4.f);
    }
  }
  if (save.nation && save.health <= 0) {
    StoreControlled(save);
    notice = LoseResident(save, save.controlled_npc, defeat_captor >= 0,
                          defeat_captor)
                 .message;
    save.controlled_npc = -1;
    save.inventory = {};
    save.health = 100;
    save.submerged = false;
    ++respawns;
    return;
  }
  if (save.health <= 0) {
    int nearest = 0;
    float best = 1e9;
    for (size_t i = 0; i < Settlements().size(); i++) {
      if (save.relations[i].treaty != Treaty::kWar &&
          Length(Settlements()[i].pos - save.player) < best) {
        best = Length(Settlements()[i].pos - save.player);
        nearest = static_cast<int>(i);
      }
    }
    DestroyPlayerWeapon(save);
    save.player = Settlements()[nearest].pos + Vec{120, 140};
    save.health = 100;
    save.coins -= save.coins / 5;
    save.submerged = false;
    invulnerable = 3;
    respawns++;
    notice =
        "You woke up in a friendly town. Lost equipped weapon and 20% of "
        "coins.";
    return;
  }
  if (save.submerged) {
    save.oxygen = std::max(
        0.f, save.oxygen - dt * (save.outfit_id == "diver" ? .75f : 1.f));
    auto cell = MapCell(save.player);
    if (ObjectId(cell.x, cell.y, save.seed) % 31 == 0) {
      pressure_clock_ += dt;
      if (pressure_clock_ >= 2) {
        save.hull = std::max(1, save.hull - 3);
        pressure_clock_ = 0;
        notice = "Dangerous reefs: hull damaged. Use the repair action.";
      }
    } else {
      pressure_clock_ = 0;
    }
    if (save.oxygen <= 0 || save.hull <= 5) {
      save.submerged = false;
      save.oxygen = 5;
      notice = "Emergency ascent. Refill air and repair the hull.";
    }
  } else {
    save.oxygen =
        std::min(save.inventory.oxygen ? 200.f : 100.f, save.oxygen + dt * 6);
  }
  bool heal = false;
  int rate = 3;
  for (size_t i = 0; i < Settlements().size(); i++) {
    if (!save.nation && save.relations[i].treaty != Treaty::kWar &&
        Length(Settlements()[i].pos - save.player) < 160) {
      heal = true;
    }
  }
  for (auto fire : save.campfires) {
    if (Length(fire - save.player) < 110 && !save.submerged) {
      heal = true;
      rate = 8;
    }
  }
  if (heal) {
    regeneration_ += dt;
    if (regeneration_ >= 1) {
      save.health = std::min(100, save.health + rate);
      regeneration_ -= 1;
    }
  } else {
    regeneration_ = 0;
  }
  civic_clock_ += dt;
  if (civic_clock_ >= 20) {
    civic_clock_ -= 20;
    for (size_t i = 0;
         i < std::min(save.relations.size(), save.treasury.size()); i++) {
      if (save.relations[i].treaty == Treaty::kUnion && save.treasury[i] > 0 &&
          save.coins < 10000000) {
        save.treasury[i]--;
        save.coins++;
      }
    }
  }
}
}  // namespace frontier
