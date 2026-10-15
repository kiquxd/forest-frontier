#include "settlement_system.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#include "adventure.hpp"
#include "geometry.hpp"
#include "nation.hpp"
#include "progress.hpp"
#include "vessels.hpp"

namespace frontier {
namespace {
bool ValidCity(int city) {
  return city >= 0 && city < static_cast<int>(Settlements().size());
}
bool Peaceful(const Save& save, int city) {
  return ValidCity(city) && city < static_cast<int>(save.relations.size()) &&
         save.relations[city].treaty != Treaty::kWar &&
         (OwnsCity(save, city) || save.relations[city].opinion >= 0);
}
bool HasFacility(const Save& save, int city, const std::string& kind) {
  return std::any_of(
      save.facilities.begin(), save.facilities.end(),
      [&](const auto& f) { return f.city == city && f.kind == kind; });
}
Vec FacilitySite(const Save& save, int city) {
  auto buildings = MakeBuildings();
  for (int ring = 0; ring < 8; ++ring) {
    float radius = 125 + ring * 110;
    if (radius + 65 > CityRadius(save, city)) {
      break;
    }
    for (int slot = 0; slot < 16; ++slot) {
      float angle = (slot + .35f) * 6.2831853f / 16;
      Vec point = Settlements()[city].pos +
                  Vec{std::cos(angle), std::sin(angle)} * radius;
      bool dry = true;
      for (Vec offset :
           {Vec{}, Vec{50, 0}, Vec{-50, 0}, Vec{0, 50}, Vec{0, -50}}) {
        auto t = TerrainAt(point + offset, save.seed);
        dry = dry && t != Terrain::kDeep && t != Terrain::kShallow;
      }
      bool occupied = std::any_of(
          buildings.begin(), buildings.end(),
          [&](const auto& b) { return Length(b.pos - point) < 100; });
      occupied =
          occupied || std::any_of(save.facilities.begin(),
                                  save.facilities.end(), [&](const auto& f) {
                                    return Length(f.position - point) < 100;
                                  });
      occupied =
          occupied ||
          std::any_of(save.walls.begin(), save.walls.end(), [&](const auto& w) {
            return w.hp > 0 && SegmentDistance(point, w.a, w.b) < 70;
          });
      if (dry && !occupied) {
        return point;
      }
    }
  }
  return {std::numeric_limits<float>::quiet_NaN(), 0};
}
void RemoveWeapon(Save& save, const std::string& id) {
  if (id.empty()) {
    return;
  }
  auto& count = save.weapon_copies[id];
  count = std::max(0, count - 1);
  if (count == 0) {
    save.inventory.weapons.erase(id);
    if (save.inventory.weapons.empty()) {
      save.inventory.sword = false;
    }
  }
}
}  // namespace
const std::vector<FacilityDef>& FacilityTypes() {
  static const std::vector<FacilityDef> kTypes = {
      {"workbench", "Workbench", 12, 4, 5},
      {"forge", "Forge", 25, 25, 30},
      {"tailor", "Tailor workshop", 20, 8, 20},
      {"school", "School", 35, 20, 40},
      {"shipyard", "Shipyard", 50, 15, 60},
      {"alchemy", "Alchemy workshop", 20, 15, 25},
      {"infirmary", "Infirmary", 20, 10, 20},
      {"hospital", "Hospital", 50, 40, 60}};
  return kTypes;
}
void InitializeTowns(Save& save) {
  if (!save.campaign || save.urban) {
    return;
  }
  save.city_radii.assign(Settlements().size(), 350);
  save.previous_owners = save.owners;
  for (int city = 0; city < static_cast<int>(save.previous_owners.size());
       ++city) {
    if (save.previous_owners[city] == 0 && city != save.home_city) {
      save.previous_owners[city] = city + 1;
    }
  }
  for (int city = 0; city < static_cast<int>(Settlements().size()); ++city) {
    auto add = [&](const std::string& kind) {
      Vec point = FacilitySite(save, city);
      if (std::isfinite(point.x)) {
        save.facilities.push_back({city, kind, point});
      }
    };
    add("workbench");
    if (!OwnsCity(save, city)) {
      add("school");
      const auto& tech =
          GetContent().towns[Settlements()[city].type].technology;
      add(tech == "shipbuilding" || tech == "diving"        ? "shipyard"
          : tech == "ironworking" || tech == "steelworking" ? "forge"
          : tech == "crystallurgy"                          ? "alchemy"
                                                            : "tailor");
    }
  }
  save.urban = true;
}
float CityRadius(const Save& save, int city) {
  return city >= 0 && city < static_cast<int>(save.city_radii.size())
             ? save.city_radii[city]
             : 350;
}
int CityAt(const Save& save, Vec point) {
  if (save.submerged) {
    return -1;
  }
  int found = -1;
  float best = std::numeric_limits<float>::max();
  for (int i = 0; i < static_cast<int>(Settlements().size()); ++i) {
    float distance = Length(point - Settlements()[i].pos);
    if (distance <= CityRadius(save, i) && distance < best) {
      best = distance;
      found = i;
    }
  }
  return found;
}
bool ResidentActive(const Save& save, int npc) {
  auto resident = GetCitizen(save, npc);
  return !resident || resident->state == "active";
}
int ActiveResidents(const Save& save) {
  return static_cast<int>(std::count_if(
      save.citizens.begin(), save.citizens.end(), [&](const auto& c) {
        return c.state == "active" && OwnsCity(save, c.city);
      }));
}
bool RoadAt(const Save& save, Vec point, float radius) {
  for (const auto& road : save.roads) {
    for (size_t i = 1; i < road.points.size(); ++i) {
      if (SegmentDistance(point, road.points[i - 1], road.points[i]) <=
          22 - radius) {
        return true;
      }
    }
  }
  return false;
}
bool BridgeAt(const Save& save, Vec point, float radius) {
  auto t = TerrainAt(point, save.seed);
  return (t == Terrain::kDeep || t == Terrain::kShallow) &&
         RoadAt(save, point, radius);
}
bool FacilityAccess(const Save& save, int city, const std::string& kind) {
  if (!save.campaign) {
    return true;  // Compatibility for pre-campaign test/sandbox state.
  }
  if (!Peaceful(save, city)) {
    return false;
  }
  std::vector<bool> seen(Settlements().size());
  std::queue<int> todo;
  todo.push(city);
  seen[city] = true;
  while (!todo.empty()) {
    int current = todo.front();
    todo.pop();
    if (HasFacility(save, current, kind)) {
      return true;
    }
    for (const auto& road : save.roads) {
      int next = road.from == current ? road.to
                 : road.to == current ? road.from
                                      : -1;
      if (next >= 0 && !seen[next] && Peaceful(save, next)) {
        seen[next] = true;
        todo.push(next);
      }
    }
  }
  return false;
}
std::string RecipeFacility(const Recipe& recipe) {
  const auto& result = recipe.result;
  if (result.starts_with("outfit:") || result == "armor") {
    return "tailor";
  }
  if (result.starts_with("vessel:") || result == "boat" || result == "ship" ||
      result == "submarine") {
    return "shipyard";
  }
  if (result == "potion") {
    return "alchemy";
  }
  if (result == "steel" || result == "pickaxe" || result == "sonar" ||
      result == "oxygen" ||
      (result.starts_with("weapon:") && Weapon(result.substr(7)).rarity > 0)) {
    return "forge";
  }
  return "workbench";
}
int RecipeRarity(const Recipe& recipe) {
  if (recipe.result.starts_with("weapon:")) {
    return Weapon(recipe.result.substr(7)).rarity;
  }
  if (recipe.result.starts_with("outfit:")) {
    const auto outfit = Outfit(recipe.result.substr(7));
    return outfit.quest_only ? 2 : outfit.technology.empty() ? 0 : 1;
  }
  if (recipe.result.starts_with("vessel:")) {
    return std::clamp(Vessel(recipe.result.substr(7)).prestige / 2, 0, 4);
  }
  if (recipe.result == "submarine" || recipe.result == "oxygen" ||
      recipe.result == "sonar") {
    return 3;
  }
  if (recipe.result == "ship" || recipe.result == "steel" ||
      recipe.quest_only) {
    return 2;
  }
  return recipe.technology.empty() ? 0 : 1;
}
double CraftDelay(const Recipe& recipe) {
  static constexpr double kSeconds[] = {60, 300, 1800, 3600, kMaxCraftCooldown};
  return kSeconds[std::clamp(RecipeRarity(recipe), 0, 4)];
}
double CraftWait(const Save& save) {
  return std::max(0.0, save.craft_ready_at - save.game_time);
}
Action BuildFacility(Save& save, int city, const std::string& kind) {
  InitializeTowns(save);
  if (!ValidCity(city)) {
    return {false, "Invalid town"};
  }
  int& wood = save.nation ? save.stocks[city][0] : save.inventory.wood;
  int& stone = save.nation ? save.stocks[city][1] : save.inventory.stone;
  auto it = std::find_if(FacilityTypes().begin(), FacilityTypes().end(),
                         [&](const auto& f) { return f.id == kind; });
  if (!ValidCity(city) || !OwnsCity(save, city) ||
      it == FacilityTypes().end()) {
    return {false, "Choose a building in your own town"};
  }
  if (!HasSkill(save, "construction")) {
    return {false, "Learn Construction first"};
  }
  int count = std::count_if(
      save.facilities.begin(), save.facilities.end(),
      [&](const auto& f) { return f.city == city && f.kind == kind; });
  if (count >= (save.nation ? FacilityLimit(save, city, kind) : 1)) {
    return {false, "This town already has that building"};
  }
  if (kind == "hospital" && !save.technologies.contains("hospitals")) {
    return {false, "Research Hospitals first"};
  }
  Vec point = FacilitySite(save, city);
  if (!std::isfinite(point.x)) {
    return {false, "No safe building site; expand the town first"};
  }
  if (wood < it->wood || stone < it->stone || save.coins < it->coins) {
    return {false, "Requires wood " + std::to_string(it->wood) + ", stone " +
                       std::to_string(it->stone) + ", coins " +
                       std::to_string(it->coins)};
  }
  wood -= it->wood;
  stone -= it->stone;
  save.coins -= it->coins;
  if (save.nation) {
    save.region_fund += it->coins;
  }
  save.facilities.push_back(
      {city, kind, point, save.nation ? save.next_entity++ : 0});
  RecordHistory(save, "building", "Built " + it->name, city);
  return {true, "Built: " + it->name};
}
Action ExpandCity(Save& save, int city) {
  InitializeTowns(save);
  if (!ValidCity(city)) {
    return {false, "Invalid town"};
  }
  int& wood = save.nation ? save.stocks[city][0] : save.inventory.wood;
  int& stone = save.nation ? save.stocks[city][1] : save.inventory.stone;
  if (!ValidCity(city) || !OwnsCity(save, city) ||
      !HasSkill(save, "construction")) {
    return {false, "Expansion requires your own town and Construction"};
  }
  float radius = CityRadius(save, city);
  int level = static_cast<int>((radius - 350) / 150) + 1;
  if (radius >= 1100) {
    return {false, "Town territory is fully expanded"};
  }
  for (int other = 0; other < static_cast<int>(Settlements().size()); ++other) {
    if (other != city &&
        Length(Settlements()[city].pos - Settlements()[other].pos) <
            radius + 150 + CityRadius(save, other)) {
      return {false, "Expansion would overlap another town"};
    }
  }
  if (wood < level * 15 || stone < level * 10 || save.coins < level * 25) {
    return {false, "Expansion requires wood " + std::to_string(level * 15) +
                       ", stone " + std::to_string(level * 10) + ", coins " +
                       std::to_string(level * 25)};
  }
  wood -= level * 15;
  stone -= level * 10;
  save.coins -= level * 25;
  if (save.nation) {
    save.region_fund += level * 25;
  }
  save.city_radii[city] += 150;
  RecordHistory(save, "territory", "Expanded town territory", city);
  return {true, "Town radius increased to " +
                    std::to_string(static_cast<int>(save.city_radii[city]))};
}
RoadQuote QuoteRoad(const Save& save, int from, int to,
                    const std::vector<Vec>& points) {
  RoadQuote out;
  auto fail = [&](const std::string& message) {
    out.message = message;
    return out;
  };
  if (!ValidCity(from) || !ValidCity(to) || from == to ||
      !OwnsCity(save, from) || !Peaceful(save, to) ||
      !save.relations[to].discovered) {
    return fail("Start at your town and finish at a discovered peaceful town");
  }
  if (!HasSkill(save, "construction")) {
    return fail("Learn Construction first");
  }
  if (points.size() < 2 || points.size() > 64 || save.roads.size() >= 128) {
    return fail("A road needs 2–64 points; maximum 128 roads");
  }
  for (Vec p : points) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || std::abs(p.x) > 2e6 ||
        std::abs(p.y) > 2e6) {
      return fail("Invalid route point");
    }
  }
  if (Length(points.front() - Settlements()[from].pos) > 1 ||
      Length(points.back() - Settlements()[to].pos) > 1) {
    return fail("The full road must connect both town centres");
  }
  for (const auto& r : save.roads) {
    if ((r.from == from && r.to == to) || (r.from == to && r.to == from)) {
      return fail("These towns already have a road");
    }
  }
  double total_length = 0;
  for (size_t i = 1; i < points.size(); ++i) {
    float length = Length(points[i] - points[i - 1]);
    total_length += length;
    if (length < 1 || total_length > 50000) {
      return fail("Route segments must be nonzero; maximum length 50000");
    }
    int steps = static_cast<int>(std::ceil(length / 24));
    double land = 0, bridge = 0;
    for (int j = 0; j <= steps; ++j) {
      Vec p = points[i - 1] +
              (points[i] - points[i - 1]) * (static_cast<float>(j) / steps);
      if (!ExploredAt(save.explored, p)) {
        return fail("Explore the entire route before building");
      }
      if (j == steps) {
        continue;
      }
      auto t = TerrainAt(p, save.seed);
      if (t == Terrain::kDeep || t == Terrain::kShallow) {
        bridge += length / steps;
      } else {
        land += length / steps;
      }
    }
    out.land_tiles += static_cast<int>(std::ceil(land / kTileSize));
    out.bridge_tiles += static_cast<int>(std::ceil(bridge / kTileSize));
  }
  out.wood = out.land_tiles + out.bridge_tiles * 4;
  out.stone = out.land_tiles + out.bridge_tiles * 3;
  out.ok = true;
  out.message = "Wood " + std::to_string(out.wood) + " · Stone " +
                std::to_string(out.stone) + " · Bridge tiles " +
                std::to_string(out.bridge_tiles);
  return out;
}
Action BuildRoad(Save& save, int from, int to, const std::vector<Vec>& points) {
  auto quote = QuoteRoad(save, from, to, points);
  if (!quote.ok) {
    return {false, quote.message};
  }
  int& wood = save.nation ? save.stocks[from][0] : save.inventory.wood;
  int& stone = save.nation ? save.stocks[from][1] : save.inventory.stone;
  if (wood < quote.wood || stone < quote.stone) {
    return {false, "Not enough materials: " + quote.message};
  }
  // Commit the entire route only after every condition and cost has passed.
  wood -= quote.wood;
  stone -= quote.stone;
  save.roads.push_back({from, to, points});
  RecordHistory(
      save, "road",
      "Connected " + Settlements()[from].name + " to " + Settlements()[to].name,
      from);
  return {true, "Complete road built; connected buildings are available"};
}
Action LoseResident(Save& save, int npc, bool captured, int captor) {
  StoreControlled(save);
  auto c = GetCitizen(save, npc);
  if (!c || c->state != "active" || !OwnsCity(save, c->city)) {
    return {false, "Resident is unavailable"};
  }
  if (captured && (!ValidCity(captor) || OwnsCity(save, captor))) {
    return {false, "Invalid captor"};
  }
  c->health = 0;
  if (captured) {
    c->state = "captive";
    c->captor = captor;
    c->ransom =
        30 + (c->weapon.empty() ? 0 : 20 * (Weapon(c->weapon).rarity + 1));
  } else {
    if (save.nation) {
      for (const auto& [id, count] : c->weapons) {
        save.weapon_copies[id] -= count;
      }
      for (const auto& [id, count] : c->clothes) {
        save.outfit_copies[id] -= count;
      }
      c->weapons.clear();
      c->clothes.clear();
      c->cargo = {};
      c->vessels.clear();
    } else {
      RemoveWeapon(save, c->weapon);
      if (!c->outfit.empty()) {
        save.outfit_copies[c->outfit] =
            std::max(0, save.outfit_copies[c->outfit] - 1);
      }
    }
    c->weapon.clear();
    c->outfit.clear();
    c->state = "dead";
    c->trained = false;
    c->job = "idle";
  }
  if (save.nation && npc == save.controlled_npc) {
    save.controlled_npc = -1;
    save.inventory = {};
    save.health = 100;
    save.submerged = false;
  }
  RecordHistory(save, captured ? "capture" : "loss",
                captured ? "Resident taken captive; equipment held"
                         : "Resident died; assigned equipment lost",
                c->city);
  return {true, captured ? "Resident captured; pay ransom in the town roster"
                         : "Resident died; assigned gear was destroyed"};
}
Action RansomResident(Save& save, int npc) {
  auto c = GetCitizen(save, npc);
  if (!c || c->state != "captive" || !OwnsCity(save, c->city)) {
    return {false, "No captive resident to ransom"};
  }
  if (save.coins < c->ransom ||
      save.treasury[c->captor] > 10000000 - c->ransom) {
    return {false, "Ransom requires " + std::to_string(c->ransom) + " coins"};
  }
  save.coins -= c->ransom;
  save.treasury[c->captor] += c->ransom;
  c->state = "active";
  c->health = 100;
  c->captor = -1;
  c->ransom = 0;
  if (save.nation) {
    c->position = Settlements()[c->city].pos + Vec{120, 140};
    c->underwater = false;
  }
  RecordHistory(save, "ransom", "Resident returned with equipment", c->city);
  return {true, "Resident returned with their weapon and clothes"};
}
void DestroyPlayerWeapon(Save& save) {
  RemoveWeapon(save, save.inventory.weapon_id);
  save.inventory.weapon_id.clear();
  save.inventory.sword = false;
}
Action SurrenderTown(Save& save, int city, int patron) {
  InitializeTowns(save);
  if (!ValidCity(city) || !ValidCity(patron) || !OwnsCity(save, city) ||
      OwnsCity(save, patron) || save.relations[patron].treaty != Treaty::kWar) {
    return {false, "Surrender requires your town and a hostile foreign ruler"};
  }
  if (std::any_of(save.tributes.begin(), save.tributes.end(),
                  [&](const auto& t) { return t.city == city; })) {
    return {false, "This town already pays tribute"};
  }
  if (save.coins < 30 || save.treasury[patron] > 10000000 - 30) {
    return {false,
            "Surrender requires 30 coins now, then 10 every 120 game seconds"};
  }
  save.coins -= 30;
  save.treasury[patron] += 30;
  save.tributes.push_back({city, patron, save.previous_owners[city], 10, 300,
                           save.game_time + 120});
  save.relations[patron].treaty = Treaty::kTruce;
  save.relations[patron].opinion = std::max(0, save.relations[patron].opinion);
  if (save.home_city >= 0) {
    save.city_opinions[save.home_city][patron] =
        save.city_opinions[patron][save.home_city] =
            save.relations[patron].opinion;
  }
  RecordHistory(save, "tribute",
                "Surrendered under tribute to " + Settlements()[patron].name,
                city);
  return {true, "Local rule retained; tribute 10 coins per 120 game seconds"};
}
Action BuyIndependence(Save& save, int city, bool restore_previous) {
  auto it = std::find_if(save.tributes.begin(), save.tributes.end(),
                         [&](const auto& t) { return t.city == city; });
  if (it == save.tributes.end() || !OwnsCity(save, city)) {
    return {false, "This town has no tribute obligation"};
  }
  if (save.coins < it->buyout ||
      save.treasury[it->patron] > 10000000 - it->buyout) {
    return {false, "Freedom costs " + std::to_string(it->buyout) + " coins"};
  }
  save.coins -= it->buyout;
  save.treasury[it->patron] += it->buyout;
  save.owners[city] = restore_previous ? it->previous_owner : 0;
  save.tributes.erase(it);
  RecordHistory(save, "independence",
                restore_previous ? "Returned to the previous ruler"
                                 : "Bought independence",
                city);
  return {true, restore_previous ? "Previous allegiance restored; tribute ended"
                                 : "Independent again; tribute ended"};
}
void TickTowns(Save& save, float dt) {
  if (!std::isfinite(dt) || dt <= 0) {
    return;
  }
  save.game_time += dt;
  for (auto it = save.tributes.begin(); it != save.tributes.end();) {
    if (save.game_time < it->next_due) {
      ++it;
      continue;
    }
    if (save.coins >= it->amount) {
      save.coins -= it->amount;
      save.treasury[it->patron] += it->amount;
      it->next_due += 120;
      RecordHistory(save, "tribute",
                    "Paid tribute: " + std::to_string(it->amount) + " coins",
                    it->city);
      ++it;
    } else {
      save.relations[it->patron].treaty = Treaty::kWar;
      save.relations[it->patron].opinion = -50;
      if (save.home_city >= 0) {
        save.city_opinions[save.home_city][it->patron] =
            save.city_opinions[it->patron][save.home_city] = -50;
      }
      RecordHistory(save, "tribute",
                    "Tribute unpaid; local hostilities resumed", it->city);
      it = save.tributes.erase(it);
    }
  }
}
std::string TownInfrastructure(const Save& save, int city) {
  std::string out = "Radius " +
                    std::to_string(static_cast<int>(CityRadius(save, city))) +
                    " · Buildings: ";
  for (const auto& f : save.facilities) {
    if (f.city == city) {
      out += f.kind + " ";
    }
  }
  for (const auto& t : save.tributes) {
    if (t.city == city) {
      out += "· Tribute to " + Settlements()[t.patron].name + ": " +
             std::to_string(t.amount) + "/120s; freedom " +
             std::to_string(t.buyout);
    }
  }
  return out;
}
}  // namespace frontier
