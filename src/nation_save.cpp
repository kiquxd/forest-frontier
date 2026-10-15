#include <cmath>
#include <stdexcept>
#include <unordered_set>

#include "adventure.hpp"
#include "nation.hpp"
#include "settlement_system.hpp"
#include "vessels.hpp"

namespace frontier {
namespace {
using Json = nlohmann::json;
Json PackInventory(const Inventory& inv) {
  Json goods = Json::array();
  for (int g = 0; g < kGoodTypes; ++g) {
    goods.push_back(GoodCount(inv, g));
  }
  return {{"goods", goods},
          {"potions", inv.potions},
          {"campfires", inv.campfires},
          {"axe", inv.axe},
          {"pickaxe", inv.pickaxe},
          {"rod", inv.rod},
          {"boat", inv.boat},
          {"ship", inv.ship},
          {"submarine", inv.submarine},
          {"sonar", inv.sonar},
          {"oxygen", inv.oxygen},
          {"ammo", inv.ammo},
          {"repairs", inv.repairs}};
}
void Require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}
Inventory UnpackInventory(const Json& j) {
  Inventory inv;
  auto goods = j.at("goods").get<std::array<int, kGoodTypes>>();
  for (int g = 0; g < kGoodTypes; ++g) {
    Require(goods[g] >= 0 && goods[g] <= 1000000, "Invalid resident cargo");
    GoodCount(inv, g) = goods[g];
  }
  inv.potions = j.at("potions");
  inv.campfires = j.at("campfires");
  inv.axe = j.at("axe");
  inv.pickaxe = j.at("pickaxe");
  inv.rod = j.at("rod");
  inv.boat = j.at("boat");
  inv.ship = j.at("ship");
  inv.submarine = j.at("submarine");
  inv.sonar = j.at("sonar");
  inv.oxygen = j.at("oxygen");
  inv.ammo = j.at("ammo");
  inv.repairs = j.at("repairs");
  Require(inv.potions >= 0 && inv.potions <= 1000000 && inv.campfires >= 0 &&
              inv.campfires <= 1000 && inv.ammo >= 0 && inv.ammo <= 1000000 &&
              inv.repairs >= 0 && inv.repairs <= 1000000,
          "Invalid resident supplies");
  return inv;
}
Vec Position(const Json& j) {
  Vec p{j.at(0).get<float>(), j.at(1).get<float>()};
  Require(std::isfinite(p.x) && std::isfinite(p.y) && Length(p) <= 2e6,
          "Invalid resident position");
  return p;
}
bool KnownRecipe(const std::string& id) {
  for (const auto& r : Recipes()) {
    if (r.id == id) {
      return true;
    }
  }
  return false;
}
void ValidateProduct(const std::string& product, int count) {
  bool known = product == "outfit:traveler";
  for (const auto& r : Recipes()) {
    known = known || product == r.result ||
            (r.result == "armor" && product == "outfit:leather_armor");
  }
  Require(known && count >= 0 && count <= 1000000, "Invalid warehouse product");
}
}  // namespace
nlohmann::json WriteNation(const Save& save) {
  Json j{{"enabled", save.nation}};
  if (!save.nation) {
    return j;
  }
  j["controlled"] = save.controlled_npc;
  j["region_fund"] = save.region_fund;
  j["money_supply"] = save.money_supply;
  j["next_entity"] = save.next_entity;
  j["economy_cycle"] = save.economy_cycle;
  j["city_products"] = save.city_products;
  j["housing"] = save.housing;
  j["residents"] = Json::array();
  for (const auto& c : save.citizens) {
    j["residents"].push_back({{"npc", c.npc},
                              {"position", {c.position.x, c.position.y}},
                              {"cargo", PackInventory(c.cargo)},
                              {"background", c.background},
                              {"weapons", c.weapons},
                              {"clothes", c.clothes},
                              {"vessels", c.vessels},
                              {"underwater", c.underwater},
                              {"air", c.air},
                              {"hull", c.hull},
                              {"vehicle", c.vehicle}});
  }
  j["production"] = Json::array();
  for (const auto& job : save.production) {
    j["production"].push_back({{"id", job.id},
                               {"facility", job.facility},
                               {"source", job.source},
                               {"destination", job.destination},
                               {"recipe", job.recipe},
                               {"due", job.due},
                               {"materials", job.materials},
                               {"fee", job.fee}});
  }
  j["plots"] = Json::array();
  for (const auto& p : save.plots) {
    j["plots"].push_back({{"city", p.city},
                          {"good", p.good},
                          {"due", p.due},
                          {"ready", p.ready}});
  }
  j["tasks"] = Json::array();
  for (const auto& t : save.city_tasks) {
    j["tasks"].push_back({{"id", t.id},
                          {"city", t.city},
                          {"kind", t.kind},
                          {"good", t.good},
                          {"quantity", t.quantity},
                          {"price", t.price},
                          {"escrow", t.escrow},
                          {"accepted", t.accepted},
                          {"completed", t.completed},
                          {"reward", t.reward},
                          {"baseline", t.baseline},
                          {"expires", t.expires}});
  }
  j["shipments"] = Json::array();
  for (const auto& s : save.shipments) {
    j["shipments"].push_back({{"id", s.id},
                              {"from", s.from},
                              {"to", s.to},
                              {"good", s.good},
                              {"quantity", s.quantity},
                              {"due", s.due}});
  }
  j["travelers"] = Json::array();
  for (const auto& t : save.travelers) {
    j["travelers"].push_back({{"npc", t.npc},
                              {"name", t.name},
                              {"city", t.city},
                              {"accepted", t.accepted},
                              {"declined", t.declined}});
  }
  return j;
}
void ReadNation(const nlohmann::json& j, Save& save) {
  save.nation = j.at("enabled");
  if (!save.nation) {
    return;
  }
  Require(save.campaign && save.urban && save.adventure,
          "Nation lacks campaign metadata");
  save.controlled_npc = j.at("controlled");
  save.region_fund = j.at("region_fund");
  save.money_supply = j.at("money_supply");
  save.next_entity = j.at("next_entity");
  save.economy_cycle = j.at("economy_cycle");
  save.city_products =
      j.at("city_products").get<decltype(save.city_products)>();
  save.housing = j.at("housing").get<decltype(save.housing)>();
  Require(
      save.region_fund >= 0 && save.money_supply > 0 && save.next_entity > 0 &&
          save.next_entity < (1ULL << 60) && save.economy_cycle >= -1 &&
          save.economy_cycle <= static_cast<int64_t>(save.game_time / 120) &&
          save.city_products.size() == save.stocks.size() &&
          save.housing.size() == save.stocks.size(),
      "Invalid nation accounts");
  auto city_ok = [&](int city) {
    return city >= 0 && city < static_cast<int>(save.stocks.size());
  };
  auto deadline = [&](double time) {
    return std::isfinite(time) && time >= 0 && time <= save.game_time + 1e8;
  };
  for (size_t city = 0; city < save.city_products.size(); ++city) {
    Require(save.housing[city] >= 1 && save.housing[city] <= 104 &&
                save.city_products[city].size() <= 256,
            "Invalid warehouse or housing");
    for (const auto& [product, count] : save.city_products[city]) {
      ValidateProduct(product, count);
    }
  }
  std::unordered_set<uint64_t> ids;
  auto entity = [&](uint64_t id) {
    Require(id > 0 && id < save.next_entity && ids.insert(id).second,
            "Duplicate nation entity");
  };
  for (const auto& f : save.facilities) {
    entity(f.id);
  }
  const auto& residents = j.at("residents");
  Require(residents.is_array() && residents.size() == save.citizens.size(),
          "Missing resident inventories");
  std::unordered_set<int> residents_seen;
  for (const auto& row : residents) {
    int npc = row.at("npc");
    auto* c = GetCitizen(save, npc);
    Require(c && residents_seen.insert(npc).second,
            "Duplicate resident inventory");
    c->position = Position(row.at("position"));
    c->cargo = UnpackInventory(row.at("cargo"));
    c->background = row.at("background");
    (void)Character(c->background);
    c->weapons = row.at("weapons").get<decltype(c->weapons)>();
    c->clothes = row.at("clothes").get<decltype(c->clothes)>();
    c->vessels = row.at("vessels").get<decltype(c->vessels)>();
    for (const auto& [id, count] : c->weapons) {
      (void)Weapon(id);
      Require(count >= 0 && count <= 1000000, "Invalid resident weapon count");
      if (count > 0) {
        c->cargo.weapons.insert(id);
      }
    }
    c->cargo.weapon_id = c->weapon;
    c->cargo.sword = !c->weapon.empty();
    for (const auto& [id, count] : c->clothes) {
      (void)Outfit(id);
      Require(count >= 0 && count <= 1000000,
              "Invalid resident clothing count");
    }
    for (const auto& id : c->vessels) {
      (void)Vessel(id);
    }
    c->underwater = row.at("underwater");
    c->air = row.at("air");
    c->hull = row.at("hull");
    c->vehicle = row.at("vehicle");
    Require(std::isfinite(c->air) && c->air >= 0 && c->air <= 200 &&
                c->hull >= 0 && c->hull <= 100 && c->vehicle >= 0 &&
                c->vehicle <= 9,
            "Invalid resident vessel state");
    if (c->state != "dead") {
      Require((c->weapon.empty() || c->weapons[c->weapon] > 0) &&
                  (!c->outfit.empty() && c->clothes[c->outfit] > 0),
              "Equipped gear absent from resident cargo");
    } else {
      Require(c->weapons.empty() && c->clothes.empty(),
              "Dead resident retained equipment");
    }
  }
  Require(j.at("production").is_array() && j.at("production").size() <= 2048,
          "Too many production jobs");
  for (const auto& row : j.at("production")) {
    WorkshopJob job{row.at("id"),
                    row.at("facility"),
                    row.at("source"),
                    row.at("destination"),
                    row.at("recipe"),
                    row.at("due"),
                    row.at("materials").get<std::array<int, kGoodTypes>>(),
                    row.at("fee")};
    entity(job.id);
    Require(city_ok(job.source) && city_ok(job.destination) &&
                KnownRecipe(job.recipe) && deadline(job.due) && job.fee >= 0 &&
                job.fee <= 10000,
            "Invalid production job");
    bool found = false;
    for (const auto& f : save.facilities) {
      found = found || f.id == job.facility;
    }
    Require(found, "Production references a missing workshop");
    for (int amount : job.materials) {
      Require(amount >= 0 && amount <= 10000, "Invalid reserved materials");
    }
    save.production.push_back(job);
  }
  Require(j.at("plots").is_array() &&
              j.at("plots").size() <= save.stocks.size() * 16,
          "Too many growing plots");
  for (const auto& row : j.at("plots")) {
    GrowingPlot p{row.at("city"), row.at("good"), row.at("due"),
                  row.at("ready")};
    Require(city_ok(p.city) && (p.good == 0 || p.good == 2 || p.good == 3) &&
                deadline(p.due),
            "Invalid growing plot");
    save.plots.push_back(p);
  }
  Require(j.at("tasks").is_array() && j.at("tasks").size() <= 2048,
          "Too many city tasks");
  for (const auto& row : j.at("tasks")) {
    CityTask t{row.at("id"),     row.at("city"),     row.at("kind"),
               row.at("good"),   row.at("quantity"), row.at("price"),
               row.at("escrow"), row.at("accepted"), row.at("completed"),
               row.at("reward"), row.at("baseline"), row.at("expires")};
    entity(t.id);
    Require(
        city_ok(t.city) && t.good >= 0 && t.good < kGoodTypes &&
            t.quantity > 0 && t.quantity <= 1000000 && t.price >= 0 &&
            t.price <= 10000000 && t.escrow >= 0 && t.escrow <= t.price &&
            deadline(t.expires) &&
            (t.kind == "project" || t.kind == "survey" || t.kind == "trade" ||
             (t.kind.starts_with("legacy:") && !t.kind.substr(7).empty())) &&
            (t.kind == "trade" || (t.price == 0 && t.escrow == 0)) &&
            (!t.completed || t.escrow == 0),
        "Invalid city task");
    if (t.kind.starts_with("legacy:")) {
      (void)Quest(t.kind.substr(7));
    }
    Require(t.reward.size() <= 128, "Invalid city reward");
    save.city_tasks.push_back(t);
  }
  Require(j.at("shipments").is_array() && j.at("shipments").size() <= 256,
          "Too many shipments");
  for (const auto& row : j.at("shipments")) {
    Shipment s{row.at("id"),   row.at("from"),     row.at("to"),
               row.at("good"), row.at("quantity"), row.at("due")};
    entity(s.id);
    Require(city_ok(s.from) && city_ok(s.to) && s.from != s.to && s.good >= 0 &&
                s.good < kGoodTypes && s.quantity > 0 &&
                s.quantity <= 1000000 && deadline(s.due),
            "Invalid shipment");
    save.shipments.push_back(s);
  }
  Require(j.at("travelers").is_array() && j.at("travelers").size() <= 128,
          "Too many travelers");
  for (const auto& row : j.at("travelers")) {
    Traveler t{row.at("npc"), row.at("name"), row.at("city"),
               row.at("accepted"), row.at("declined")};
    Require(t.npc >= 0 && t.name.size() > 0 && t.name.size() < 80 &&
                city_ok(t.city) && !(t.accepted && t.declined),
            "Invalid traveler");
    save.travelers.push_back(t);
  }
  // Compare physical allocations with state totals; active view is not a copy.
  std::unordered_map<std::string, int64_t> weapons, clothes;
  for (const auto& c : save.citizens) {
    for (const auto& [id, count] : c.weapons) {
      weapons[id] += count;
    }
    for (const auto& [id, count] : c.clothes) {
      clothes[id] += count;
    }
  }
  for (const auto& store : save.city_products) {
    for (const auto& [id, count] : store) {
      if (id.starts_with("weapon:")) {
        weapons[id.substr(7)] += count;
      } else if (id.starts_with("outfit:")) {
        clothes[id.substr(7)] += count;
      }
    }
  }
  for (const auto& [id, count] : save.weapon_copies) {
    Require(weapons[id] == count, "Weapon ledger mismatch");
    weapons.erase(id);
  }
  for (const auto& [id, count] : save.outfit_copies) {
    Require(clothes[id] == count, "Clothing ledger mismatch");
    clothes.erase(id);
  }
  for (const auto& [id, count] : weapons) {
    Require(count == 0, "Uncounted resident weapon");
  }
  for (const auto& [id, count] : clothes) {
    Require(count == 0, "Uncounted resident outfit");
  }
  Require(save.controlled_npc == -1 || GetCitizen(save, save.controlled_npc),
          "Controlled resident missing");
  if (const auto* c = GetCitizen(save, save.controlled_npc)) {
    Require(c->state == "active" &&
                PackInventory(c->cargo) == PackInventory(save.inventory) &&
                c->position.x == save.player.x &&
                c->position.y == save.player.y && c->health == save.health &&
                c->weapon == save.inventory.weapon_id &&
                c->outfit == save.outfit_id && c->vessels == save.vessels,
            "Controlled resident view differs from saved cargo");
  }
  Require(MoneySupply(save) == save.money_supply, "Money ledger mismatch");
}
}  // namespace frontier
