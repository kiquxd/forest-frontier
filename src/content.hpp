#ifndef FOREST_FRONTIER_SRC_CONTENT_HPP_
#define FOREST_FRONTIER_SRC_CONTENT_HPP_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace frontier {
constexpr int kGoodTypes = 13;
struct NpcDef {
  std::string id;
  std::string role;
  std::string personality;
  int weight;
  int rarity;
  std::array<int, 3> color;
  std::vector<std::string> names;
};
struct WeaponDef {
  std::string id;
  std::string name;
  std::string kind;
  std::string technology;
  int damage;
  int rarity;
  int weight;
  float reach;
  float duration;
  std::array<int, kGoodTypes> cost{};
};
struct TownDef {
  std::string id;
  std::string name;
  std::string teacher;
  std::string technology;
  int weight;
  int rarity;
  int attitude;
  std::array<int, 3> color;
  std::vector<int> goods;
};
struct Recipe {
  std::string name;
  std::array<int, kGoodTypes> cost{};
  std::string effect;
  std::string id;
  std::string result;
  std::string technology;
  bool quest_only{};
};
struct CharacterDef {
  std::string id;
  std::string name;
  std::string lore;
  std::string pros;
  std::string cons;
  float speed{1};
  float gather{1};
  float damage{1};
  float taken{1};
  float trade{1};
  int coins{};
};
struct OutfitDef {
  std::string id;
  std::string name;
  std::string description;
  std::string technology;
  std::string job;
  float speed{1};
  float gather{1};
  float taken{1};
  float trade{1};
  std::array<int, kGoodTypes> cost{};
  std::array<int, 3> color{};
  bool quest_only{};
};
struct QuestDef {
  std::string id;
  std::string title;
  std::string lore;
  std::string objective;
  std::string knowledge;
  std::string recipe;
  std::string weapon;
  std::string outfit;
  std::string role;
  int good{};
  int target{};
  int coins{};
  std::array<int, kGoodTypes> rewards{};
};
struct Content {
  std::vector<CharacterDef> characters;
  std::vector<OutfitDef> outfits;
  std::vector<QuestDef> quests;
  std::vector<NpcDef> npcs;
  std::vector<WeaponDef> weapons;
  std::vector<TownDef> towns;
  std::vector<Recipe> recipes;
};
const Content& GetContent();
const CharacterDef& Character(const std::string& id);
const OutfitDef& Outfit(const std::string& id);
const QuestDef& Quest(const std::string& id);
const WeaponDef& Weapon(const std::string& id);
int NpcType(const std::string& id);
int TownType(const std::string& id);
int WeightedNpc(uint64_t random);
int WeightedTown(uint64_t random);
const WeaponDef& WeightedWeapon(uint64_t random);
const char* RarityName(int rarity);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_CONTENT_HPP_
