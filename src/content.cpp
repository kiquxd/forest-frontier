#include "content.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "json.hpp"

namespace frontier {
namespace {
std::vector<nlohmann::json> Files(const std::string& folder) {
  std::vector<std::filesystem::path> paths;
  for (auto& file : std::filesystem::directory_iterator(
           std::filesystem::path(FRONTIER_ROOT) / "content" / folder)) {
    if (file.path().extension() == ".json") {
      paths.push_back(file.path());
    }
  }
  std::sort(paths.begin(), paths.end());
  std::vector<nlohmann::json> out;
  std::unordered_set<std::string> ids;
  for (auto& p : paths) {
    std::ifstream f(p);
    nlohmann::json json;
    f >> json;
    if (!ids.insert(json.at("id").get<std::string>()).second) {
      throw std::runtime_error("Duplicate content ID: " + p.string());
    }
    out.push_back(std::move(json));
  }
  if (out.empty()) {
    throw std::runtime_error("Empty content directory: " + folder);
  }
  return out;
}
std::array<int, kGoodTypes> Costs(const nlohmann::json& json) {
  std::array<int, kGoodTypes> out{};
  for (auto it = json.begin(); it != json.end(); ++it) {
    int index = std::stoi(it.key()), amount = it.value().get<int>();
    if (index < 0 || index >= kGoodTypes || amount < 0 || amount > 10000) {
      throw std::runtime_error("Invalid recipe cost");
    }
    out[index] = amount;
  }
  return out;
}
Content Load() {
  Content c;
  for (auto& json : Files("npcs")) {
    c.npcs.push_back({json.at("id"), json.at("role"), json.at("personality"),
                      json.at("weight"), json.at("rarity"),
                      json.at("color").get<std::array<int, 3>>(),
                      json.at("names").get<std::vector<std::string>>()});
  }
  for (auto& json : Files("weapons")) {
    c.weapons.push_back({json.at("id"), json.at("name"), json.at("kind"),
                         json.at("technology"), json.at("damage"),
                         json.at("rarity"), json.at("weight"), json.at("reach"),
                         json.at("duration"), Costs(json.at("cost"))});
  }
  for (auto& json : Files("settlements")) {
    c.towns.push_back({json.at("id"), json.at("name"), json.at("teacher"),
                       json.at("technology"), json.at("weight"),
                       json.at("rarity"), json.at("attitude"),
                       json.at("color").get<std::array<int, 3>>(),
                       json.at("goods").get<std::vector<int>>()});
  }
  auto all = Files("recipes");
  const std::vector<std::string> order = {
      "axe",   "pickaxe",   "sword",    "rod",    "boat",
      "armor", "potion",    "campfire", "arrows", "ship",
      "steel", "submarine", "sonar",    "oxygen", "repair"};
  for (auto& id : order) {
    auto it = std::find_if(all.begin(), all.end(),
                           [&](auto& j) { return j.at("id") == id; });
    if (it == all.end()) {
      throw std::runtime_error("Missing core recipe: " + id);
    }
    auto& json = *it;
    c.recipes.push_back({json.at("name"), Costs(json.at("cost")),
                         json.at("effect"), json.at("id"), json.at("result"),
                         json.at("technology")});
  }
  for (auto& weapon : c.weapons) {
    if (weapon.id != "wood_sword") {
      c.recipes.push_back(
          {weapon.name, weapon.cost,
           "Weapon: damage " + std::to_string(weapon.damage) + ", reach " +
               std::to_string(static_cast<int>(weapon.reach)),
           "weapon_" + weapon.id, "weapon:" + weapon.id, weapon.technology});
    }
  }
  for (auto& json : all) {
    if (std::find(order.begin(), order.end(),
                  json.at("id").get<std::string>()) == order.end()) {
      c.recipes.push_back({json.at("name"), Costs(json.at("cost")),
                           json.at("effect"), json.at("id"), json.at("result"),
                           json.at("technology"),
                           json.value("quest_only", false)});
    }
  }
  for (auto& json : Files("characters")) {
    c.characters.push_back({json.at("id"), json.at("name"), json.at("lore"),
                            json.at("pros"), json.at("cons"), json.at("speed"),
                            json.at("gather"), json.at("damage"),
                            json.at("taken"), json.at("trade"),
                            json.at("coins")});
  }
  for (auto& json : Files("outfits")) {
    c.outfits.push_back({json.at("id"), json.at("name"), json.at("description"),
                         json.at("technology"), json.at("job"),
                         json.at("speed"), json.at("gather"), json.at("taken"),
                         json.at("trade"), Costs(json.at("cost")),
                         json.at("color").get<std::array<int, 3>>(),
                         json.at("quest_only")});
  }
  for (auto& json : Files("quests")) {
    c.quests.push_back({json.at("id"), json.at("title"), json.at("lore"),
                        json.at("objective"), json.at("knowledge"),
                        json.at("recipe"), json.at("weapon"), json.at("outfit"),
                        json.at("role"), json.at("good"), json.at("target"),
                        json.at("coins"), Costs(json.at("rewards"))});
  }
  for (auto& outfit : c.outfits) {
    if (outfit.id != "traveler") {
      c.recipes.push_back({outfit.name, outfit.cost, outfit.description,
                           "outfit_" + outfit.id, "outfit:" + outfit.id,
                           outfit.technology});
    }
  }
  for (auto& n : c.npcs) {
    if (n.weight < 1 || n.rarity < 0 || n.rarity > 3 || n.names.empty()) {
      throw std::runtime_error("Invalid NPC: " + n.id);
    }
  }
  for (auto& weapon : c.weapons) {
    if (weapon.weight < 1 || weapon.damage < 1 || weapon.reach < 20 ||
        weapon.duration < .2f || weapon.rarity < 0 || weapon.rarity > 3) {
      throw std::runtime_error("Invalid weapon: " + weapon.id);
    }
  }
  for (auto& t : c.towns) {
    if (t.weight < 1 || t.rarity < 0 || t.rarity > 3 || t.attitude < -100 ||
        t.attitude > 100) {
      throw std::runtime_error("Invalid town: " + t.id);
    }
  }
  return c;
}
}  // namespace

const Content& GetContent() {
  static const Content kContent = Load();
  return kContent;
}
const WeaponDef& Weapon(const std::string& id) {
  auto& list = GetContent().weapons;
  auto it = std::find_if(list.begin(), list.end(),
                         [&](auto& w) { return w.id == id; });
  if (it == list.end()) {
    throw std::runtime_error("Unknown weapon: " + id);
  }
  return *it;
}
int NpcType(const std::string& id) {
  auto& v = GetContent().npcs;
  for (size_t i = 0; i < v.size(); i++) {
    if (v[i].id == id) {
      return static_cast<int>(i);
    }
  }
  throw std::runtime_error("Unknown NPC type: " + id);
}
int TownType(const std::string& id) {
  auto& v = GetContent().towns;
  for (size_t i = 0; i < v.size(); i++) {
    if (v[i].id == id) {
      return static_cast<int>(i);
    }
  }
  throw std::runtime_error("Unknown town type: " + id);
}
template <class T>
static int Choose(const std::vector<T>& v, uint64_t n) {
  int total = 0;
  for (auto& item : v) {
    total += item.weight;
  }
  int roll = static_cast<int>(n % static_cast<uint64_t>(total));
  for (size_t i = 0; i < v.size(); i++) {
    roll -= v[i].weight;
    if (roll < 0) {
      return static_cast<int>(i);
    }
  }
  return 0;
}
int WeightedNpc(uint64_t n) {
  return Choose(GetContent().npcs, n);
}
int WeightedTown(uint64_t n) {
  return Choose(GetContent().towns, n);
}
const WeaponDef& WeightedWeapon(uint64_t n) {
  return GetContent().weapons[Choose(GetContent().weapons, n)];
}
namespace {
template <class T>
const T& FindDef(const std::vector<T>& list, const std::string& id) {
  for (auto& item : list) {
    if (item.id == id) {
      return item;
    }
  }
  throw std::runtime_error("Unknown content ID: " + id);
}
}  // namespace

const CharacterDef& Character(const std::string& id) {
  return FindDef(GetContent().characters, id);
}
const OutfitDef& Outfit(const std::string& id) {
  return FindDef(GetContent().outfits, id);
}
const QuestDef& Quest(const std::string& id) {
  return FindDef(GetContent().quests, id);
}
const char* RarityName(int r) {
  static const char* kNames[] = {"common", "rare", "epic", "legendary"};
  return r >= 0 && r < 4 ? kNames[r] : "";
}
}  // namespace frontier
