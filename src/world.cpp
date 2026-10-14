#include "world.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <queue>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "geometry.hpp"
#include "json.hpp"
#include "progress.hpp"
#include "settlement_system.hpp"

namespace frontier {
Vec operator+(Vec a, Vec b) {
  return {a.x + b.x, a.y + b.y};
}
Vec operator-(Vec a, Vec b) {
  return {a.x - b.x, a.y - b.y};
}
Vec operator*(Vec a, float b) {
  return {a.x * b, a.y * b};
}
float Length(Vec a) {
  return std::sqrt(a.x * a.x + a.y * a.y);
}
Vec Normalized(Vec a) {
  auto l = Length(a);
  return l > 0 ? a * (1 / l) : Vec{};
}
Vec Project(Vec a) {
  return {(a.x - a.y) * 0.8660254f, (a.x + a.y) * 0.5f};
}
Vec Unproject(Vec a) {
  return {a.y + a.x / 1.7320508f, a.y - a.x / 1.7320508f};
}
Coord ChunkAt(Vec position) {
  return {static_cast<int>(std::floor(position.x / kChunkSize)),
          static_cast<int>(std::floor(position.y / kChunkSize))};
}
uint64_t Hash(uint64_t n) {
  n += 0x9e3779b97f4a7c15ULL;
  n = (n ^ (n >> 30)) * 0xbf58476d1ce4e5b9ULL;
  n = (n ^ (n >> 27)) * 0x94d049bb133111ebULL;
  return n ^ (n >> 31);
}
size_t CoordHash::operator()(Coord coord) const {
  return Hash((static_cast<uint64_t>(static_cast<uint32_t>(coord.x)) << 32) |
              static_cast<uint32_t>(coord.y));
}
uint64_t ObjectId(int x, int y, uint64_t seed) {
  return Hash(((static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
               static_cast<uint32_t>(y)) ^
              seed);
}
Terrain TerrainAt(Vec position, uint64_t seed) {
  // Quantize before classifying so generation, collision and shoreline agree
  // exactly.
  position = {(std::floor(position.x / kTileSize) + .5f) * kTileSize,
              (std::floor(position.y / kTileSize) + .5f) * kTileSize};
  for (auto& s : Settlements()) {
    if (std::abs(position.x - s.pos.x) < 370 &&
        std::abs(position.y - s.pos.y) < 370 &&
        Length(position - s.pos) < 370) {
      return Terrain::kForest;
    }
  }
  float variation = static_cast<float>(seed % 97) * .025f;
  float ripple = std::sin(position.x * .006f + variation) * .01f +
                 std::cos(position.y * .009f) * .01f;
  auto lake = [&](Vec center, float rx, float ry) {
    Vec d = position - center;
    return std::sqrt(d.x * d.x / (rx * rx) + d.y * d.y / (ry * ry)) + ripple;
  };
  float shore =
      std::min(lake({1100, -1100}, 700, 700), lake({2850, 2050}, 850, 850));
  // Further north lies an open coastline, with deterministic bays.
  shore = std::min(shore, (position.y + 3100) / 570 +
                              std::sin(position.x * .0018f + variation) * .13f);
  if (shore < .72f) {
    return Terrain::kDeep;
  }
  if (shore < 1.f) {
    return Terrain::kShallow;
  }
  if (shore < 1.17f) {
    return Terrain::kSand;
  }
  return Terrain::kForest;
}
const char* TerrainName(Terrain t) {
  switch (t) {
    case Terrain::kForest:
      return "Pine forest";
    case Terrain::kSand:
      return "Lake shore";
    case Terrain::kShallow:
      return "Shallows";
    case Terrain::kDeep:
      return "Lake";
  }
  return "";
}
static std::vector<Settlement> towns;
static uint64_t configured_seed = 0;
void ConfigureWorld(uint64_t seed) {
  if (!towns.empty() && configured_seed == seed) {
    return;
  }
  configured_seed = seed;
  towns.clear();
  auto add = [&](std::string name, Vec pos, const std::string& type,
                 int attitude = 999) {
    int index = TownType(type);
    auto& def = GetContent().towns[index];
    towns.push_back({std::move(name), pos, index, def.rarity,
                     attitude == 999 ? def.attitude : attitude});
  };
  add("Pineford", {0, 0}, "village", 20);
  add("Quiet Glade", {2000, 700}, "village", 25);
  add("Saltwater Wharf", {-1000, -2300}, "port", 15);
  add("Ironhold", {3600, 700}, "forge", 0);
  add("Moon Academy", {4200, -1200}, "academy", 10);
  add("Copper Beacon", {6000, -2400}, "engineers", 0);
  add("Pearl Haven", {3100, 2900}, "divers", 10);
  add("Black Palisade", {-2200, 1400}, "raiders", -55);
  add("Archive of Tides", {-3600, 2200}, "ancients", -10);
  const char* names[] = {"White", "Far",   "North", "Stone",
                         "Quiet", "River", "Green", "Old"};
  const char* suffix[] = {"Bastion", "Port",   "Frontier",
                          "Cape",    "Island", "Haven"};
  for (int y = -2; y <= 3; y++) {
    for (int x = -3; x <= 3; x++) {
      auto n = ObjectId(x, y, seed ^ 0x781aULL);
      if (n % 100 >= 55) {
        continue;
      }
      Vec pos{static_cast<float>(x) * 4800 +
                  static_cast<float>((n >> 8) % 1400) - 700,
              static_cast<float>(y) * 4800 +
                  static_cast<float>((n >> 24) % 1400) - 700};
      bool close = false;
      for (auto& city : towns) {
        if (Length(city.pos - pos) < 2700) {
          close = true;
        }
      }
      if (close) {
        continue;
      }
      int type = WeightedTown(Hash(n));
      auto& def = GetContent().towns[type];
      towns.push_back(
          {std::string(names[(n >> 32) % 8]) + " " + suffix[(n >> 40) % 6], pos,
           type, def.rarity, def.attitude});
    }
  }
}
const std::vector<Settlement>& Settlements() {
  if (towns.empty()) {
    ConfigureWorld(742019);
  }
  return towns;
}
Coord MapCell(Vec position) {
  return {static_cast<int>(std::floor(position.x / kMapCellSize)),
          static_cast<int>(std::floor(position.y / kMapCellSize))};
}
bool ExploredAt(const std::unordered_set<Coord, CoordHash>& cells,
                Vec position) {
  return cells.contains(MapCell(position));
}
std::vector<Npc> MakeNpcs() {
  std::vector<Npc> out;
  for (size_t city = 0; city < Settlements().size(); city++) {
    auto& settlement = Settlements()[city];
    auto& type = GetContent().towns[settlement.type];
    int residents = 6 + settlement.rarity;
    for (int i = 0; i < residents; i++) {
      auto n = ObjectId(static_cast<int>(city), i, configured_seed ^ 0x842ULL);
      int kind = i == 0               ? NpcType(type.teacher)
                 : i == 1             ? NpcType("elder")
                 : i == residents - 1 ? NpcType("wanderer")
                                      : WeightedNpc(n);
      auto& def = GetContent().npcs[kind];
      float angle = i * 6.2831853f / residents;
      Vec home =
          settlement.pos + Vec{std::cos(angle) * 110, std::sin(angle) * 110};
      out.push_back({static_cast<int>(out.size()), static_cast<int>(city),
                     def.names[(n >> 16) % def.names.size()], def.role,
                     def.personality, home, home,
                     static_cast<float>(i + city * 3), home, 0, kind,
                     def.rarity});
      if (i == residents - 1) {
        int origin = static_cast<int>(
            Hash(n) % static_cast<uint64_t>(Settlements().size() - 1));
        if (origin >= static_cast<int>(city)) {
          origin++;
        }
        out.back().origin_city = origin;
      }
      out.back().ruler = i == 1;
      if (i == 1) {
        out.back().role = "Ruler";
        out.back().pos = out.back().home = out.back().previous = settlement.pos;
      }
    }
  }
  return out;
}
std::vector<Building> MakeBuildings() {
  std::vector<Building> out;
  for (auto& city : Settlements()) {
    for (int i = 0; i < 5 + city.rarity; i++) {
      float angle = i * 6.2831853f / (5 + city.rarity);
      out.push_back(
          {city.pos + Vec{std::cos(angle) * 235, std::sin(angle) * 235},
           city.type % 3});
    }
  }
  for (auto& city : Settlements()) {
    out.push_back({city.pos, 0, true});
  }
  return out;
}
bool PathAt(Vec position) {
  for (auto& city : Settlements()) {
    if (std::abs(position.x - city.pos.x) < 310 &&
        std::abs(position.y - city.pos.y) < 310 &&
        Length(position - city.pos) < 290) {
      return true;
    }
  }
  return false;
}
Chunk GenerateChunk(Coord coord, uint64_t seed) {
  const auto start = std::chrono::steady_clock::now();
  Chunk out;
  out.coord = coord;
  out.tiles.reserve(kChunkTiles * kChunkTiles);
  for (int y = 0; y < kChunkTiles; y++) {
    for (int x = 0; x < kChunkTiles; x++) {
      int tx = coord.x * kChunkTiles + x, ty = coord.y * kChunkTiles + y;
      const uint64_t n = ObjectId(tx, ty, seed);
      Vec center = {(tx + .5f) * kTileSize, (ty + .5f) * kTileSize};
      bool path = PathAt(center);
      auto terrain = TerrainAt(center, seed);
      out.tiles.push_back({static_cast<uint8_t>(n % 8), path, terrain});
      if (path) {
        continue;
      }
      const unsigned chance = n % 100;
      if (terrain != Terrain::kForest) {
        if (terrain == Terrain::kSand && chance < 16) {
          out.resources.push_back({n, center, ResourceKind::kReed,
                                   static_cast<int>((n >> 32) % 3)});
        }
        if (terrain == Terrain::kShallow && chance < 9) {
          out.resources.push_back({n, center, ResourceKind::kFish,
                                   static_cast<int>((n >> 32) % 3)});
        }
        if (terrain == Terrain::kDeep || terrain == Terrain::kShallow) {
          if (chance < 4) {
            out.underwater.push_back(
                {Hash(n ^ 0xd1ULL), center, ResourceKind::kPearl, 0});
          } else if (chance < 6) {
            out.underwater.push_back(
                {Hash(n ^ 0xd2ULL), center, ResourceKind::kCrystal, 0});
          } else if (chance < 8) {
            out.underwater.push_back(
                {Hash(n ^ 0xd3ULL), center, ResourceKind::kSalvage, 0});
          }
        }
        continue;
      }
      if (chance < 21) {
        auto kind = chance < 14   ? ResourceKind::kTree
                    : chance < 18 ? ResourceKind::kStone
                                  : ResourceKind::kBerry;
        Vec position = center + Vec{static_cast<float>((n >> 16) % 21) - 10,
                                    static_cast<float>((n >> 24) % 21) - 10};
        out.resources.push_back(
            {n, position, kind, static_cast<int>((n >> 32) % 3)});
      }
      if (chance == 93) {
        out.resources.push_back({n, center, ResourceKind::kOre, 0});
      }
      if (chance == 97 && out.animals.size() < 4) {
        out.animals.push_back({center, center,
                               static_cast<float>((n >> 8) % 628) * .01f,
                               center});
      }
      if (chance == 99 && out.enemies.empty()) {
        bool safe = false;
        for (auto& s : Settlements()) {
          if (Length(center - s.pos) < 500) {
            safe = true;
          }
        }
        if (!safe) {
          out.enemies.push_back({Hash(n ^ 0xabcdeULL), center, center, center});
        }
      }
    }
  }
  for (size_t city = 0; city < Settlements().size(); city++) {
    for (int guard = 0; guard < 2; guard++) {
      auto home = Settlements()[city].pos + Vec{guard ? -315.f : 315.f, 40};
      if (ChunkAt(home) != coord) {
        continue;
      }
      Enemy enemy{ObjectId(static_cast<int>(city), guard, seed ^ 0xbcbcULL),
                  home, home, home};
      enemy.kind = 1;
      enemy.city = static_cast<int>(city);
      enemy.hp = 100;
      out.enemies.push_back(enemy);
    }
  }
  out.generation_ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - start)
                          .count();
  return out;
}
Save LoadSave(const std::filesystem::path& path) {
  Save save;
  if (!std::filesystem::exists(path)) {
    return save;
  }
  std::ifstream f(path);
  nlohmann::json json;
  f >> json;
  int version = json.at("version").get<int>();
  if (version < 1 || version > 8) {
    throw std::runtime_error("Unsupported save version");
  }
  save.seed = json.at("seed").get<uint64_t>();
  save.player = {json.at("player").at(0).get<float>(),
                 json.at("player").at(1).get<float>()};
  if (!std::isfinite(save.player.x) || !std::isfinite(save.player.y) ||
      Length(save.player) > 1e7f) {
    throw std::runtime_error("Invalid player coordinates");
  }
  save.inventory = {json.at("wood").get<int>(), json.at("stone").get<int>(),
                    json.at("berries").get<int>()};
  if (save.inventory.wood < 0 || save.inventory.stone < 0 ||
      save.inventory.berries < 0) {
    throw std::runtime_error("Invalid inventory");
  }
  for (auto& id : json.at("harvested")) {
    save.harvested.insert(id.get<uint64_t>());
  }
  if (version >= 2) {
    auto& inv = json.at("inventory");
    save.inventory.reeds = inv.at("reeds");
    save.inventory.fish = inv.at("fish");
    save.inventory.leather = inv.at("leather");
    save.inventory.potions = inv.at("potions");
    save.inventory.campfires = inv.at("campfires");
    save.inventory.axe = inv.at("axe");
    save.inventory.pickaxe = inv.at("pickaxe");
    save.inventory.sword = inv.at("sword");
    save.inventory.rod = inv.at("rod");
    save.inventory.boat = inv.at("boat");
    save.inventory.armor = inv.at("armor");
    save.health = json.at("health");
    save.coins = json.at("coins");
    save.stocks.clear();
    for (auto& row : json.at("stocks")) {
      std::array<int, kGoodTypes> stock{};
      if (row.size() > kGoodTypes) {
        throw std::runtime_error("Invalid stock size");
      }
      for (size_t i = 0; i < row.size(); i++) {
        stock[i] = row[i].get<int>();
      }
      save.stocks.push_back(stock);
    }
    save.treasury = json.at("treasury").get<decltype(save.treasury)>();
    for (auto& id : json.at("defeated")) {
      save.defeated.insert(id.get<uint64_t>());
    }
    for (auto& p : json.at("campfires")) {
      Vec pos{p.at(0).get<float>(), p.at(1).get<float>()};
      if (!std::isfinite(pos.x) || !std::isfinite(pos.y) ||
          Length(pos) > 1e7f) {
        throw std::runtime_error("Invalid campfire");
      }
      save.campfires.push_back(pos);
    }
    if (save.health < 1 || save.health > 100 || save.coins < 0 ||
        save.coins > 10000000 || save.inventory.reeds < 0 ||
        save.inventory.fish < 0 || save.inventory.leather < 0 ||
        save.inventory.potions < 0 || save.inventory.campfires < 0) {
      throw std::runtime_error("Invalid gameplay save");
    }
    for (auto& stock : save.stocks) {
      for (auto count : stock) {
        if (count < 0 || count > 10000000) {
          throw std::runtime_error("Invalid stock");
        }
      }
    }
    for (int coins : save.treasury) {
      if (coins < 0 || coins > 10000000) {
        throw std::runtime_error("Invalid treasury");
      }
    }
  }
  if (version >= 3) {
    auto& inv = json.at("inventory");
    save.inventory.rare = inv.at("rare").get<decltype(save.inventory.rare)>();
    save.inventory.weapons =
        inv.at("weapons").get<decltype(save.inventory.weapons)>();
    save.inventory.weapon_id = inv.at("weapon_id");
    save.inventory.ship = inv.at("ship");
    save.inventory.submarine = inv.at("submarine");
    save.inventory.sonar = inv.at("sonar");
    save.inventory.oxygen = inv.at("oxygen_module");
    save.inventory.ammo = inv.at("ammo");
    save.inventory.repairs = inv.at("repairs");
    save.technologies =
        json.at("technologies").get<decltype(save.technologies)>();
    for (auto& item : json.at("relations")) {
      int treaty = item.at("treaty");
      int opinion = item.at("opinion");
      if (treaty < 0 || treaty > 4 || opinion < -100 || opinion > 100) {
        throw std::runtime_error("Invalid diplomacy state");
      }
      save.relations.push_back(
          {opinion, Treaty(treaty), item.at("discovered")});
    }
    auto read_map = [](const nlohmann::json& array, auto& cells) {
      for (auto& item : array) {
        Coord coord{item.at(0).template get<int>(),
                    item.at(1).template get<int>()};
        if (std::abs(coord.x) > 20000 || std::abs(coord.y) > 20000) {
          throw std::runtime_error("Invalid explored cell");
        }
        cells.insert(coord);
      }
    };
    read_map(json.at("explored"), save.explored);
    read_map(json.at("explored_underwater"), save.explored_underwater);
    for (auto& wall : json.at("walls")) {
      Wall w{wall.at("city"),
             {wall.at("a").at(0), wall.at("a").at(1)},
             {wall.at("b").at(0), wall.at("b").at(1)},
             wall.at("hp")};
      if (w.city < 0 || w.city >= static_cast<int>(save.relations.size()) ||
          !std::isfinite(Length(w.a)) || !std::isfinite(Length(w.b)) ||
          Length(w.b - w.a) > 200 || Length(w.b - w.a) < 1 || w.hp < 0 ||
          w.hp > 1000) {
        throw std::runtime_error("Invalid wall");
      }
      save.walls.push_back(w);
    }
    save.submerged = json.at("submerged");
    save.oxygen = json.at("oxygen");
    save.hull = json.at("hull");
    save.vehicle = json.at("vehicle");
    if (!std::isfinite(save.oxygen) || save.oxygen < 0 || save.oxygen > 200 ||
        save.hull < 1 || save.hull > 100 || save.vehicle < 0 ||
        save.vehicle > (version >= 7 ? 9 : 3)) {
      throw std::runtime_error("Invalid vessel state");
    }
    for (int n : save.inventory.rare) {
      if (n < 0 || n > 1000000) {
        throw std::runtime_error("Invalid rare resource");
      }
    }
    for (auto& id : save.inventory.weapons) {
      (void)Weapon(id);
    }
    if (!save.inventory.weapon_id.empty() &&
        !save.inventory.weapons.contains(save.inventory.weapon_id)) {
      throw std::runtime_error("Invalid equipped weapon");
    }
    if (save.inventory.ammo < 0 || save.inventory.ammo > 1000000 ||
        save.inventory.repairs < 0 || save.inventory.repairs > 1000000) {
      throw std::runtime_error("Invalid supplies");
    }
  } else if (save.inventory.sword) {
    save.inventory.weapons.insert("wood_sword");
    save.inventory.weapon_id = "wood_sword";
  }
  if (version >= 4) {
    save.campaign = json.at("campaign");
    save.home_city = json.at("home_city");
    save.owners = json.at("owners").get<decltype(save.owners)>();
    save.city_opinions =
        json.at("city_opinions").get<decltype(save.city_opinions)>();
    save.skills = json.at("skills").get<decltype(save.skills)>();
    save.gathered = json.at("gathered");
    save.crafted = json.at("crafted");
    save.caught = json.at("caught");
    save.orders = json.at("orders");
    if (save.campaign) {
      if (save.home_city < 0 ||
          save.home_city >= static_cast<int>(save.owners.size()) ||
          save.city_opinions.size() != save.owners.size() ||
          save.owners.size() != save.relations.size()) {
        throw std::runtime_error("Invalid campaign");
      }
      for (auto& row : save.city_opinions) {
        if (row.size() != save.owners.size()) {
          throw std::runtime_error("Invalid city matrix");
        }
        for (int value : row) {
          if (value < -100 || value > 100) {
            throw std::runtime_error("Invalid opinion");
          }
        }
      }
      if (save.gathered < 0 || save.crafted < 0 || save.caught < 0 ||
          save.orders < 0) {
        throw std::runtime_error("Invalid progress");
      }
    }
  }
  if (version >= 5) {
    save.adventure = json.at("adventure");
    save.character_id = json.at("character");
    save.outfit_id = json.at("outfit");
    save.weapon_copies =
        json.at("weapon_copies").get<decltype(save.weapon_copies)>();
    save.outfit_copies =
        json.at("outfit_copies").get<decltype(save.outfit_copies)>();
    save.known_recipes =
        json.at("known_recipes").get<decltype(save.known_recipes)>();
    save.work_clock = json.at("work_clock");
    save.tracked_quest = json.value("tracked_quest", -1);
    save.wolf_kills = json.value("wolf_kills", 0);
    if (save.wolf_kills < 0 || save.tracked_quest < -1 ||
        save.tracked_quest > 10000) {
      throw std::runtime_error("Invalid quest counters");
    }
    for (auto& row : json.at("quests")) {
      int npc = row.at("npc");
      QuestProgress progress{row.at("id"), row.at("completed"),
                             row.at("baseline")};
      (void)Quest(progress.id);
      if (npc < 0 || npc > 10000 || progress.baseline < 0) {
        throw std::runtime_error("Invalid quest progress");
      }
      save.quests.emplace(npc, std::move(progress));
    }
    std::unordered_set<int> resident_ids;
    for (auto& row : json.at("citizens")) {
      Citizen resident{row.at("npc"),    row.at("city"),   row.at("job"),
                       row.at("weapon"), row.at("outfit"), row.at("trained")};
      if (version >= 8) {
        resident.health = row.at("health");
        resident.state = row.at("state");
        resident.captor = row.at("captor");
        resident.ransom = row.at("ransom");
        bool captive = resident.state == "captive";
        bool dead = resident.state == "dead";
        if ((!captive && !dead && resident.state != "active") ||
            resident.health < 0 || resident.health > 100 ||
            (resident.state == "active" && resident.health == 0) ||
            (resident.state != "active" && resident.health != 0) ||
            (captive &&
             (resident.captor < 0 ||
              resident.captor >= static_cast<int>(save.relations.size()) ||
              resident.ransom < 1 || resident.ransom > 10000)) ||
            (!captive && (resident.captor != -1 || resident.ransom != 0)) ||
            (dead && (!resident.weapon.empty() || !resident.outfit.empty() ||
                      resident.trained || resident.job != "idle"))) {
          throw std::runtime_error("Invalid resident status");
        }
      }
      if (resident.npc < 0 || resident.npc > 10000 || resident.city < 0 ||
          resident.city >= static_cast<int>(save.relations.size()) ||
          !resident_ids.insert(resident.npc).second ||
          std::find(ResidentJobs().begin(), ResidentJobs().end(),
                    resident.job) == ResidentJobs().end()) {
        throw std::runtime_error("Invalid resident");
      }
      if (!resident.weapon.empty()) {
        (void)Weapon(resident.weapon);
      }
      if (!resident.outfit.empty()) {
        (void)Outfit(resident.outfit);
      }
      save.citizens.push_back(std::move(resident));
    }
    (void)Character(save.character_id);
    (void)Outfit(save.outfit_id);
    if (!std::isfinite(save.work_clock) || save.work_clock < 0 ||
        save.work_clock >= 20) {
      throw std::runtime_error("Invalid work clock");
    }
    for (auto& [id, count] : save.weapon_copies) {
      (void)Weapon(id);
      if (count < 0 || count > 1000000) {
        throw std::runtime_error("Invalid weapon count");
      }
    }
    for (auto& [id, count] : save.outfit_copies) {
      (void)Outfit(id);
      if (count < 0 || count > 1000000) {
        throw std::runtime_error("Invalid outfit count");
      }
    }
    if (save.adventure) {
      if (AvailableOutfit(save, save.outfit_id) < 1 ||
          AvailableWeapon(save, save.inventory.weapon_id) < 1) {
        throw std::runtime_error("Invalid equipped allocation");
      }
      for (auto& [id, count] : save.weapon_copies) {
        if (AvailableWeapon(save, id) < 0) {
          throw std::runtime_error("Duplicate weapon allocation");
        }
      }
      for (auto& [id, count] : save.outfit_copies) {
        if (AvailableOutfit(save, id) < 0) {
          throw std::runtime_error("Duplicate outfit allocation");
        }
      }
    }
  }
  if (version >= 7) {
    save.vessels = json.at("vessels").get<decltype(save.vessels)>();
    for (const auto& id : save.vessels) {
      if (id != "raft" && id != "canoe" && id != "skiff" && id != "cutter" &&
          id != "brigantine" && id != "flagship") {
        throw std::runtime_error("Invalid vessel ID");
      }
    }
    auto deposits = json.value("known_deposits", nlohmann::json::array());
    if (!deposits.is_array() || deposits.size() > 4096) {
      throw std::runtime_error("Invalid known deposits");
    }
    for (const auto& row : deposits) {
      uint64_t id = row.at("id");
      KnownDeposit site{
          {row.at("x"), row.at("y")}, row.at("good"), row.at("underwater")};
      if (!std::isfinite(site.position.x) || !std::isfinite(site.position.y) ||
          Length(site.position) > 2e6f || site.good < 6 ||
          site.good >= kGoodTypes ||
          !save.known_deposits.emplace(id, site).second) {
        throw std::runtime_error("Invalid known deposit");
      }
    }
    const auto& notes = json.at("map_notes");
    if (!notes.is_array() || notes.size() > 128) {
      throw std::runtime_error("Invalid map journal");
    }
    for (const auto& row : notes) {
      MapNote note{{row.at("x"), row.at("y")},
                   row.at("underwater"),
                   row.at("title"),
                   row.at("text")};
      if (!std::isfinite(note.position.x) || !std::isfinite(note.position.y) ||
          Length(note.position) > 2e6f || note.title.empty() ||
          note.title.size() > 64 || note.text.size() > 2048) {
        throw std::runtime_error("Invalid map note");
      }
      save.map_notes.push_back(std::move(note));
    }
  }
  if (version >= 6) {
    save.known_goods = json.at("known_goods").get<decltype(save.known_goods)>();
    const auto& history = json.at("history");
    if (!history.is_array() || history.size() > 512) {
      throw std::runtime_error("Invalid history");
    }
    for (const auto& row : history) {
      HistoryEntry event{row.at("kind"), row.at("description"),
                         row.at("timestamp"), row.at("city")};
      if (event.kind.empty() || event.kind.size() > 64 ||
          event.description.size() > 512 || event.timestamp < 0 ||
          event.city < -1 ||
          event.city >= static_cast<int>(save.relations.size())) {
        throw std::runtime_error("Invalid history entry");
      }
      save.history.push_back(std::move(event));
    }
  } else {
    RememberResources(save);
    if (save.gathered > 0) {
      save.known_goods[0] = save.known_goods[1] = save.known_goods[2] =
          save.known_goods[3] = true;
    }
    if (save.caught > 0) {
      save.known_goods[4] = true;
    }
    if (save.wolf_kills > 0) {
      save.known_goods[5] = true;
    }
  }
  if (version >= 8) {
    save.urban = json.at("urban");
    save.game_time = json.at("game_time");
    save.craft_ready_at = json.at("craft_ready_at");
    save.city_radii = json.at("city_radii").get<decltype(save.city_radii)>();
    save.previous_owners =
        json.at("previous_owners").get<decltype(save.previous_owners)>();
    size_t towns = save.relations.size();
    if (!std::isfinite(save.game_time) || save.game_time < 0 ||
        save.game_time > 1e12 || !std::isfinite(save.craft_ready_at) ||
        save.craft_ready_at < 0 ||
        save.craft_ready_at > save.game_time + kMaxCraftCooldown ||
        (save.urban && (save.city_radii.size() != towns ||
                        save.previous_owners.size() != towns)) ||
        (!save.urban &&
         (!save.city_radii.empty() || !save.previous_owners.empty()))) {
      throw std::runtime_error("Invalid town progression");
    }
    for (float radius : save.city_radii) {
      if (!std::isfinite(radius) || radius < 350 || radius > 1100) {
        throw std::runtime_error("Invalid town territory");
      }
    }
    for (int owner : save.previous_owners) {
      if (owner < 0 || owner > static_cast<int>(towns)) {
        throw std::runtime_error("Invalid previous ruler");
      }
    }
    auto valid_city = [&](int city) {
      return city >= 0 && city < static_cast<int>(towns);
    };
    auto point = [](const nlohmann::json& row) {
      Vec p{row.at(0).get<float>(), row.at(1).get<float>()};
      if (!std::isfinite(p.x) || !std::isfinite(p.y) || std::abs(p.x) > 2e6 ||
          std::abs(p.y) > 2e6) {
        throw std::runtime_error("Invalid infrastructure position");
      }
      return p;
    };
    const auto& facilities = json.at("facilities");
    const auto& roads = json.at("roads");
    const auto& tributes = json.at("tributes");
    if (!facilities.is_array() ||
        facilities.size() > towns * FacilityTypes().size() ||
        !roads.is_array() || roads.size() > 128 || !tributes.is_array() ||
        tributes.size() > towns ||
        (!save.urban &&
         (!facilities.empty() || !roads.empty() || !tributes.empty()))) {
      throw std::runtime_error("Invalid infrastructure counts");
    }
    std::unordered_set<std::string> sites;
    for (const auto& row : facilities) {
      Facility f{row.at("city"), row.at("kind"), point(row.at("position"))};
      if (!valid_city(f.city) ||
          std::none_of(FacilityTypes().begin(), FacilityTypes().end(),
                       [&](const auto& type) { return type.id == f.kind; }) ||
          !sites.insert(std::to_string(f.city) + ":" + f.kind).second) {
        throw std::runtime_error("Invalid town building");
      }
      save.facilities.push_back(f);
    }
    std::unordered_set<std::string> connections;
    for (const auto& row : roads) {
      Road r{row.at("from"), row.at("to"), {}};
      const auto& route = row.at("points");
      if (!valid_city(r.from) || !valid_city(r.to) || r.from == r.to ||
          !route.is_array() || route.size() < 2 || route.size() > 64 ||
          !connections
               .insert(std::to_string(std::min(r.from, r.to)) + ":" +
                       std::to_string(std::max(r.from, r.to)))
               .second) {
        throw std::runtime_error("Invalid saved road");
      }
      double length = 0;
      for (const auto& vertex : route) {
        Vec p = point(vertex);
        if (!r.points.empty()) {
          float segment = Length(p - r.points.back());
          if (segment < 1) {
            throw std::runtime_error("Invalid road segment");
          }
          length += segment;
        }
        r.points.push_back(p);
      }
      if (length > 50000) {
        throw std::runtime_error("Saved road is too long");
      }
      save.roads.push_back(std::move(r));
    }
    std::unordered_set<int> subjects;
    for (const auto& row : tributes) {
      Tribute t{row.at("city"),   row.at("patron"), row.at("previous_owner"),
                row.at("amount"), row.at("buyout"), row.at("next_due")};
      if (!valid_city(t.city) || !valid_city(t.patron) || t.city == t.patron ||
          t.previous_owner < 0 || t.previous_owner > static_cast<int>(towns) ||
          t.amount != 10 || t.buyout != 300 || !std::isfinite(t.next_due) ||
          t.next_due < 0 || t.next_due > save.game_time + 120 ||
          !subjects.insert(t.city).second) {
        throw std::runtime_error("Invalid tribute obligation");
      }
      save.tributes.push_back(t);
    }
  }
  return save;
}
void WriteSave(const std::filesystem::path& path, const Save& save) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  nlohmann::json inv = {{"reeds", save.inventory.reeds},
                        {"fish", save.inventory.fish},
                        {"leather", save.inventory.leather},
                        {"potions", save.inventory.potions},
                        {"campfires", save.inventory.campfires},
                        {"axe", save.inventory.axe},
                        {"pickaxe", save.inventory.pickaxe},
                        {"sword", save.inventory.sword},
                        {"rod", save.inventory.rod},
                        {"boat", save.inventory.boat},
                        {"armor", save.inventory.armor},
                        {"rare", save.inventory.rare},
                        {"weapons", save.inventory.weapons},
                        {"weapon_id", save.inventory.weapon_id},
                        {"ship", save.inventory.ship},
                        {"submarine", save.inventory.submarine},
                        {"sonar", save.inventory.sonar},
                        {"oxygen_module", save.inventory.oxygen},
                        {"ammo", save.inventory.ammo},
                        {"repairs", save.inventory.repairs}};
  nlohmann::json fires = nlohmann::json::array();
  for (auto position : save.campfires) {
    fires.push_back({position.x, position.y});
  }
  nlohmann::json diplomacy = nlohmann::json::array(),
                 map = nlohmann::json::array(),
                 underwater_map = nlohmann::json::array(),
                 walls = nlohmann::json::array();
  for (auto r : save.relations) {
    diplomacy.push_back({{"opinion", r.opinion},
                         {"treaty", static_cast<int>(r.treaty)},
                         {"discovered", r.discovered}});
  }
  for (auto coord : save.explored) {
    map.push_back({coord.x, coord.y});
  }
  for (auto coord : save.explored_underwater) {
    underwater_map.push_back({coord.x, coord.y});
  }
  for (auto& w : save.walls) {
    walls.push_back({{"city", w.city},
                     {"a", {w.a.x, w.a.y}},
                     {"b", {w.b.x, w.b.y}},
                     {"hp", w.hp}});
  }
  nlohmann::json json = {{"version", 8},
                         {"seed", save.seed},
                         {"player", {save.player.x, save.player.y}},
                         {"wood", save.inventory.wood},
                         {"stone", save.inventory.stone},
                         {"berries", save.inventory.berries},
                         {"harvested", save.harvested},
                         {"inventory", inv},
                         {"health", save.health},
                         {"coins", save.coins},
                         {"defeated", save.defeated},
                         {"campfires", fires},
                         {"stocks", save.stocks},
                         {"treasury", save.treasury},
                         {"relations", diplomacy},
                         {"technologies", save.technologies},
                         {"explored", map},
                         {"explored_underwater", underwater_map},
                         {"walls", walls},
                         {"submerged", save.submerged},
                         {"oxygen", save.oxygen},
                         {"hull", save.hull},
                         {"vehicle", save.vehicle}};
  json["campaign"] = save.campaign;
  json["home_city"] = save.home_city;
  json["owners"] = save.owners;
  json["city_opinions"] = save.city_opinions;
  json["skills"] = save.skills;
  json["gathered"] = save.gathered;
  json["crafted"] = save.crafted;
  json["caught"] = save.caught;
  json["orders"] = save.orders;
  json["adventure"] = save.adventure;
  json["character"] = save.character_id;
  json["outfit"] = save.outfit_id;
  json["weapon_copies"] = save.weapon_copies;
  json["outfit_copies"] = save.outfit_copies;
  json["known_recipes"] = save.known_recipes;
  json["work_clock"] = save.work_clock;
  json["tracked_quest"] = save.tracked_quest;
  json["wolf_kills"] = save.wolf_kills;
  json["quests"] = nlohmann::json::array();
  for (auto& [id, q] : save.quests) {
    json["quests"].push_back({{"npc", id},
                              {"id", q.id},
                              {"completed", q.completed},
                              {"baseline", q.baseline}});
  }
  json["citizens"] = nlohmann::json::array();
  for (auto& resident : save.citizens) {
    json["citizens"].push_back({{"npc", resident.npc},
                                {"city", resident.city},
                                {"job", resident.job},
                                {"weapon", resident.weapon},
                                {"outfit", resident.outfit},
                                {"trained", resident.trained},
                                {"health", resident.health},
                                {"state", resident.state},
                                {"captor", resident.captor},
                                {"ransom", resident.ransom}});
  }
  json["urban"] = save.urban;
  json["game_time"] = save.game_time;
  json["craft_ready_at"] = save.craft_ready_at;
  json["city_radii"] = save.city_radii;
  json["previous_owners"] = save.previous_owners;
  json["facilities"] = nlohmann::json::array();
  for (const auto& f : save.facilities) {
    json["facilities"].push_back({{"city", f.city},
                                  {"kind", f.kind},
                                  {"position", {f.position.x, f.position.y}}});
  }
  json["roads"] = nlohmann::json::array();
  for (const auto& r : save.roads) {
    nlohmann::json points = nlohmann::json::array();
    for (Vec p : r.points) {
      points.push_back({p.x, p.y});
    }
    json["roads"].push_back(
        {{"from", r.from}, {"to", r.to}, {"points", points}});
  }
  json["tributes"] = nlohmann::json::array();
  for (const auto& t : save.tributes) {
    json["tributes"].push_back({{"city", t.city},
                                {"patron", t.patron},
                                {"previous_owner", t.previous_owner},
                                {"amount", t.amount},
                                {"buyout", t.buyout},
                                {"next_due", t.next_due}});
  }
  json["known_deposits"] = nlohmann::json::array();
  std::vector<uint64_t> deposit_ids;
  for (const auto& [id, site] : save.known_deposits) {
    deposit_ids.push_back(id);
  }
  std::sort(deposit_ids.begin(), deposit_ids.end());
  for (auto id : deposit_ids) {
    const auto& site = save.known_deposits.at(id);
    json["known_deposits"].push_back({{"id", id},
                                      {"x", site.position.x},
                                      {"y", site.position.y},
                                      {"good", site.good},
                                      {"underwater", site.underwater}});
  }
  json["vessels"] = save.vessels;
  json["map_notes"] = nlohmann::json::array();
  for (const auto& note : save.map_notes) {
    json["map_notes"].push_back({{"x", note.position.x},
                                 {"y", note.position.y},
                                 {"underwater", note.underwater},
                                 {"title", note.title},
                                 {"text", note.text}});
  }
  json["known_goods"] = save.known_goods;
  json["history"] = nlohmann::json::array();
  for (const auto& event : save.history) {
    json["history"].push_back({{"kind", event.kind},
                               {"description", event.description},
                               {"timestamp", event.timestamp},
                               {"city", event.city}});
  }
  if (std::filesystem::exists(path)) {
    std::ifstream existing(path);
    auto previous = nlohmann::json::parse(existing, nullptr, false);
    int old_version = previous.is_object() ? previous.value("version", 0) : 0;
    auto backup = path;
    backup += ".v" + std::to_string(old_version) + ".bak";
    if (old_version > 0 && old_version < 8 &&
        !std::filesystem::exists(backup)) {
      std::filesystem::copy_file(path, backup);
    }
  }
  auto temp = path;
  temp += ".tmp";
  {
    std::ofstream f(temp);
    if (!f) {
      throw std::runtime_error("Cannot open save file");
    }
    f << json.dump(2) << '\n';
    f.flush();
    if (!f) {
      throw std::runtime_error("Cannot write save");
    }
  }
  std::filesystem::rename(temp, path);
}
World::World(uint64_t seed) : seed_(seed) {
  ConfigureWorld(seed);
  npcs = MakeNpcs();
  buildings = MakeBuildings();
  worker_ = std::thread([this] { Run(); });
}
World::~World() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  wake_.notify_all();
  worker_.join();
}
void World::Request(Vec player, int radius) {
  radius = std::clamp(radius, 1, 5);
  Coord center = ChunkAt(player);
  std::vector<Coord> needed;
  std::lock_guard lock(mutex_);
  desired_.clear();
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      Coord coord{center.x + x, center.y + y};
      desired_.insert(coord);
      if (!chunks.contains(coord) && !pending_.contains(coord)) {
        needed.push_back(coord);
      }
    }
  }
  // Drop stale jobs before queueing nearest-first work. Queue is bounded by
  // view radius.
  std::erase_if(jobs_, [&](Coord coord) {
    if (desired_.contains(coord)) {
      return false;
    }
    pending_.erase(coord);
    return true;
  });
  std::erase_if(ready_, [&](const Chunk& chunk) {
    if (desired_.contains(chunk.coord)) {
      return false;
    }
    pending_.erase(chunk.coord);
    return true;
  });
  std::sort(needed.begin(), needed.end(), [&](Coord a, Coord b) {
    return (a.x - center.x) * (a.x - center.x) +
               (a.y - center.y) * (a.y - center.y) <
           (b.x - center.x) * (b.x - center.x) +
               (b.y - center.y) * (b.y - center.y);
  });
  for (Coord coord : needed) {
    jobs_.push_back(coord);
    pending_.insert(coord);
  }
  std::erase_if(
      chunks, [&](const auto& pair) { return !desired_.contains(pair.first); });
  wake_.notify_one();
}
void World::Run() {
  for (;;) {
    Coord coord;
    {
      std::unique_lock lock(mutex_);
      wake_.wait(lock, [&] { return stopping_ || !jobs_.empty(); });
      if (stopping_) {
        return;
      }
      coord = jobs_.front();
      jobs_.pop_front();
    }
    Chunk chunk = GenerateChunk(coord, seed_);
    {
      std::lock_guard lock(mutex_);
      if (desired_.contains(coord)) {
        ready_.push_back(std::move(chunk));
      } else {
        pending_.erase(coord);
      }
    }
  }
}
int World::Integrate(int budget) {
  std::lock_guard lock(mutex_);
  int count = 0;
  while (!ready_.empty() && count < budget) {
    Chunk chunk = std::move(ready_.front());
    ready_.pop_front();
    pending_.erase(chunk.coord);
    generated_ms += chunk.generation_ms;
    max_generation_ms = std::max(max_generation_ms, chunk.generation_ms);
    generated_count++;
    if (desired_.contains(chunk.coord)) {
      chunks.insert_or_assign(chunk.coord, std::move(chunk));
    }
    count++;
  }
  return count;
}
size_t World::Pending() const {
  std::lock_guard lock(mutex_);
  return pending_.size();
}
void World::Tick(float dt, const Save& save, double time) {
  if (save.submerged) {
    return;
  }
  Vec player = save.player;
  for (auto& npc : npcs) {
    if (Length(npc.home - player) < 2200) {
      npc.previous = npc.pos;
      if (!ResidentActive(save, npc.id)) {
        npc.pos = npc.previous = npc.home;
        continue;
      }
      if (npc.ruler) {
        continue;
      }
      if (npc.id == talking_npc) {
        npc.walk = 0;
        continue;
      }
      Vec target =
          npc.home +
          Vec{std::sin(static_cast<float>(time) * .16f + npc.phase) * 50,
              std::cos(static_cast<float>(time) * .13f + npc.phase) * 40};
      if (npc.origin_city >= 0 && save.campaign &&
          save.city_opinions[npc.settlement][npc.origin_city] < -25) {
        target = target +
                 Normalized(npc.home - Settlements()[npc.settlement].pos) * 180;
      }
      if (auto resident = GetCitizen(save, npc.id);
          resident && resident->trained && resident->job == "guard") {
        target = target +
                 Normalized(npc.home - Settlements()[npc.settlement].pos) * 160;
      }
      Vec diff = target - npc.pos;
      npc.pos =
          Move(npc.pos, Normalized(diff) * std::min(Length(diff), dt * 24),
               save, 10);
      npc.walk += Length(npc.pos - npc.previous) / 10;
    }
  }
  // Local disc contacts affect positions, not schedules or destinations.
  for (size_t i = 0; i < npcs.size(); i++) {
    if (npcs[i].ruler || !ResidentActive(save, npcs[i].id) ||
        Length(npcs[i].pos - save.player) > 1800) {
      continue;
    }
    for (size_t j = i + 1; j < npcs.size(); j++) {
      auto& a = npcs[i];
      auto& b = npcs[j];
      if (b.ruler || !ResidentActive(save, b.id)) {
        continue;
      }
      Vec d = b.pos - a.pos;
      float distance = Length(d);
      if (distance >= 20) {
        continue;
      }
      Vec normal = distance > .001f ? d * (1 / distance) : Vec{1, 0};
      float overlap = 20 - distance;
      float share = a.id == talking_npc ? 0 : b.id == talking_npc ? 1 : .5f;
      Vec old_a = a.pos, old_b = b.pos;
      a.pos = Move(a.pos, normal * (-overlap * share), save, 10);
      b.pos = Move(b.pos, normal * (overlap * (1 - share)), save, 10);
      a.previous = a.previous + (a.pos - old_a);
      b.previous = b.previous + (b.pos - old_b);
    }
  }
  for (auto& [coord, c] : chunks) {
    for (auto& a : c.animals) {
      if (Length(a.home - player) < 1100) {
        a.previous = a.pos;
        Vec target =
            a.home +
            Vec{std::sin(static_cast<float>(time) * .21f + a.phase) * 70,
                std::cos(static_cast<float>(time) * .17f + a.phase) * 60};
        if (Length(a.pos - player) < 95) {
          target = a.pos + Normalized(a.pos - player) * 120;
        }
        Vec d = target - a.pos;
        a.pos =
            Move(a.pos, Normalized(d) * std::min(Length(d), dt * 48), save, 6);
        if (Length(a.pos - a.home) > 150) {
          a.pos = a.home + Normalized(a.pos - a.home) * 150;
        }
        a.walk += Length(a.pos - a.previous) / 8;
      }
    }
  }
}
const Resource* World::Nearest(Vec position,
                               const std::unordered_set<uint64_t>& removed,
                               float reach) const {
  const Resource* out = nullptr;
  float best = reach;
  Coord center = ChunkAt(position);
  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
      auto it = chunks.find({center.x + x, center.y + y});
      if (it == chunks.end()) {
        continue;
      }
      for (auto& resource : it->second.resources) {
        if (!removed.contains(resource.id)) {
          float d = Length(resource.pos - position);
          if (d < best) {
            best = d;
            out = &resource;
          }
        }
      }
    }
  }
  return out;
}
const Resource* World::Nearest(Vec position, const Save& save,
                               float reach) const {
  if (!save.submerged) {
    return Nearest(position, save.harvested, reach);
  }
  const Resource* found = nullptr;
  float best = reach;
  auto center = ChunkAt(position);
  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
      auto it = chunks.find({center.x + x, center.y + y});
      if (it == chunks.end()) {
        continue;
      }
      for (auto& resource : it->second.underwater) {
        if (!save.harvested.contains(resource.id)) {
          float d = Length(resource.pos - position);
          if (d < best) {
            best = d;
            found = &resource;
          }
        }
      }
    }
  }
  return found;
}
void World::BeginConversation(int id, const Save& save) {
  talking_npc = id;
  if (id < 0 || id >= static_cast<int>(npcs.size())) {
    return;
  }
  auto& npc = npcs[id];
  if (npc.ruler) {
    return;
  }
  if (Length(npc.pos - save.player) > 85) {
    for (int i = 0; i < 8; i++) {
      float a = i * .785398f;
      Vec position = save.player + Vec{std::cos(a) * 62, std::sin(a) * 62};
      if (Clear(position, save, 10)) {
        npc.pos = position;
        break;
      }
    }
  }
  npc.previous = npc.pos;
  npc.walk = 0;
}
bool World::Clear(Vec position, const Save& save, float radius,
                  bool boat) const {
  int lx = static_cast<int>(std::floor((position.x - radius) / kTileSize)),
      hx = static_cast<int>(std::floor((position.x + radius) / kTileSize));
  int ly = static_cast<int>(std::floor((position.y - radius) / kTileSize)),
      hy = static_cast<int>(std::floor((position.y + radius) / kTileSize));
  for (int y = ly; y <= hy; y++) {
    for (int x = lx; x <= hx; x++) {
      BoxShape tile{{x * kTileSize, y * kTileSize},
                    {(x + 1) * kTileSize, (y + 1) * kTileSize}};
      if (!CircleBox(position, radius, tile)) {
        continue;
      }
      auto center = (tile.low + tile.high) * .5f;
      Coord coord = ChunkAt(center);
      auto it = chunks.find(coord);
      if (it == chunks.end()) {
        return false;
      }
      int tx = x - coord.x * kChunkTiles, ty = y - coord.y * kChunkTiles;
      auto terrain = it->second.tiles[ty * kChunkTiles + tx].terrain;
      if (save.submerged && terrain != Terrain::kDeep &&
          terrain != Terrain::kShallow) {
        return false;
      }
      if (!save.submerged && !boat && terrain == Terrain::kDeep &&
          !RoadAt(save, position, radius)) {
        return false;
      }
    }
  }
  if (save.submerged) {
    return true;
  }
  auto close = [&](Vec q, float r) {
    Vec d = position - q;
    return d.x * d.x + d.y * d.y < r * r;
  };
  for (auto& b : buildings) {
    if (CircleBox(position, radius, BuildingFootprint(b)) &&
        CirclePolygon(position, radius, BuildingOutline(b))) {
      return false;
    }
  }
  for (auto& wall : save.walls) {
    if (wall.hp > 0) {
      if (CircleCapsule(position, radius, {wall.a, wall.b, 7})) {
        return false;
      }
    }
  }
  auto low = ChunkAt(position - Vec{radius + 18, radius + 18}),
       high = ChunkAt(position + Vec{radius + 18, radius + 18});
  for (int y = low.y; y <= high.y; y++) {
    for (int x = low.x; x <= high.x; x++) {
      auto it = chunks.find({x, y});
      if (it == chunks.end()) {
        continue;
      }
      for (auto& resource : it->second.resources) {
        if (resource.kind != ResourceKind::kTree &&
            resource.kind != ResourceKind::kStone &&
            resource.kind != ResourceKind::kOre) {
          continue;
        }
        if (save.harvested.contains(resource.id) ||
            RoadAt(save, resource.pos)) {
          continue;
        }
        const std::array<Vec, 6> rock = {
            resource.pos + Vec{-14, -6}, resource.pos + Vec{-5, -13},
            resource.pos + Vec{10, -10}, resource.pos + Vec{15, 4},
            resource.pos + Vec{4, 14},   resource.pos + Vec{-12, 10}};
        bool overlap = resource.kind == ResourceKind::kTree
                           ? close(resource.pos, radius + 9)
                           : CirclePolygon(position, radius, rock);
        if (overlap) {
          return false;
        }
      }
    }
  }
  return true;
}
void World::SyncInfrastructure(const Save& save) {
  size_t built =
      std::count_if(buildings.begin(), buildings.end(),
                    [](const auto& b) { return !b.facility.empty(); });
  if (built == save.facilities.size()) {
    return;
  }
  std::erase_if(buildings, [](const auto& b) { return !b.facility.empty(); });
  for (const auto& f : save.facilities) {
    buildings.push_back({f.position, 0, false, f.kind});
  }
}
void World::ResolvePlayerContacts(Save& save) {
  if (save.submerged) {
    return;
  }
  auto terrain = TerrainAt(save.player, save.seed);
  if ((terrain == Terrain::kDeep || terrain == Terrain::kShallow) &&
      !BridgeAt(save, save.player)) {
    return;
  }
  for (auto& npc : npcs) {
    if (npc.ruler || !ResidentActive(save, npc.id)) {
      continue;
    }
    Vec d = save.player - npc.pos;
    float distance = Length(d);
    if (distance >= 22) {
      continue;
    }
    Vec normal = distance > .001f ? d * (1 / distance) : Vec{1, 0};
    float overlap = 22 - distance;
    if (npc.id != talking_npc) {
      Vec old = npc.pos;
      npc.pos = Move(npc.pos, normal * (-overlap * .5f), save, 10);
      npc.previous = npc.previous + (npc.pos - old);
    }
    save.player = Move(
        save.player, normal * std::max(0.f, 22 - Length(save.player - npc.pos)),
        save, 12);
  }
  for (auto& [coord, chunk] : chunks) {
    for (auto& enemy : chunk.enemies) {
      if (save.defeated.contains(enemy.id)) {
        continue;
      }
      Vec d = save.player - enemy.pos;
      float distance = Length(d);
      if (distance < 22 && distance > .001f) {
        save.player =
            Move(save.player, d * ((22 - distance) / distance), save, 12);
      }
    }
  }
}
bool World::SightBlocked(Vec start, Vec end, const Save& save) const {
  if (save.submerged) {
    return false;
  }
  for (const auto& b : buildings) {
    if (SegmentBox(start, end, BuildingFootprint(b)) &&
        SegmentPolygon(start, end, BuildingOutline(b))) {
      return true;
    }
  }
  for (const auto& w : save.walls) {
    if (w.hp > 0) {
      BoxShape bounds{{std::min(w.a.x, w.b.x) - 7, std::min(w.a.y, w.b.y) - 7},
                      {std::max(w.a.x, w.b.x) + 7, std::max(w.a.y, w.b.y) + 7}};
      if (!SegmentBox(start, end, bounds)) {
        continue;
      }
      if (SegmentsDistance(start, end, w.a, w.b) < 7) {
        return true;
      }
    }
  }
  return false;
}
Vec World::Move(Vec position, Vec displacement, const Save& save, float radius,
                bool boat) const {
  int steps =
      std::clamp(static_cast<int>(std::ceil(Length(displacement) / 6)), 1, 256);
  Vec step = displacement * (1.f / steps);
  for (int i = 0; i < steps; i++) {
    if (Clear(position + step, save, radius, boat)) {
      position = position + step;
    } else {
      if (Clear(position + Vec{step.x, 0}, save, radius, boat)) {
        position.x += step.x;
      }
      if (Clear(position + Vec{0, step.y}, save, radius, boat)) {
        position.y += step.y;
      }
    }
  }
  return position;
}
std::vector<Vec> World::Path(Vec start, Vec goal, const Save& save) const {
  // Short plans keep work bounded; a distant goal is approached in successive
  // segments.
  if (Length(goal - start) > 900) {
    goal = start + Normalized(goal - start) * 900;
  }
  auto cell = [](Vec position) {
    return Coord{static_cast<int>(std::floor(position.x / kTileSize)),
                 static_cast<int>(std::floor(position.y / kTileSize))};
  };
  auto center = [](Coord coord) {
    return Vec{(coord.x + .5f) * kTileSize, (coord.y + .5f) * kTileSize};
  };
  Coord from = cell(start), to = cell(goal);
  if (!Clear(center(to), save, 12,
             (save.inventory.boat || save.inventory.ship ||
              save.inventory.submarine || !save.vessels.empty()))) {
    float best = 1e9;
    Coord candidate = from;
    for (int y = -5; y <= 5; y++) {
      for (int x = -5; x <= 5; x++) {
        Coord coord{to.x + x, to.y + y};
        float d = Length(center(coord) - goal);
        if (d < best &&
            Clear(center(coord), save, 12,
                  (save.inventory.boat || save.inventory.ship ||
                   save.inventory.submarine || !save.vessels.empty()))) {
          best = d;
          candidate = coord;
        }
      }
    }
    to = candidate;
  }
  struct Node {
    float rank;
    Coord c;
    bool operator<(const Node& b) const {
      return rank > b.rank;
    }
  };
  struct Record {
    float cost;
    Coord parent;
  };
  std::priority_queue<Node> open;
  std::unordered_map<Coord, Record, CoordHash> records;
  open.push({0, from});
  records.emplace(from, Record{0, from});
  Coord reached = from;
  float nearest = Length(center(from) - center(to));
  int visited = 0;
  while (!open.empty() && visited++ < 1800) {
    Coord coord = open.top().c;
    open.pop();
    float cost = records.at(coord).cost;
    float d = Length(center(coord) - center(to));
    if (d < nearest) {
      nearest = d;
      reached = coord;
    }
    if (coord == to) {
      reached = coord;
      break;
    }
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        if (!x && !y) {
          continue;
        }
        Coord next{coord.x + x, coord.y + y};
        if (std::abs(next.x - from.x) > 32 || std::abs(next.y - from.y) > 32) {
          continue;
        }
        if (!Clear(center(next), save, 12,
                   (save.inventory.boat || save.inventory.ship ||
                    save.inventory.submarine || !save.vessels.empty()))) {
          continue;
        }
        if (x && y &&
            (!Clear(center({coord.x + x, coord.y}), save, 12,
                    (save.inventory.boat || save.inventory.ship ||
                     save.inventory.submarine)) ||
             !Clear(center({coord.x, coord.y + y}), save, 12,
                    (save.inventory.boat || save.inventory.ship ||
                     save.inventory.submarine || !save.vessels.empty())))) {
          continue;
        }
        float weight =
            (x && y ? 1.4142f : 1.f) *
            (TerrainAt(center(next), seed_) == Terrain::kShallow ? 1.8f : 1.f);
        float candidate = cost + weight;
        auto found = records.find(next);
        if (found != records.end() && found->second.cost <= candidate) {
          continue;
        }
        records.insert_or_assign(next, Record{candidate, coord});
        open.push(
            {candidate + Length(center(next) - center(to)) / kTileSize, next});
      }
    }
  }
  std::vector<Vec> out;
  while (reached != from) {
    out.push_back(center(reached));
    reached = records.at(reached).parent;
  }
  std::reverse(out.begin(), out.end());
  return out;
}
}  // namespace frontier
