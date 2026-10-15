#include <algorithm>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include "json.hpp"
#include "summary.hpp"

namespace {
void Check(bool ok, const char* why) {
  if (!ok) {
    throw std::runtime_error(why);
  }
}

}  // namespace

int main() {
  try {
    frontier::ConfigureWorld(742019);
    frontier::Save save;
    frontier::InitializeCampaign(save);
    frontier::InitializeAdventure(save);
    save.inventory.wood = 17;
    save.coins = 123;
    save.inventory.weapons.insert("wood_sword");
    save.weapon_copies["wood_sword"] = 3;
    save.inventory.weapon_id = "wood_sword";
    save.outfit_copies["forager"] = 2;
    auto rows = frontier::InventorySummary(save);
    auto find = [&](const std::string& id) -> const frontier::SummaryEntry& {
      for (const auto& row : rows) {
        if (row.icon == id) {
          return row;
        }
      }
      throw std::runtime_error("Missing inventory row: " + id);
    };
    Check(find("good_0").count == 17 && find("coins").count == 123,
          "Inventory summary reports real counts");
    Check(find("weapon_wood_sword").count == 3 &&
              find("outfit_forager").count == 2,
          "Every equipment kind gets a counted row");
    std::ifstream manifest(std::string(FRONTIER_ROOT) +
                           "/assets/ui/icons.json");
    auto icons = nlohmann::json::parse(manifest).get<std::set<std::string>>();
    for (const auto& recipe : frontier::Recipes()) {
      Check(icons.contains(frontier::RecipeIcon(recipe)),
            "Every craft item resolves to an actual PNG atlas entry");
    }
    auto cards = frontier::ProgressCards(save);
    std::set<std::string> ids;
    std::set<std::pair<int, int>> cells;
    for (const auto& card : cards) {
      Check(ids.insert(card.id).second, "Unique progress IDs");
      Check(cells.insert({card.column, card.row}).second,
            "Tree cells do not overlap");
      Check(icons.contains(card.icon), "Tree has a PNG icon for every node");
    }
    for (const auto& row : rows) {
      Check(icons.contains(row.icon),
            "Inventory has a PNG icon for every item");
    }
    std::set<std::string> visiting, finished;
    std::function<void(const std::string&)> walk = [&](const auto& id) {
      Check(ids.contains(id), "Dependency resolves to a visible node");
      if (finished.contains(id)) {
        return;
      }
      Check(visiting.insert(id).second, "Dependency graph is acyclic");
      for (const auto& card : cards) {
        if (card.id == id) {
          for (const auto& parent : card.dependencies) {
            walk(parent);
          }
        }
      }
      visiting.erase(id);
      finished.insert(id);
    };
    for (const auto& card : cards) {
      walk(card.id);
    }
    auto node = [&](const std::string& id) -> const frontier::ProgressCard& {
      for (const auto& card : cards) {
        if (card.id == id) {
          return card;
        }
      }
      throw std::runtime_error("Missing progress node");
    };
    for (const auto& skill : frontier::Skills()) {
      for (const auto& parent : skill.prerequisites) {
        const auto& dependencies = node(skill.id).dependencies;
        Check(std::find(dependencies.begin(), dependencies.end(), parent) !=
                  dependencies.end(),
              "Tree edges match authoritative skill prerequisites");
      }
    }
    Check(node("gathering").ready && !node("gathering").learned &&
              !node("trading").ready,
          "Fresh tree reflects locked skills");
    frontier::TrainSkill(save, 0);
    cards = frontier::ProgressCards(save);
    Check(node("gathering").learned && node("trading").ready,
          "Learning refreshes completion and dependent readiness");
    save.gathered = 4;
    cards = frontier::ProgressCards(save);
    Check(node("crafting").detail.find("4 / 5") != std::string::npos &&
              !node("crafting").ready,
          "Activity requirement remains a real counter");
    save.gathered = 5;
    cards = frontier::ProgressCards(save);
    Check(node("crafting").ready,
          "Tree readiness matches the lesson operation");
    frontier::TrainSkill(save, 2);
    cards = frontier::ProgressCards(save);
    const auto& town_tech =
        frontier::GetContent()
            .towns[frontier::Settlements()[save.home_city].type]
            .technology;
    Check(node(town_tech).ready,
          "Known peaceful teacher shows town knowledge ready when its actual "
          "gates pass");
    frontier::Save probe = save;
    Check(frontier::LearnTechnology(probe, save.home_city).ok,
          "Town-node readiness matches authoritative teaching");
    save.relations[save.home_city].treaty = frontier::Treaty::kWar;
    cards = frontier::ProgressCards(save);
    Check(!node(town_tech).ready, "War blocks town teaching in the tree");
    Check(save.coins == 123 && save.weapon_copies.at("wood_sword") == 3 &&
              save.outfit_copies.at("forager") == 2,
          "Summary and research readiness cannot spend resources");
    std::cout << "Summary inventory, PNG coverage, graph validity and live "
                 "prerequisites passed\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
