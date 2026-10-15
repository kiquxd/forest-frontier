#include "nation.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>

#include "adventure.hpp"
#include "progress.hpp"
#include "settlement_system.hpp"
#include "vessels.hpp"

namespace frontier {
namespace {
bool ValidCity(const Save& save, int city) {
  return city >= 0 && city < static_cast<int>(save.stocks.size());
}
bool PeacefulCity(const Save& save, int city) {
  return ValidCity(save, city) && save.relations[city].treaty != Treaty::kWar &&
         (OwnsCity(save, city) || save.relations[city].opinion >= 0);
}
int RecipeIndex(const std::string& id) {
  for (int i = 0; i < static_cast<int>(Recipes().size()); ++i) {
    if (Recipes()[i].id == id) {
      return i;
    }
  }
  return -1;
}
bool ReachableFacility(const Save& save, int source, int city) {
  return PeacefulCity(save, source) && PeacefulCity(save, city) &&
         (source == city || ConnectedCities(save, source, city));
}
void LoadControlled(Save& save, Citizen& c) {
  save.controlled_npc = c.npc;
  save.player = c.position;
  save.inventory = c.cargo;
  save.inventory.weapons.clear();
  for (const auto& [id, count] : c.weapons) {
    if (count > 0) {
      save.inventory.weapons.insert(id);
    }
  }
  save.inventory.weapon_id = c.weapon;
  save.inventory.sword = !c.weapon.empty();
  save.outfit_id = c.outfit.empty() ? "traveler" : c.outfit;
  save.character_id = c.background;
  save.health = std::max(1, c.health);
  save.submerged = c.underwater;
  save.oxygen = c.air;
  save.hull = c.hull;
  save.vehicle = c.vehicle;
  save.vessels = c.vessels;
}
std::string CanonicalProduct(const std::string& result) {
  return result == "armor" ? "outfit:leather_armor" : result;
}
int Incoming(const Save& save, int city, int good) {
  int amount = 0;
  for (const auto& shipment : save.shipments) {
    if (shipment.to == city && shipment.good == good) {
      amount += shipment.quantity;
    }
  }
  for (const auto& task : save.city_tasks) {
    if (!task.completed && task.kind == "trade" && task.city == city &&
        task.good == good) {
      amount += task.quantity;
    }
  }
  return amount;
}
void UnlockTaskReward(Save& save, const std::string& reward) {
  if (reward.starts_with("recipe:")) {
    save.known_recipes.insert(reward.substr(7));
  } else if (reward.starts_with("technology:")) {
    save.technologies.insert(reward.substr(11));
  }
}
}  // namespace

bool HasControlled(const Save& save) {
  const auto* c = GetCitizen(save, save.controlled_npc);
  return !save.nation || (c && c->state == "active" && OwnsCity(save, c->city));
}
int64_t MoneySupply(const Save& save) {
  int64_t total = save.coins + static_cast<int64_t>(save.region_fund);
  for (int coins : save.treasury) {
    total += coins;
  }
  for (const auto& task : save.city_tasks) {
    total += task.escrow;
  }
  for (const auto& job : save.production) {
    total += job.fee;
  }
  return total;
}
std::string ResidentName(const Save& save, int npc) {
  for (const auto& t : save.travelers) {
    if (t.npc == npc) {
      return t.name;
    }
  }
  static uint64_t cached_seed = 0;
  static std::vector<Npc> people;
  if (people.empty() || cached_seed != save.seed) {
    people = MakeNpcs();
    cached_seed = save.seed;
  }
  return npc >= 0 && npc < static_cast<int>(people.size())
             ? people[npc].name
             : "Resident " + std::to_string(npc);
}
void StoreControlled(Save& save) {
  if (!save.nation) {
    return;
  }
  auto* c = GetCitizen(save, save.controlled_npc);
  if (!c || c->state != "active") {
    return;
  }
  c->position = save.player;
  c->cargo = save.inventory;
  c->weapon = save.inventory.weapon_id;
  c->outfit = save.outfit_id;
  c->background = save.character_id;
  c->health = save.health;
  c->underwater = save.submerged;
  c->air = save.oxygen;
  c->hull = save.hull;
  c->vehicle = save.vehicle;
  c->vessels = save.vessels;
}
void InitializeNation(Save& save) {
  if (!save.campaign || save.nation) {
    return;
  }
  InitializeAdventure(save);
  auto npcs = MakeNpcs();
  save.city_products.resize(Settlements().size());
  save.housing.resize(Settlements().size(), 8);
  int first = -1;
  for (auto& c : save.citizens) {
    c.position = npcs[c.npc].pos;
    c.background = "traveler";
    if (first < 0 && c.state == "active" && c.city == save.home_city) {
      first = c.npc;
    }
    if (!c.weapon.empty()) {
      c.weapons[c.weapon] = 1;
      c.cargo.weapons.insert(c.weapon);
      c.cargo.weapon_id = c.weapon;
    }
    if (!c.outfit.empty()) {
      c.clothes[c.outfit] = 1;
    } else if (c.state != "dead") {
      c.outfit = "traveler";
      c.clothes["traveler"] = 1;
      ++save.outfit_copies["traveler"];
    }
    save.housing[c.city] =
        std::max(save.housing[c.city], CityPopulation(save, c.city));
  }
  if (first < 0) {
    throw std::runtime_error("No living resident in the starting town");
  }
  auto* c = GetCitizen(save, first);
  // Legacy loose goods move to the town; equipment follows the first resident.
  for (int g = 0; g < kGoodTypes; ++g) {
    int moved = std::min(GoodCount(save.inventory, g),
                         1000000 - save.stocks[save.home_city][g]);
    save.stocks[save.home_city][g] += moved;
    GoodCount(save.inventory, g) -= moved;
  }
  if (c->outfit == "traveler" && save.outfit_id == "traveler") {
    --save.outfit_copies["traveler"];  // The old avatar's copy is reused.
  }
  c->cargo = save.inventory;
  c->position = save.player;
  c->health = save.health;
  c->background = save.character_id;
  c->outfit = save.outfit_id;
  c->clothes[c->outfit] = std::max(1, c->clothes[c->outfit]);
  c->weapon = save.inventory.weapon_id;
  c->vessels = save.vessels;
  c->underwater = save.submerged;
  c->air = save.oxygen;
  c->hull = save.hull;
  c->vehicle = save.vehicle;
  // Allocate every legacy gear copy once, including copies carried as spares.
  for (const auto& [id, total] : save.weapon_copies) {
    int assigned = 0;
    for (const auto& resident : save.citizens) {
      if (resident.npc != first && resident.state != "dead" &&
          resident.weapon == id) {
        ++assigned;
      }
    }
    if (total > assigned) {
      c->weapons[id] = total - assigned;
    }
  }
  for (const auto& [id, total] : save.outfit_copies) {
    int assigned = 0;
    for (const auto& resident : save.citizens) {
      if (resident.npc != first && resident.state != "dead" &&
          resident.outfit == id) {
        ++assigned;
      }
    }
    if (total > assigned) {
      c->clothes[id] = total - assigned;
    }
  }
  save.region_fund = 5000;
  int grant = std::min(100, std::max(0, save.treasury[save.home_city] - 40));
  save.treasury[save.home_city] -= grant;
  save.coins += grant;
  save.nation = true;
  save.craft_ready_at = 0;
  LoadControlled(save, *c);
  uint64_t next = 1;
  for (auto& f : save.facilities) {
    f.id = next++;
  }
  save.next_entity = next;
  // Preserve accepted legacy objectives on the city board without coin rewards.
  for (const auto& [npc, progress] : save.quests) {
    const auto& q = Quest(progress.id);
    int city = npcs[npc].settlement;
    std::string reward = !q.recipe.empty()      ? "recipe:" + q.recipe
                         : !q.knowledge.empty() ? "technology:" + q.knowledge
                                                : "trust";
    save.city_tasks.push_back({save.next_entity++, city, "legacy:" + q.id,
                               q.good, q.target, 0, 0, true, progress.completed,
                               reward, progress.baseline, 0});
  }
  RefreshCityTasks(save);
  for (int city = 0; city < static_cast<int>(save.stocks.size()); ++city) {
    if (OwnsCity(save, city)) {
      for (int g = 0; g < kGoodTypes; ++g) {
        save.known_goods[g] = save.known_goods[g] || save.stocks[city][g] > 0;
      }
    }
  }
  save.money_supply = MoneySupply(save);
  RecordHistory(save, "government",
                "Established resident control and city warehouses",
                save.home_city);
}
Action SelectResident(Save& save, int npc) {
  if (!save.nation) {
    return {false, "Resident control is not enabled"};
  }
  StoreControlled(save);
  auto* c = GetCitizen(save, npc);
  if (!c || c->state != "active" || !OwnsCity(save, c->city)) {
    return {false, "Choose a living, free resident of your state"};
  }
  LoadControlled(save, *c);
  save.visible_surface.clear();
  save.visible_underwater.clear();
  return {true, "Controlling " + ResidentName(save, npc)};
}
void SyncNationActors(World& world, const Save& save) {
  if (!save.nation) {
    return;
  }
  for (const auto& t : save.travelers) {
    if (t.npc == static_cast<int>(world.npcs.size())) {
      Npc npc;
      npc.id = t.npc;
      npc.name = t.name;
      npc.role = "Wanderer";
      npc.personality = "I am looking for a town to call home.";
      npc.settlement = t.city;
      npc.pos = npc.previous = npc.home =
          Settlements()[t.city].pos + Vec{140, 80};
      npc.type = NpcType("forester");
      world.npcs.push_back(npc);
    }
  }
  for (const auto& c : save.citizens) {
    if (c.npc >= 0 && c.npc < static_cast<int>(world.npcs.size())) {
      auto& npc = world.npcs[c.npc];
      npc.settlement = c.city;
      npc.pos = npc.previous =
          c.npc == save.controlled_npc ? save.player : c.position;
      npc.home = c.position;
    }
  }
}
int CityPopulation(const Save& save, int city) {
  return static_cast<int>(std::count_if(
      save.citizens.begin(), save.citizens.end(),
      [&](const auto& c) { return c.city == city && c.state == "active"; }));
}
int FacilityLimit(const Save& save, int city, const std::string& kind) {
  return kind == "forge" ? std::max(1, CityPopulation(save, city) / 6) : 1;
}
bool ConnectedCities(const Save& save, int from, int to) {
  if (!PeacefulCity(save, from) || !PeacefulCity(save, to)) {
    return false;
  }
  std::vector<bool> seen(save.stocks.size());
  std::queue<int> todo;
  seen[from] = true;
  todo.push(from);
  while (!todo.empty()) {
    int city = todo.front();
    todo.pop();
    if (city == to) {
      return true;
    }
    for (const auto& road : save.roads) {
      int next = road.from == city ? road.to : road.to == city ? road.from : -1;
      if (next >= 0 && !seen[next] && PeacefulCity(save, next)) {
        seen[next] = true;
        todo.push(next);
      }
    }
  }
  return false;
}
Action TransferGoods(Save& save, int city, int good, int amount, bool deposit) {
  if (!HasControlled(save) || !OwnsCity(save, city) ||
      CityAt(save, save.player) != city || good < 0 || good >= kGoodTypes ||
      amount < 1 || amount > 1000000) {
    return {false, "Visit your town territory to transfer cargo"};
  }
  int& carried = GoodCount(save.inventory, good);
  int& stock = save.stocks[city][good];
  int& source = deposit ? carried : stock;
  int& destination = deposit ? stock : carried;
  if (source < amount || destination > 1000000 - amount) {
    return {false, "Insufficient stock or storage full"};
  }
  source -= amount;
  destination += amount;
  StoreControlled(save);
  RememberResources(save);
  return {true, deposit ? "Cargo deposited" : "Cargo withdrawn"};
}
Action TransferProduct(Save& save, int city, const std::string& raw,
                       bool deposit) {
  if (!HasControlled(save) || !OwnsCity(save, city) ||
      CityAt(save, save.player) != city) {
    return {false, "Visit your town to transfer equipment"};
  }
  StoreControlled(save);
  auto* c = GetCitizen(save, save.controlled_npc);
  std::string product = CanonicalProduct(raw);
  auto& stock = save.city_products[city][product];
  if (!deposit && stock < 1) {
    return {false, "No copy in this warehouse"};
  }
  auto exchange = [&](int& count) {
    if ((deposit && count < 1) || (!deposit && count >= 1000000)) {
      return false;
    }
    count += deposit ? -1 : 1;
    return true;
  };
  bool ok = false;
  if (product.starts_with("weapon:")) {
    auto id = product.substr(7);
    (void)Weapon(id);
    if (deposit && c->weapon == id && c->weapons[id] <= 1) {
      return {false, "Unequip this weapon before depositing it"};
    }
    ok = exchange(c->weapons[id]);
    if (ok && c->weapons[id] == 0) {
      save.inventory.weapons.erase(id);
    } else if (ok) {
      save.inventory.weapons.insert(id);
    }
  } else if (product.starts_with("outfit:")) {
    auto id = product.substr(7);
    (void)Outfit(id);
    if (deposit && save.outfit_id == id && c->clothes[id] <= 1) {
      return {false, "Change clothes before depositing this outfit"};
    }
    ok = exchange(c->clothes[id]);
  } else {
    bool* flag = product == "axe"         ? &save.inventory.axe
                 : product == "pickaxe"   ? &save.inventory.pickaxe
                 : product == "rod"       ? &save.inventory.rod
                 : product == "boat"      ? &save.inventory.boat
                 : product == "ship"      ? &save.inventory.ship
                 : product == "submarine" ? &save.inventory.submarine
                 : product == "sonar"     ? &save.inventory.sonar
                 : product == "oxygen"    ? &save.inventory.oxygen
                                          : nullptr;
    if (flag) {
      if (*flag == deposit) {
        *flag = !deposit;
        ok = true;
      }
    } else if (product.starts_with("vessel:")) {
      std::string id = product.substr(7);
      (void)Vessel(id);
      if (save.vessels.contains(id) == deposit) {
        if (deposit) {
          save.vessels.erase(id);
        } else {
          save.vessels.insert(id);
        }
        ok = true;
      }
    } else if (product == "potion") {
      ok = exchange(save.inventory.potions);
    } else if (product == "campfire") {
      ok = exchange(save.inventory.campfires);
    } else if (product == "ammo") {
      ok = exchange(save.inventory.ammo);
    } else if (product == "repair") {
      ok = exchange(save.inventory.repairs);
    }
  }
  if (!ok) {
    return {false, "No transferable copy, or already carrying this tool"};
  }
  stock += deposit ? 1 : -1;
  StoreControlled(save);
  return {true, "Equipment transferred"};
}
Action QueueProduction(Save& save, int recipe, int source, uint64_t facility) {
  if (!save.nation || recipe < 0 ||
      recipe >= static_cast<int>(Recipes().size())) {
    return {false, "Unknown production recipe"};
  }
  if (source < 0) {
    source = CityAt(save, save.player);
  }
  const auto& r = Recipes()[recipe];
  if (!OwnsCity(save, source)) {
    return {false, "Choose your town's warehouse as the source"};
  }
  if (!HasSkill(save, "crafting") || !HasSkill(save, RecipeSkill(r)) ||
      !RecipeKnown(save, r) ||
      (!r.technology.empty() && !save.technologies.contains(r.technology))) {
    return {false, "State skill, recipe or specialist knowledge is missing"};
  }
  const Facility* selected = nullptr;
  double earliest = 1e30;
  for (const auto& f : save.facilities) {
    if (f.kind != RecipeFacility(r) || (facility && f.id != facility) ||
        !ReachableFacility(save, source, f.city)) {
      continue;
    }
    int queued = 0;
    double ready = save.game_time;
    for (const auto& job : save.production) {
      if (job.facility == f.id) {
        ++queued;
        ready = std::max(ready, job.due);
      }
    }
    if (queued < 8 && ready < earliest) {
      selected = &f;
      earliest = ready;
    }
  }
  if (!selected) {
    return {false, "No accessible workshop with a free queue slot"};
  }
  if (r.result.starts_with("vessel:") &&
      Vessel(r.result.substr(7)).prestige >= 6 &&
      !save.technologies.contains("shipbuilding")) {
    return {false, "Learn Shipbuilding from a specialist town first"};
  }
  int fee =
      r.result.starts_with("vessel:") ? Vessel(r.result.substr(7)).coins : 0;
  if (save.coins < fee) {
    return {false, "State treasury cannot cover the commissioning fee"};
  }
  if (r.result == "vessel:flagship" &&
      !save.technologies.contains("navigation_school")) {
    return {false, "Navigation school is required"};
  }
  for (int g = 0; g < kGoodTypes; ++g) {
    if (save.stocks[source][g] < r.cost[g]) {
      return {false, "Warehouse needs " + std::string(GoodName(g))};
    }
  }
  for (int g = 0; g < kGoodTypes; ++g) {
    save.stocks[source][g] -= r.cost[g];
  }
  save.coins -= fee;
  save.production.push_back({save.next_entity++, selected->id, source, source,
                             r.id, earliest + CraftDelay(r), r.cost, fee});
  RecordHistory(save, "production", "Queued " + r.name, source);
  return {true, "Queued at " + Settlements()[selected->city].name +
                    "; output returns to the source warehouse"};
}
Action CancelProduction(Save& save, uint64_t id) {
  auto it = std::find_if(save.production.begin(), save.production.end(),
                         [&](const auto& j) { return j.id == id; });
  if (it == save.production.end()) {
    return {false, "Production job unavailable"};
  }
  for (int g = 0; g < kGoodTypes; ++g) {
    if (save.stocks[it->source][g] > 1000000 - it->materials[g]) {
      return {false, "Source warehouse is full; clear space before cancelling"};
    }
  }
  if (save.coins > 10000000 - it->fee) {
    return {false, "Treasury full"};
  }
  double duration = CraftDelay(Recipes()[RecipeIndex(it->recipe)]);
  auto facility = it->facility;
  auto due = it->due;
  for (int g = 0; g < kGoodTypes; ++g) {
    save.stocks[it->source][g] += it->materials[g];
  }
  save.coins += it->fee;
  save.production.erase(it);
  for (auto& job : save.production) {
    if (job.facility == facility && job.due > due) {
      job.due = std::max(save.game_time, job.due - duration);
    }
  }
  return {true, "Job cancelled; reserved materials and fee returned"};
}
Action PlantResource(Save& save, int city, int good) {
  if (!OwnsCity(save, city) || (good != 0 && good != 2 && good != 3)) {
    return {false, "Choose a forest, food or reed plot in your town"};
  }
  int plots = static_cast<int>(
      std::count_if(save.plots.begin(), save.plots.end(),
                    [&](const auto& p) { return p.city == city; }));
  int limit = 3 + static_cast<int>((CityRadius(save, city) - 350) / 150) * 2;
  if (plots >= limit) {
    return {false, "Expand town territory to grow more plots"};
  }
  if (save.stocks[city][0] < 2 || save.stocks[city][2] < 2) {
    return {false, "Planting needs 2 wood and 2 berries from the warehouse"};
  }
  save.stocks[city][0] -= 2;
  save.stocks[city][2] -= 2;
  save.plots.push_back(
      {city, good, save.game_time + (good == 0 ? 600 : 180), false});
  RecordHistory(save, "planting", "Planted " + std::string(GoodName(good)),
                city);
  return {true, good == 0 ? "Trees planted; mature in 10 game minutes"
                          : "Plot planted; mature in 3 game minutes"};
}
Action HarvestPlot(Save& save, int index) {
  if (index < 0 || index >= static_cast<int>(save.plots.size()) ||
      !save.plots[index].ready) {
    return {false, "Plot is not ready"};
  }
  auto p = save.plots[index];
  int amount = p.good == 0 ? 20 : 12;
  if (save.stocks[p.city][p.good] > 1000000 - amount) {
    return {false, "Warehouse full"};
  }
  save.stocks[p.city][p.good] += amount;
  save.known_goods[p.good] = true;
  save.plots.erase(save.plots.begin() + index);
  return {true,
          "Harvest delivered to the city warehouse; replant to grow more"};
}
Action OrderRareGoods(Save& save, int from, int to, int good, int quantity) {
  if (!ValidCity(save, from) || !OwnsCity(save, to) || OwnsCity(save, from) ||
      !save.relations[from].discovered || !PeacefulCity(save, from) ||
      good < 6 || good >= kGoodTypes || quantity < 1 || quantity > 10 ||
      save.shipments.size() >= 256) {
    return {false,
            "Choose a discovered peaceful specialist and a destination town"};
  }
  const auto& type = GetContent().towns[Settlements()[from].type];
  if (std::find(type.goods.begin(), type.goods.end(), good) ==
          type.goods.end() ||
      save.relations[from].opinion < 20 ||
      !save.technologies.contains(type.technology)) {
    return {false, "Specialty knowledge and trust 20 are required"};
  }
  int pending = 0;
  for (const auto& s : save.shipments) {
    if (s.from == from && s.good == good) {
      pending += s.quantity;
    }
  }
  if (pending + quantity > 10) {
    return {false, "Specialist already has its maximum pending order"};
  }
  int price = BuyPrice(from, good) * quantity;
  if (save.coins < price || save.treasury[from] > 10000000 - price) {
    return {false, "Insufficient state funds"};
  }
  save.coins -= price;
  save.treasury[from] += price;
  double travel =
      120 + Length(Settlements()[from].pos - Settlements()[to].pos) / 40;
  save.shipments.push_back(
      {save.next_entity++, from, to, good, quantity, save.game_time + travel});
  return {
      true,
      "Specialist order paid; goods are produced for the scheduled delivery"};
}
Action ShipGoods(Save& save, int from, int to, int good, int quantity) {
  if (!OwnsCity(save, from) || !OwnsCity(save, to) || from == to ||
      !ConnectedCities(save, from, to) || good < 0 || good >= kGoodTypes ||
      quantity < 1 || quantity > 1000000 || save.shipments.size() >= 256) {
    return {false,
            "Warehouse transfer requires two owned towns connected by a "
            "peaceful road"};
  }
  if (save.stocks[from][good] < quantity) {
    return {false, "Insufficient source stock"};
  }
  save.stocks[from][good] -= quantity;
  save.shipments.push_back(
      {save.next_entity++, from, to, good, quantity,
       save.game_time + 30 +
           Length(Settlements()[from].pos - Settlements()[to].pos) / 80});
  return {true, "Cargo reserved for inter-town delivery"};
}
void RefreshCityTasks(Save& save) {
  for (int city = 0; city < static_cast<int>(save.stocks.size()); ++city) {
    if (!save.relations[city].discovered) {
      continue;
    }
    int active = 0;
    for (const auto& t : save.city_tasks) {
      if (t.city == city && !t.completed) {
        ++active;
      }
    }
    if (active >= 4 || save.city_tasks.size() >= 2048) {
      continue;
    }
    // Every town provides an introductory project, then responds to deficits.
    bool intro = std::any_of(
        save.city_tasks.begin(), save.city_tasks.end(), [&](const auto& t) {
          return t.city == city && t.reward == "recipe:outfit_forager" &&
                 (!t.completed || t.accepted);
        });
    if (!intro) {
      save.city_tasks.push_back({save.next_entity++, city, "project", 0, 6, 0,
                                 0, false, false, "recipe:outfit_forager", 0,
                                 0});
      ++active;
    }
    bool survey = std::any_of(
        save.city_tasks.begin(), save.city_tasks.end(), [](const auto& t) {
          return t.kind == "survey" && (!t.completed || t.accepted);
        });
    if (city == save.home_city && !survey &&
        !save.technologies.contains("civic_charter") && active < 4) {
      save.city_tasks.push_back({save.next_entity++, city, "survey", 0, 2, 0, 0,
                                 false, false, "technology:civic_charter", 0,
                                 0});
      ++active;
    }
    int offset = static_cast<int>(
        Hash(save.seed ^ city ^
             static_cast<uint64_t>(std::max<int64_t>(0, save.economy_cycle))) %
        3);
    const int goods[] = {0, 2, 3};
    for (int j = 0; j < 3 && active < 3; ++j) {
      int good = goods[(j + offset) % 3];
      int target = good == 0 ? 50 : 30;
      if (save.stocks[city][good] + Incoming(save, city, good) >= target) {
        continue;
      }
      bool existing = std::any_of(
          save.city_tasks.begin(), save.city_tasks.end(), [&](const auto& t) {
            return t.city == city && !t.completed && t.good == good;
          });
      if (existing) {
        continue;
      }
      int amount = std::min(
          12, target - save.stocks[city][good] - Incoming(save, city, good));
      bool trade = !OwnsCity(save, city);
      int price = trade ? std::max(1, SellPrice(city, good)) * amount : 0;
      if (trade && save.treasury[city] - price < 40) {
        continue;
      }
      std::string reward = trade ? "" : "trust";
      if (!trade) {
        if (good == 2) {
          reward = save.technologies.contains("medical_care")
                       ? "care"
                       : "technology:medical_care";
        } else if (good == 0) {
          reward = "housing";
        } else {
          for (const auto& outfit : GetContent().outfits) {
            if (outfit.quest_only &&
                !save.known_recipes.contains("outfit_" + outfit.id)) {
              reward = "recipe:outfit_" + outfit.id;
              break;
            }
          }
        }
      }
      save.city_tasks.push_back(
          {save.next_entity++, city, trade ? "trade" : "project", good, amount,
           price, 0, false, false, reward, 0, save.game_time + 600});
      ++active;
    }
    const auto& type = GetContent().towns[Settlements()[city].type];
    bool lesson = std::any_of(
        save.city_tasks.begin(), save.city_tasks.end(), [&](const auto& t) {
          return t.city == city &&
                 t.reward == "technology:" + type.technology &&
                 (!t.completed || t.accepted);
        });
    if (!lesson && active < 4) {
      save.city_tasks.push_back({save.next_entity++, city, "project", 1, 8, 0,
                                 0, false, false,
                                 "technology:" + type.technology, 0, 0});
    }
  }
}
Action AcceptCityTask(Save& save, uint64_t id) {
  auto it = std::find_if(save.city_tasks.begin(), save.city_tasks.end(),
                         [&](const auto& t) { return t.id == id; });
  if (it == save.city_tasks.end() || it->completed || it->accepted ||
      !PeacefulCity(save, it->city)) {
    return {false, "Task is unavailable or already accepted"};
  }
  if (it->kind == "trade") {
    if (save.treasury[it->city] - it->price < 40) {
      return {false, "The buyer must keep its protected reserve"};
    }
    save.treasury[it->city] -= it->price;
    it->escrow = it->price;
  }
  if (it->kind == "survey") {
    it->baseline = std::count_if(save.relations.begin(), save.relations.end(),
                                 [](const auto& r) { return r.discovered; });
  }
  it->accepted = true;
  return {true, it->kind == "trade"
                    ? "Trade order accepted; payment reserved by the buyer"
                    : "Town project accepted; no coin reward"};
}
Action FinishCityTask(Save& save, uint64_t id) {
  auto it = std::find_if(save.city_tasks.begin(), save.city_tasks.end(),
                         [&](const auto& t) { return t.id == id; });
  if (it == save.city_tasks.end() || it->completed || !it->accepted ||
      !PeacefulCity(save, it->city)) {
    return {false, "No accepted task to complete"};
  }
  int city = it->city;
  if (CityAt(save, save.player) != city || !HasControlled(save)) {
    return {false,
            "Bring a resident to the requesting town to complete delivery"};
  }
  if (save.coins > 10000000 - it->escrow) {
    return {false, "State treasury full"};
  }
  bool legacy = it->kind.starts_with("legacy:");
  bool delivery = it->kind != "survey";
  if (it->kind == "survey") {
    int known = std::count_if(save.relations.begin(), save.relations.end(),
                              [](const auto& r) { return r.discovered; });
    if (known - it->baseline < it->quantity) {
      return {false, "Discover two more towns after accepting the survey"};
    }
  }
  if (legacy) {
    const auto q = Quest(it->kind.substr(7));
    delivery = q.objective == "deliver";
    QuestProgress p{q.id, false, it->baseline};
    if (GetQuestProgress(save, p) < q.target) {
      return {false, "Legacy objective is not yet complete"};
    }
  }
  if (delivery) {
    if (OwnsCity(save, city)) {
      if (save.stocks[city][it->good] < it->quantity) {
        return {false, "Deposit the required goods in this town's warehouse"};
      }
      save.stocks[city][it->good] -= it->quantity;
    } else {
      if (GoodCount(save.inventory, it->good) < it->quantity ||
          save.stocks[city][it->good] > 1000000 - it->quantity) {
        return {false, "The selected resident must carry the requested goods"};
      }
      GoodCount(save.inventory, it->good) -= it->quantity;
      save.stocks[city][it->good] += it->quantity;
    }
  }
  if (save.coins > 10000000 - it->escrow) {
    return {false, "State treasury full"};
  }
  save.coins += it->escrow;
  it->escrow = 0;
  it->completed = true;
  UnlockTaskReward(save, it->reward);
  if (it->reward == "housing") {
    save.housing[city] = std::min(104, save.housing[city] + 1);
  } else if (it->reward == "care") {
    for (auto& c : save.citizens) {
      if (c.city == city && c.state == "active") {
        c.health = std::min(100, c.health + 20);
        if (c.npc == save.controlled_npc) {
          save.health = c.health;
        }
      }
    }
  }
  save.relations[city].opinion =
      std::min(100, save.relations[city].opinion + 5);
  if (it->kind == "trade") {
    ++save.orders;
  }
  StoreControlled(save);
  RecordHistory(save, "city_task",
                it->kind == "trade" ? "Completed a trade order"
                                    : "Completed a town project",
                city);
  return {true,
          it->kind == "trade"
              ? "Delivery accepted; escrow paid to the state treasury"
              : "Town project complete; knowledge/trust awarded without coins"};
}
Action CancelCityTask(Save& save, uint64_t id) {
  auto it = std::find_if(save.city_tasks.begin(), save.city_tasks.end(),
                         [&](const auto& t) { return t.id == id; });
  if (it == save.city_tasks.end() || !it->accepted || it->completed) {
    return {false, "No accepted task to abandon"};
  }
  if (save.treasury[it->city] > 10000000 - it->escrow) {
    return {false, "Buyer treasury full"};
  }
  save.treasury[it->city] += it->escrow;
  it->escrow = 0;
  it->accepted = false;
  it->completed = true;
  return {true, "Task abandoned; reserved payment returned to the buyer"};
}
Action AcceptTraveler(Save& save, int npc) {
  auto it = std::find_if(save.travelers.begin(), save.travelers.end(),
                         [&](const auto& t) { return t.npc == npc; });
  if (it == save.travelers.end() || it->accepted || it->declined ||
      !OwnsCity(save, it->city)) {
    return {false, "Wanderer unavailable"};
  }
  int city = it->city;
  if (CityPopulation(save, city) >= save.housing[city] ||
      save.stocks[city][2] < 5) {
    return {false, "A new resident needs a housing place and 5 berries"};
  }
  save.stocks[city][2] -= 5;
  Citizen c;
  c.npc = npc;
  c.city = city;
  c.position = Settlements()[city].pos + Vec{140, 80};
  c.outfit = "traveler";
  c.clothes["traveler"] = 1;
  ++save.outfit_copies["traveler"];
  save.citizens.push_back(c);
  it->accepted = true;
  RecordHistory(save, "immigration", "Welcomed " + it->name, city);
  return {true, "Wanderer joined the town and can be selected"};
}
Action MoveResident(Save& save, int npc, int city) {
  StoreControlled(save);
  auto* c = GetCitizen(save, npc);
  if (!c || c->state != "active" || !OwnsCity(save, c->city) ||
      !OwnsCity(save, city) || city == c->city ||
      CityAt(save, c->npc == save.controlled_npc ? save.player : c->position) !=
          city) {
    return {
        false,
        "Walk this resident to the destination town before changing residence"};
  }
  if (CityPopulation(save, city) >= save.housing[city] ||
      save.stocks[city][2] < 5) {
    return {false, "Destination needs housing space and 5 berries"};
  }
  save.stocks[city][2] -= 5;
  c->city = city;
  RecordHistory(save, "migration", "Resident moved home town", city);
  return {true, "Home town changed without teleporting the resident"};
}
Action BuildHousing(Save& save, int city) {
  if (!OwnsCity(save, city) || !HasSkill(save, "construction") ||
      save.housing[city] >= 100) {
    return {false,
            "Housing requires your town and Construction; maximum 100 places"};
  }
  if (save.stocks[city][0] < 20 || save.stocks[city][1] < 10 ||
      save.coins < 10) {
    return {false,
            "Housing needs warehouse wood 20, stone 10 and state coins 10"};
  }
  save.stocks[city][0] -= 20;
  save.stocks[city][1] -= 10;
  save.coins -= 10;
  save.region_fund += 10;
  save.housing[city] += 4;
  return {true, "Housing capacity increased by four places"};
}
Action TreatResident(Save& save, int npc) {
  StoreControlled(save);
  auto* c = GetCitizen(save, npc);
  if (!c || c->state != "active" || c->health >= 100 ||
      !OwnsCity(save, c->city) ||
      CityAt(save, c->npc == save.controlled_npc ? save.player : c->position) !=
          c->city) {
    return {false, "Bring an injured resident to their home town"};
  }
  bool hospital = FacilityAccess(save, c->city, "hospital");
  if (!hospital && !FacilityAccess(save, c->city, "infirmary")) {
    return {false, "A local or road-connected infirmary is required"};
  }
  int cost = hospital ? 3 : 5;
  if (save.stocks[c->city][2] < cost || save.coins < cost) {
    return {false,
            "Treatment needs berries and coins: " + std::to_string(cost)};
  }
  save.stocks[c->city][2] -= cost;
  save.coins -= cost;
  save.region_fund += cost;
  c->health = std::min(100, c->health + (hospital ? 100 : 40));
  if (c->npc == save.controlled_npc) {
    save.health = c->health;
  }
  return {true, "Resident treated"};
}
Action ResearchHospital(Save& save) {
  if (save.technologies.contains("hospitals")) {
    return {false, "Hospital technology already known"};
  }
  if (!save.technologies.contains("medical_care") ||
      !FacilityAccess(save, CityAt(save, save.player), "school") ||
      save.coins < 80) {
    return {
        false,
        "Hospital research needs Medical care, a school and 80 state coins"};
  }
  save.coins -= 80;
  save.region_fund += 80;
  save.technologies.insert("hospitals");
  return {true, "Hospital technology learned by the state"};
}
Action AssignNationCitizen(Save& save, int npc, const std::string& job,
                           const std::string& weapon,
                           const std::string& outfit) {
  StoreControlled(save);
  auto* c = GetCitizen(save, npc);
  if (!c || c->state != "active" || !OwnsCity(save, c->city)) {
    return {false, "Resident unavailable"};
  }
  if (std::find(ResidentJobs().begin(), ResidentJobs().end(), job) ==
      ResidentJobs().end()) {
    return {false, "Unknown role"};
  }
  std::string tech = job == "gatherer" || job == "crafter" ? "apprenticeship"
                     : job == "guard"                      ? "militia"
                     : job == "trader"                     ? "commerce_school"
                     : job == "fisher"                     ? "navigation_school"
                     : job == "scholar" || job == "engineer"
                         ? "citizen_engineering"
                     : job == "ambassador" ? "diplomatic_corps"
                                           : "";
  if (!tech.empty() && (!save.technologies.contains(tech) ||
                        !FacilityAccess(save, c->city, "school"))) {
    return {false, "State training and a school are required"};
  }
  if ((!weapon.empty() && c->weapons[weapon] < 1) ||
      (!outfit.empty() && c->clothes[outfit] < 1)) {
    return {false,
            "Select this resident and withdraw the equipment from a warehouse "
            "first"};
  }
  if (job != "idle" && (outfit.empty() || Outfit(outfit).job != job)) {
    return {false, "Matching work clothes required"};
  }
  if (job == "guard" && (weapon.empty() || Weapon(weapon).kind == "bow")) {
    return {false, "Defender requires a melee weapon"};
  }
  c->job = job;
  c->trained = job != "idle";
  c->weapon = weapon;
  if (!outfit.empty()) {
    c->outfit = outfit;
  }
  if (npc == save.controlled_npc) {
    LoadControlled(save, *c);
  }
  return {true, "Role and carried equipment assigned; no autonomous orders"};
}
void TickNation(Save& save, float dt) {
  if (!save.nation || !std::isfinite(dt) || dt <= 0) {
    return;
  }
  StoreControlled(save);
  for (auto& task : save.city_tasks) {
    if (!task.completed && task.kind == "trade" && OwnsCity(save, task.city)) {
      save.treasury[task.city] += task.escrow;
      task.escrow = 0;
      task.completed = true;
    }
  }
  for (auto it = save.production.begin(); it != save.production.end();) {
    auto f =
        std::find_if(save.facilities.begin(), save.facilities.end(),
                     [&](const auto& site) { return site.id == it->facility; });
    if (f == save.facilities.end() ||
        !ReachableFacility(save, it->source, f->city)) {
      // Shift the full queue while access is blocked; materials stay reserved.
      it->due += dt;
      ++it;
      continue;
    }
    if (it->due > save.game_time) {
      ++it;
      continue;
    }
    const auto& recipe = Recipes()[RecipeIndex(it->recipe)];
    std::string product = CanonicalProduct(recipe.result);
    int amount = product == "ammo" ? 10 : 1;
    if (product == "steel") {
      if (save.stocks[it->destination][10] > 999997) {
        ++it;
        continue;
      }
      save.stocks[it->destination][10] += 3;
    } else {
      auto& count = save.city_products[it->destination][product];
      if (count > 1000000 - amount) {
        ++it;
        continue;
      }
      count += amount;
      if (product.starts_with("weapon:")) {
        ++save.weapon_copies[product.substr(7)];
      } else if (product.starts_with("outfit:")) {
        ++save.outfit_copies[product.substr(7)];
      }
    }
    save.region_fund += it->fee;
    ++save.crafted;
    RecordHistory(save, "production", "Finished " + recipe.name,
                  it->destination);
    it = save.production.erase(it);
  }
  for (auto& plot : save.plots) {
    plot.ready = save.game_time >= plot.due;
  }
  for (auto it = save.shipments.begin(); it != save.shipments.end();) {
    bool access = OwnsCity(save, it->from)
                      ? ConnectedCities(save, it->from, it->to)
                      : PeacefulCity(save, it->from);
    if (!access) {
      it->due += dt;
      ++it;
    } else if (save.game_time >= it->due &&
               save.stocks[it->to][it->good] <= 1000000 - it->quantity) {
      save.stocks[it->to][it->good] += it->quantity;
      save.known_goods[it->good] = true;
      RecordHistory(save, "shipment", "Received scheduled cargo", it->to);
      it = save.shipments.erase(it);
    } else {
      ++it;
    }
  }
  int64_t cycle = static_cast<int64_t>(save.game_time / 120);
  if (cycle != save.economy_cycle) {
    save.economy_cycle = cycle;
    for (auto& task : save.city_tasks) {
      if (!task.completed && !task.accepted && task.expires > 0 &&
          task.expires <= save.game_time) {
        task.completed = true;
      }
    }
    // The regional market pays from its real budget for a bounded surplus.
    // Goods are consumed by that abstract external market, never sold twice.
    for (int city = 0; city < static_cast<int>(save.stocks.size()); ++city) {
      if (!OwnsCity(save, city)) {
        for (int g : {0, 2, 3}) {
          save.stocks[city][g] = std::min(1000000, save.stocks[city][g] + 4);
        }
      }
      int good = static_cast<int>(
          Hash(save.seed ^ city ^ static_cast<uint64_t>(cycle)) % 3);
      good = good == 0 ? 0 : good == 1 ? 2 : 3;
      if (save.stocks[city][good] > 30 && save.region_fund >= 4 &&
          save.treasury[city] <= 9999996) {
        save.stocks[city][good] -= 2;
        save.region_fund -= 4;
        save.treasury[city] += 4;
      }
    }
    RefreshCityTasks(save);
    if (cycle % 3 == 0 && save.travelers.size() < 128) {
      for (int city = 0; city < static_cast<int>(save.stocks.size()); ++city) {
        bool waiting = std::any_of(
            save.travelers.begin(), save.travelers.end(), [&](const auto& t) {
              return t.city == city && !t.accepted && !t.declined;
            });
        if (OwnsCity(save, city) && !waiting) {
          int npc = static_cast<int>(MakeNpcs().size() + save.travelers.size());
          save.travelers.push_back(
              {npc, "Wanderer " + std::to_string(save.travelers.size() + 1),
               city, false, false});
        }
      }
    }
  }
  if (MoneySupply(save) != save.money_supply) {
    throw std::runtime_error(
        "Money ledger mismatch: a payment created or destroyed coins");
  }
}
}  // namespace frontier
