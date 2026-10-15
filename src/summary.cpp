#include "summary.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "nation.hpp"
#include "vessels.hpp"

namespace frontier {
std::vector<SummaryEntry> InventorySummary(const Save& save) {
  std::vector<SummaryEntry> rows;
  rows.push_back({"coins", "Coins", "", save.coins});
  for (int g = 0; g < kGoodTypes; g++) {
    int count = GoodCount(save.inventory, g);
    if (save.nation) {
      count = 0;
      for (int city = 0; city < static_cast<int>(save.stocks.size()); ++city) {
        if (OwnsCity(save, city)) {
          count += save.stocks[city][g];
        }
      }
      for (const auto& c : save.citizens) {
        if (c.state == "active") {
          count += c.npc == save.controlled_npc ? GoodCount(save.inventory, g)
                                                : GoodCount(c.cargo, g);
        }
      }
    }
    rows.push_back(
        {"good_" + std::to_string(g), GoodName(g),
         save.nation ? "State warehouses + resident cargo" : "Resource",
         count});
  }
  auto stored_count = [&](const std::string& product) {
    int count = 0;
    for (int city = 0; city < static_cast<int>(save.city_products.size());
         ++city) {
      if (OwnsCity(save, city)) {
        auto found = save.city_products[city].find(product);
        if (found != save.city_products[city].end()) {
          count += found->second;
        }
      }
    }
    for (const auto& c : save.citizens) {
      if (c.state != "active" || !OwnsCity(save, c.city)) {
        continue;
      }
      const auto& cargo =
          c.npc == save.controlled_npc ? save.inventory : c.cargo;
      if (product == "potion") {
        count += cargo.potions;
      } else if (product == "campfire") {
        count += cargo.campfires;
      } else if (product == "ammo") {
        count += cargo.ammo;
      } else if (product == "repair") {
        count += cargo.repairs;
      } else if (product == "axe") {
        count += cargo.axe;
      } else if (product == "pickaxe") {
        count += cargo.pickaxe;
      } else if (product == "rod") {
        count += cargo.rod;
      } else if (product == "boat") {
        count += cargo.boat;
      } else if (product == "ship") {
        count += cargo.ship;
      } else if (product == "submarine") {
        count += cargo.submarine;
      } else if (product == "sonar") {
        count += cargo.sonar;
      } else if (product == "oxygen") {
        count += cargo.oxygen;
      } else if (product.starts_with("vessel:")) {
        const auto& vessels =
            c.npc == save.controlled_npc ? save.vessels : c.vessels;
        count += vessels.contains(product.substr(7));
      }
    }
    return count;
  };
  auto add = [&](const char* id, const char* name, int count) {
    const std::string product = std::string(id) == "arrows" ? "ammo" : id;
    rows.push_back({id, name, "", save.nation ? stored_count(product) : count});
  };
  add("potion", "Healing potions", save.inventory.potions);
  add("campfire", "Campfires", save.inventory.campfires);
  add("arrows", "Arrows", save.inventory.ammo);
  add("repair", "Repair kits", save.inventory.repairs);
  auto flag = [&](const char* id, const char* name, bool have) {
    if (have || (save.nation && stored_count(id) > 0)) {
      add(id, name, 1);
    }
  };
  flag("axe", "Axe", save.inventory.axe);
  flag("pickaxe", "Pickaxe", save.inventory.pickaxe);
  flag("rod", "Fishing rod", save.inventory.rod);
  flag("boat", "Rowboat", save.inventory.boat);
  flag("ship", "Ship", save.inventory.ship);
  flag("submarine", "Submarine", save.inventory.submarine);
  flag("sonar", "Sonar", save.inventory.sonar);
  flag("oxygen", "Oxygen module", save.inventory.oxygen);
  for (const auto& item : GetContent().weapons) {
    if (save.nation || save.inventory.weapons.contains(item.id)) {
      auto found = save.weapon_copies.find(item.id);
      int copies = found == save.weapon_copies.end() ? (save.nation ? 0 : 1)
                                                     : found->second;
      if (copies > 0) {
        rows.push_back(
            {"weapon_" + item.id, item.name,
             "Damage " + std::to_string(item.damage) + " · " +
                 RarityName(item.rarity) +
                 (save.inventory.weapon_id == item.id ? " · equipped" : ""),
             copies});
      }
    }
  }
  for (const auto& item : GetContent().outfits) {
    auto found = save.outfit_copies.find(item.id);
    if (found != save.outfit_copies.end() && found->second > 0) {
      rows.push_back(
          {"outfit_" + item.id, item.name,
           (save.outfit_id == item.id ? "Worn · " : "") + item.description,
           found->second});
    }
  }
  for (const auto& v : Vessels()) {
    int count = save.nation ? stored_count("vessel:" + v.id) : 1;
    if (v.number > 3 &&
        (save.nation ? count > 0 : OwnsVessel(save, v.number))) {
      rows.push_back({"ship", v.name,
                      "Prestige " + std::to_string(v.prestige) + " · sight " +
                          std::to_string(static_cast<int>(v.view)),
                      count});
    }
  }
  return rows;
}

std::vector<ProgressCard> ProgressCards(const Save& save) {
  std::vector<ProgressCard> cards;
  auto activity = [&](std::string id, std::string name, int amount, int row) {
    cards.push_back({id,
                     name,
                     id,
                     "Recorded activity: " + std::to_string(amount) +
                         ". Each connected lesson has its own required count; "
                         "click its cell to check progress.",
                     {},
                     0,
                     row,
                     amount > 0,
                     amount > 0,
                     true,
                     amount});
  };
  activity("gather_sites", "Gather sites", save.gathered, 1);
  activity("craft_items", "Craft items", save.crafted, 2);
  activity("catch_fish", "Catch fish", save.caught, 3);
  activity("town_orders", "Town orders", save.orders, 4);
  const std::vector<std::string> parent = {"",
                                           "gathering",
                                           "gather_sites",
                                           "craft_items",
                                           "craft_items",
                                           "gather_sites",
                                           "catch_fish",
                                           "gather_sites",
                                           "craft_items",
                                           "trading",
                                           "town_orders"};
  const int columns[] = {0, 1, 1, 2, 2, 1, 2, 1, 2, 2, 1},
            rows[] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 0, 5};
  for (int i = 0; i < static_cast<int>(Skills().size()); i++) {
    const auto& skill = Skills()[i];
    auto state = GetLessonState(save, i);
    std::string detail = state.detail + ". Learn from this tree when ready. ";
    if (i == 0) {
      detail += "Unlocks gathering trees, stones, berries and reeds.";
    } else {
      detail += skill.requirement + ".";
    }
    cards.push_back({skill.id, skill.name, "skill_" + skill.id, detail,
                     parent[i].empty() ? std::vector<std::string>{}
                                       : std::vector<std::string>{parent[i]},
                     columns[i], rows[i], state.learned, state.ready});
  }
  cards.push_back({"civic_charter",
                   "Civic charter",
                   "civic_charter",
                   save.nation
                       ? "Town project: accept the home town survey, then "
                         "discover two new towns and return."
                       : "Quest reward: complete Know your neighbors (discover "
                         "two new towns after accepting the quest).",
                   {},
                   2,
                   5,
                   save.technologies.contains("civic_charter"),
                   false});
  const int research_rows[] = {1, 2, 0, 3, 5, 6, 1};
  for (int i = 0; i < static_cast<int>(ResearchNodes().size()); i++) {
    const auto& n = ResearchNodes()[i];
    std::vector<std::string> parents{n.prerequisite};
    if (!n.second_prerequisite.empty()) {
      parents.push_back(n.second_prerequisite);
    }
    bool ready = ResearchReady(save, n);
    cards.push_back({n.id, n.name, "tech_" + n.id,
                     ResearchRequirements(save, n) +
                         ". Research from this tree when ready.",
                     parents, n.id == "guild_charter" ? 4 : 3, research_rows[i],
                     save.technologies.contains(n.id), ready});
  }
  int row = 3;
  std::vector<std::string> added;
  for (const auto& town : GetContent().towns) {
    if (std::find(added.begin(), added.end(), town.technology) != added.end()) {
      continue;
    }
    added.push_back(town.technology);
    bool learned = save.technologies.contains(town.technology), teacher = false;
    for (size_t city = 0;
         city < Settlements().size() && city < save.relations.size(); city++) {
      if (GetContent().towns[Settlements()[city].type].technology ==
              town.technology &&
          save.relations[city].discovered &&
          save.relations[city].treaty != Treaty::kWar &&
          save.relations[city].opinion >= 20) {
        teacher = true;
      }
    }
    bool ready =
        !learned && teacher && HasSkill(save, "crafting") && save.coins >= 20;
    cards.push_back({town.technology,
                     TechnologyName(town.technology),
                     "tech_" + town.technology,
                     "Find a " + town.name +
                         " town master: peace, trust 20, Craftsmanship and 20 "
                         "coins (you have " +
                         std::to_string(save.coins) +
                         "). Visit the town master for a specialist lesson or "
                         "use the town hall.",
                     {"crafting"},
                     4,
                     row++,
                     learned,
                     ready});
  }
  if (save.nation) {
    cards.push_back({"medical_care",
                     "Medical care",
                     "book",
                     "Complete a town medical-supplies project. Knowledge "
                     "belongs to the state.",
                     {},
                     3,
                     7,
                     save.technologies.contains("medical_care"),
                     false});
    cards.push_back({"hospitals",
                     "Hospitals",
                     "book",
                     "Research in State > Growth: Medical care, an accessible "
                     "school and 80 state coins.",
                     {"medical_care"},
                     4,
                     11,
                     save.technologies.contains("hospitals"),
                     false});
  }
  return cards;
}
}  // namespace frontier
