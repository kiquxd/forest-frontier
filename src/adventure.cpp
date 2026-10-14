#include "adventure.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "progress.hpp"
#include "settlement_system.hpp"

namespace frontier {
const CharacterDef& PlayerCharacter(const Save& save) {
  return Character(save.character_id);
}
const OutfitDef& PlayerOutfit(const Save& save) {
  return Outfit(save.outfit_id);
}
float GatherTimeMultiplier(const Save& save) {
  return PlayerCharacter(save).gather * PlayerOutfit(save).gather;
}
Citizen* GetCitizen(Save& save, int id) {
  for (auto& resident : save.citizens) {
    if (resident.npc == id) {
      return &resident;
    }
  }
  return nullptr;
}
const Citizen* GetCitizen(const Save& save, int id) {
  for (auto& resident : save.citizens) {
    if (resident.npc == id) {
      return &resident;
    }
  }
  return nullptr;
}
void SyncCitizens(Save& save) {
  if (!save.campaign) {
    return;
  }
  for (auto& npc : MakeNpcs()) {
    if (OwnsCity(save, npc.settlement) && !npc.ruler && npc.origin_city < 0 &&
        !GetCitizen(save, npc.id)) {
      save.citizens.push_back({npc.id, npc.settlement, "idle", "", "", false});
    }
  }
}
void InitializeAdventure(Save& save) {
  if (!save.adventure) {
    for (auto& id : save.inventory.weapons) {
      save.weapon_copies[id] = 1;
    }
    if (save.inventory.sword && save.inventory.weapons.empty()) {
      save.inventory.weapons.insert("wood_sword");
      save.weapon_copies["wood_sword"] = 1;
    }
    save.outfit_copies["traveler"] = 1;
    save.outfit_id = "traveler";
    if (save.inventory.armor) {
      save.outfit_copies["leather_armor"] = 1;
      save.outfit_id = "leather_armor";
    }
    save.adventure = true;
  }
  auto npcs = MakeNpcs();
  for (auto& [id, q] : save.quests) {
    if (id < 0 || id >= static_cast<int>(npcs.size())) {
      throw std::runtime_error("Quest giver no longer exists");
    }
  }
  for (auto& resident : save.citizens) {
    if (resident.npc < 0 || resident.npc >= static_cast<int>(npcs.size()) ||
        npcs[resident.npc].settlement != resident.city ||
        npcs[resident.npc].ruler || npcs[resident.npc].origin_city >= 0) {
      throw std::runtime_error("Invalid saved resident");
    }
  }
  SyncCitizens(save);
  InitializeTowns(save);
}
int AvailableWeapon(const Save& save, const std::string& id, int excluded) {
  if (id.empty()) {
    return 1000000;
  }
  auto found = save.weapon_copies.find(id);
  int count = found == save.weapon_copies.end()
                  ? (save.inventory.weapons.contains(id) ? 1 : 0)
                  : found->second;
  for (auto& resident : save.citizens) {
    if (resident.state != "dead" && resident.npc != excluded &&
        resident.weapon == id) {
      count--;
    }
  }
  return count;
}
int AvailableOutfit(const Save& save, const std::string& id, int excluded) {
  if (id.empty()) {
    return 1000000;
  }
  auto found = save.outfit_copies.find(id);
  int count = found == save.outfit_copies.end() ? 0 : found->second;
  for (auto& resident : save.citizens) {
    if (resident.state != "dead" && resident.npc != excluded &&
        resident.outfit == id) {
      count--;
    }
  }
  return count;
}
Action EquipOutfit(Save& save, const std::string& id) {
  if (AvailableOutfit(save, id) < 1) {
    return {false, "No free copy of that outfit"};
  }
  save.outfit_id = id;
  save.inventory.armor = PlayerOutfit(save).taken < 1;
  return {true, "Wearing: " + PlayerOutfit(save).name};
}
std::vector<std::string> OwnedOutfits(const Save& save) {
  std::vector<std::string> out;
  for (auto& item : GetContent().outfits) {
    if (save.outfit_copies.contains(item.id) &&
        save.outfit_copies.at(item.id) > 0) {
      out.push_back(item.id);
    }
  }
  return out;
}
bool RecipeKnown(const Save& save, const Recipe& r) {
  if (!save.adventure) {
    return true;
  }
  if (r.quest_only && !save.known_recipes.contains(r.id)) {
    return false;
  }
  if (!r.result.starts_with("outfit:")) {
    return true;
  }
  auto id = r.result.substr(7);
  const auto& item = Outfit(id);
  return !item.quest_only || save.known_recipes.contains(r.id);
}
const QuestDef* NpcQuest(const Save& save, const Npc& npc) {
  if (auto found = save.quests.find(npc.id); found != save.quests.end()) {
    return &Quest(found->second.id);
  }
  const auto& role = GetContent().npcs[npc.type].id;
  bool teacher =
      role == GetContent().towns[Settlements()[npc.settlement].type].teacher;
  if (!teacher && !npc.ruler &&
      Hash(save.seed ^ static_cast<uint64_t>(npc.id) * 0x8391ULL) % 100 >= 45) {
    return nullptr;
  }
  std::string quest_role =
      npc.ruler || role == "elder" || role == "admiral" ? "diplomat"
      : role == "archivist" || role == "navigator"      ? "scholar"
      : role == "engineer"                              ? "carpenter"
                                                        : role;
  for (auto& quest : GetContent().quests) {
    if (quest.role == quest_role) {
      return &quest;
    }
  }
  return &Quest("supplies");
}
std::string NpcIntroduction(const Save& save, const Npc& npc) {
  std::string out = "I'm " + npc.name + ", " + npc.role + " of " +
                    Settlements()[npc.settlement].name + ". " + npc.personality;
  if (npc.origin_city >= 0) {
    out += " I came here from " + Settlements()[npc.origin_city].name + ".";
  }
  if (OwnsCity(save, npc.settlement) && !npc.ruler && npc.origin_city < 0) {
    out += " This is our town. You can train and equip me for a job.";
  }
  if (!NpcQuest(save, npc)) {
    out +=
        " I have no task for you today, but you can still trade or meet our "
        "town master.";
  }
  return out;
}
namespace {
int ObjectiveValue(const Save& save, const QuestDef& quest) {
  if (quest.objective == "deliver") {
    return GoodCount(save.inventory, quest.good);
  }
  if (quest.objective == "craft") {
    return save.crafted;
  }
  if (quest.objective == "kills") {
    return save.wolf_kills;
  }
  if (quest.objective == "explore") {
    return static_cast<int>(save.explored.size());
  }
  if (quest.objective == "discover") {
    int count = 0;
    for (auto& r : save.relations) {
      if (r.discovered) {
        count++;
      }
    }
    return count;
  }
  return 0;
}
}  // namespace

int GetQuestProgress(const Save& save, const QuestProgress& progress) {
  auto& quest = Quest(progress.id);
  return std::max(0,
                  ObjectiveValue(save, quest) -
                      (quest.objective == "deliver" ? 0 : progress.baseline));
}
std::string QuestStatus(const Save& save, const Npc& npc) {
  auto quest = NpcQuest(save, npc);
  if (!quest) {
    return "No quest available.";
  }
  auto found = save.quests.find(npc.id);
  std::string status =
      found == save.quests.end() ? "Available"
      : found->second.completed
          ? "Completed"
          : std::to_string(std::min(quest->target,
                                    GetQuestProgress(save, found->second))) +
                " / " + std::to_string(quest->target);
  std::string reward = std::to_string(quest->coins) + " coins";
  if (!quest->recipe.empty()) {
    for (auto& r : Recipes()) {
      if (r.id == quest->recipe) {
        reward += " + recipe: " + r.name;
      }
    }
  }
  if (!quest->knowledge.empty()) {
    reward += " + " + quest->knowledge;
  }
  if (!quest->weapon.empty()) {
    reward += " + " + Weapon(quest->weapon).name;
  }
  if (!quest->outfit.empty()) {
    reward += " + " + Outfit(quest->outfit).name;
  }
  for (int i = 0; i < kGoodTypes; i++) {
    if (quest->rewards[i]) {
      reward += " + " + std::to_string(quest->rewards[i]) + " " + GoodName(i);
    }
  }
  return quest->title + " · " + status + "\n" + quest->lore +
         "\nReward: " + reward;
}
Action AcceptQuest(Save& save, const Npc& npc) {
  auto quest = NpcQuest(save, npc);
  if (!quest) {
    return {false, "This resident has no quest"};
  }
  if (save.quests.contains(npc.id)) {
    return {false, "Quest already accepted or completed"};
  }
  if (save.relations[npc.settlement].treaty == Treaty::kWar) {
    return {false, "Negotiate a truce before accepting tasks"};
  }
  save.quests[npc.id] = {
      quest->id, false,
      quest->objective == "deliver" ? 0 : ObjectiveValue(save, *quest)};
  save.tracked_quest = npc.id;
  RecordHistory(save, "quest_accepted", quest->title, npc.settlement);
  return {true, "Quest accepted: " + quest->title};
}
Action FinishQuest(Save& save, const Npc& npc) {
  auto found = save.quests.find(npc.id);
  if (found == save.quests.end() || found->second.completed) {
    return {false, "No active quest to turn in"};
  }
  auto& progress = found->second;
  auto& quest = Quest(progress.id);
  if (save.relations[npc.settlement].treaty == Treaty::kWar) {
    return {false, "This town is at war with you"};
  }
  if (GetQuestProgress(save, progress) < quest.target) {
    return {false, "Quest requirements are not met"};
  }
  if (quest.objective == "deliver") {
    GoodCount(save.inventory, quest.good) -= quest.target;
  }
  save.coins = std::min(10000000, save.coins + quest.coins);
  for (int i = 0; i < kGoodTypes; i++) {
    GoodCount(save.inventory, i) =
        std::min(1000000, GoodCount(save.inventory, i) + quest.rewards[i]);
  }
  if (!quest.recipe.empty()) {
    save.known_recipes.insert(quest.recipe);
  }
  if (!quest.knowledge.empty()) {
    save.technologies.insert(quest.knowledge);
  }
  if (!quest.weapon.empty()) {
    save.inventory.weapons.insert(quest.weapon);
    save.weapon_copies[quest.weapon]++;
  }
  if (!quest.outfit.empty()) {
    save.outfit_copies[quest.outfit]++;
  }
  progress.completed = true;
  if (save.tracked_quest == npc.id) {
    save.tracked_quest = -1;
  }
  save.relations[npc.settlement].opinion =
      std::min(100, save.relations[npc.settlement].opinion + 5);
  if (save.campaign && !OwnsCity(save, npc.settlement)) {
    save.city_opinions[save.home_city][npc.settlement] =
        save.city_opinions[npc.settlement][save.home_city] =
            save.relations[npc.settlement].opinion;
  }
  RememberResources(save);
  RecordHistory(save, "quest_completed", quest.title, npc.settlement);
  return {true, "Quest completed: " + quest.title + " · rewards received"};
}
int AverageTrust(const Save& save) {
  int sum = 0, count = 0;
  for (int i = 0; i < static_cast<int>(save.relations.size()); i++) {
    if (save.relations[i].discovered && !OwnsCity(save, i)) {
      sum += save.relations[i].opinion;
      count++;
    }
  }
  return count ? sum / count : 0;
}
const std::vector<ProgressNode>& ResearchNodes() {
  static const std::vector<ProgressNode> kNodes = {
      {"apprenticeship", "Apprenticeship", "Train gatherers and craftspeople",
       15, 4, 3, 1, 0, 1, 0, 1, "crafting"},
      {"militia", "Militia training",
       "Train equipped defenders; strategic war remains a placeholder", 20, 4,
       4, 1, 1, 1, 0, 1, "combat"},
      {"commerce_school", "Commerce school",
       "Train traders and earn town treasury income", 20, 4, 4, 1, 0, 1, 10, 1,
       "trading"},
      {"navigation_school", "Navigation school",
       "Train fishers and improve town supplies", 25, 4, 5, 1, 0, 1, 0, 1,
       "sailing"},
      {"citizen_engineering", "Engineering school",
       "Train scholars and engineers", 40, 6, 7, 2, 1, 1, 15, 2, "engineering"},
      {"diplomatic_corps", "Diplomatic corps",
       "A foundation for multi-town leadership", 50, 8, 7, 2, 2, 2, 30, 2,
       "civic_charter", "diplomacy"},
      {"guild_charter", "Guild charter",
       "Skilled residents produce larger work batches", 35, 4, 6, 2, 1, 1, 20,
       2, "apprenticeship"}};
  return kNodes;
}
namespace {
int CountClothes(const Save& save) {
  int count = 0;
  for (auto& [id, n] : save.outfit_copies) {
    if (id != "traveler") {
      count += n;
    }
  }
  return count;
}
int CountWeapons(const Save& save) {
  int count = 0;
  for (auto& [id, n] : save.weapon_copies) {
    count += n;
  }
  return count;
}
int CompletedQuests(const Save& save) {
  int count = 0;
  for (auto& [id, q] : save.quests) {
    if (q.completed) {
      count++;
    }
  }
  return count;
}
}  // namespace

std::string ResearchRequirements(const Save& save, const ProgressNode& n) {
  std::ostringstream out;
  auto add = [&](const char* name, int have, int need) {
    if (need) {
      out << name << " " << have << "/" << need << " · ";
    }
  };
  add("Coins", save.coins, n.coins);
  add("Residents", ActiveResidents(save), n.residents);
  add("Skills", static_cast<int>(save.skills.size()), n.skills);
  add("Clothes", CountClothes(save), n.clothes);
  add("Weapons", CountWeapons(save), n.weapons);
  add("Towns",
      static_cast<int>(std::count(save.owners.begin(), save.owners.end(), 0)),
      n.cities);
  add("Avg. foreign trust", AverageTrust(save), n.trust);
  add("Quests", CompletedQuests(save), n.quests);
  out << "Needs "
      << (std::string(TechnologyName(n.prerequisite)).empty()
              ? SkillName(n.prerequisite)
              : TechnologyName(n.prerequisite));
  if (!n.second_prerequisite.empty()) {
    out << " + " << SkillName(n.second_prerequisite);
  }
  if (!FacilityAccess(save, CityAt(save, save.player), "school")) {
    out << " · Requires a local or road-connected school";
  }
  return out.str();
}
bool ResearchReady(const Save& save, const ProgressNode& n) {
  return FacilityAccess(save, CityAt(save, save.player), "school") &&
         !save.technologies.contains(n.id) && save.coins >= n.coins &&
         ActiveResidents(save) >= n.residents &&
         static_cast<int>(save.skills.size()) >= n.skills &&
         CountClothes(save) >= n.clothes && CountWeapons(save) >= n.weapons &&
         static_cast<int>(std::count(save.owners.begin(), save.owners.end(),
                                     0)) >= n.cities &&
         AverageTrust(save) >= n.trust && CompletedQuests(save) >= n.quests &&
         (save.skills.contains(n.prerequisite) ||
          save.technologies.contains(n.prerequisite)) &&
         (n.second_prerequisite.empty() ||
          save.skills.contains(n.second_prerequisite) ||
          save.technologies.contains(n.second_prerequisite));
}
Action Research(Save& save, int index) {
  if (index < 0 || index >= static_cast<int>(ResearchNodes().size())) {
    return {false, "Unknown research"};
  }
  auto& n = ResearchNodes()[index];
  if (save.technologies.contains(n.id)) {
    return {false, "Already researched"};
  }
  if (!ResearchReady(save, n)) {
    return {false, ResearchRequirements(save, n)};
  }
  save.coins -= n.coins;
  save.technologies.insert(n.id);
  RecordHistory(save, "research", "Researched " + n.name);
  return {true, "Researched: " + n.name};
}
const std::vector<std::string>& ResidentJobs() {
  static const std::vector<std::string> kJobs = {
      "idle",   "gatherer", "crafter",  "guard",     "trader",
      "fisher", "scholar",  "engineer", "ambassador"};
  return kJobs;
}
Action AssignCitizen(Save& save, int id, const std::string& job,
                     const std::string& weapon_id,
                     const std::string& outfit_id) {
  auto resident = GetCitizen(save, id);
  if (!resident || resident->state != "active" ||
      !OwnsCity(save, resident->city)) {
    return {false, "This is not your resident"};
  }
  if (std::find(ResidentJobs().begin(), ResidentJobs().end(), job) ==
      ResidentJobs().end()) {
    return {false, "Unknown job"};
  }
  if (!weapon_id.empty()) {
    (void)Weapon(weapon_id);
  }
  if (!outfit_id.empty()) {
    (void)Outfit(outfit_id);
  }
  if (AvailableWeapon(save, weapon_id, id) -
              (save.inventory.weapon_id == weapon_id && !weapon_id.empty()
                   ? 1
                   : 0) <
          1 ||
      AvailableOutfit(save, outfit_id, id) -
              (save.outfit_id == outfit_id && !outfit_id.empty() ? 1 : 0) <
          1) {
    return {false, "Gear is in use. Unequip it or craft another copy"};
  }
  std::string tech = job == "gatherer" || job == "crafter" ? "apprenticeship"
                     : job == "guard"                      ? "militia"
                     : job == "trader"                     ? "commerce_school"
                     : job == "fisher"                     ? "navigation_school"
                     : job == "scholar" || job == "engineer"
                         ? "citizen_engineering"
                     : job == "ambassador" ? "diplomatic_corps"
                                           : "";
  if (!tech.empty() && !save.technologies.contains(tech)) {
    return {false, "Training required: " + tech};
  }
  if (job != "idle" && !FacilityAccess(save, resident->city, "school")) {
    return {false, "Training requires a local or road-connected school"};
  }
  if (job != "idle" && (outfit_id.empty() || Outfit(outfit_id).job != job)) {
    return {false, "This job needs matching work clothes"};
  }
  if (job == "guard" && weapon_id.empty()) {
    return {false, "Defenders need a weapon"};
  }
  if (job == "guard" && Weapon(weapon_id).kind == "bow") {
    return {false,
            "Local defenders need a melee weapon; ranged troops are not "
            "implemented"};
  }
  resident->job = job;
  resident->weapon = weapon_id;
  resident->outfit = outfit_id;
  resident->trained = job != "idle";
  return {true, "Resident assigned: " + job};
}
void TickResidents(Save& save, float dt) {
  if (!save.adventure) {
    return;
  }
  save.work_clock += dt;
  if (save.work_clock < 20) {
    return;
  }
  save.work_clock = std::fmod(save.work_clock, 20.f);
  int batch = save.technologies.contains("guild_charter") ? 2 : 1;
  for (auto& n : save.citizens) {
    if (n.state == "active" && OwnsCity(save, n.city) && n.trained &&
        save.relations[n.city].treaty != Treaty::kWar) {
      int good = n.job == "gatherer"   ? 0
                 : n.job == "crafter"  ? 1
                 : n.job == "fisher"   ? 4
                 : n.job == "scholar"  ? 2
                 : n.job == "engineer" ? 3
                                       : -1;
      std::string facility = n.job == "crafter"    ? "workbench"
                             : n.job == "scholar"  ? "school"
                             : n.job == "engineer" ? "forge"
                             : n.job == "fisher"   ? "shipyard"
                                                   : "workbench";
      if (good >= 0 && FacilityAccess(save, n.city, facility)) {
        save.stocks[n.city][good] =
            std::min(1000000, save.stocks[n.city][good] + batch);
      }
      if (n.job == "ambassador") {
        int partner = -1;
        for (int city = 0; city < static_cast<int>(save.relations.size());
             city++) {
          if (!OwnsCity(save, city) && save.relations[city].discovered &&
              save.relations[city].treaty != Treaty::kWar &&
              save.relations[city].opinion < 50 &&
              (partner < 0 || save.relations[city].opinion <
                                  save.relations[partner].opinion)) {
            partner = city;
          }
        }
        if (partner >= 0) {
          save.relations[partner].opinion++;
          if (save.campaign) {
            save.city_opinions[save.home_city][partner] =
                save.city_opinions[partner][save.home_city] =
                    save.relations[partner].opinion;
          }
        }
      }
      if (n.job == "trader") {
        save.treasury[n.city] =
            std::min(10000000, save.treasury[n.city] + batch * 2);
      }
    }
  }
}
}  // namespace frontier
