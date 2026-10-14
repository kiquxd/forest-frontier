#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>

namespace frontier {
constexpr int goodTypes=13;
struct NpcDef {std::string id,role,personality;int weight,rarity;std::array<int,3> color;std::vector<std::string> names;};
struct WeaponDef {std::string id,name,kind,technology;int damage,rarity,weight;float reach,duration;std::array<int,goodTypes> cost{};};
struct TownDef {std::string id,name,teacher,technology;int weight,rarity,attitude;std::array<int,3> color;std::vector<int> goods;};
struct Recipe {std::string name;std::array<int,goodTypes> cost{};std::string effect,id,result,technology;bool questOnly{};};
struct CharacterDef {std::string id,name,lore,pros,cons;float speed{1},gather{1},damage{1},taken{1},trade{1};int coins{};};
struct OutfitDef {std::string id,name,description,technology,job;float speed{1},gather{1},taken{1},trade{1};std::array<int,goodTypes> cost{};std::array<int,3> color{};bool questOnly{};};
struct QuestDef {std::string id,title,lore,objective,knowledge,recipe,weapon,outfit,role;int good{},target{},coins{};std::array<int,goodTypes> rewards{};};
struct Content {std::vector<CharacterDef> characters;std::vector<OutfitDef> outfits;std::vector<QuestDef> quests;std::vector<NpcDef> npcs;std::vector<WeaponDef> weapons;std::vector<TownDef> towns;std::vector<Recipe> recipes;};
const Content& content();
const CharacterDef& character(const std::string& id);
const OutfitDef& outfit(const std::string& id);
const QuestDef& quest(const std::string& id);
const WeaponDef& weapon(const std::string& id);
int npcType(const std::string& id);
int townType(const std::string& id);
int weightedNpc(uint64_t random);
int weightedTown(uint64_t random);
const WeaponDef& weightedWeapon(uint64_t random);
const char* rarityName(int rarity);
}
