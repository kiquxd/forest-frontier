#include "progress.hpp"

#include <algorithm>
#include <chrono>

#include "nation.hpp"
#include "settlement_system.hpp"
namespace frontier {
void RecordHistory(Save& save, const std::string& kind,
                   const std::string& description, int city) {
  auto now = std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
                 .count();
  save.history.push_back({kind, description, now, city});
  if (save.history.size() > 512) {
    save.history.erase(save.history.begin());
  }
}
void RememberResources(Save& save) {
  for (int g = 0; g < kGoodTypes; ++g) {
    if (GoodCount(save.inventory, g) > 0) {
      save.known_goods[g] = true;
    }
  }
}
Action UnlockProgress(Save& save, const std::string& id) {
  for (int i = 0; i < static_cast<int>(Skills().size()); ++i) {
    if (Skills()[i].id == id) {
      return TrainSkill(save, i);
    }
  }
  for (int i = 0; i < static_cast<int>(ResearchNodes().size()); ++i) {
    if (ResearchNodes()[i].id == id) {
      return Research(save, i);
    }
  }
  return {false,
          "Visit the indicated town master or complete the town project."};
}
std::vector<TownReport> TownReports(const Save& save,
                                    const std::vector<Npc>& npcs) {
  std::vector<TownReport> result;
  for (int i = 0; i < static_cast<int>(Settlements().size()); ++i) {
    if (i >= static_cast<int>(save.relations.size()) ||
        !save.relations[i].discovered) {
      continue;
    }
    const auto& town = Settlements()[i];
    const auto& type = GetContent().towns[town.type];
    TownReport report;
    report.city = i;
    report.owned = OwnsCity(save, i);
    report.name = town.name;
    report.specialty = type.name;
    report.technology = TechnologyName(type.technology);
    for (const auto& npc : npcs) {
      if (npc.settlement != i) {
        continue;
      }
      if (npc.origin_city >= 0) {
        ++report.visitors;
      } else if (!npc.ruler && ResidentActive(save, npc.id)) {
        ++report.residents;
      }
    }
    if (save.nation && report.owned) {
      report.residents = CityPopulation(save, i);
    }
    for (const auto& resident : save.citizens) {
      if (resident.city == i && resident.state == "active" &&
          resident.job != "idle") {
        ++report.workers;
      }
    }
    for (const auto& wall : save.walls) {
      if (wall.city == i) {
        ++report.walls;
        report.wall_health += wall.hp;
      }
    }
    result.push_back(report);
  }
  return result;
}
std::vector<SummaryEntry> ResourceCatalog(const Save& save) {
  std::vector<SummaryEntry> rows;
  static const char* kSources[] = {"Gather trees",
                                   "Gather stone sites",
                                   "Gather berry bushes",
                                   "Gather reeds",
                                   "Fish with a rod",
                                   "Defeat wolves",
                                   "Mine ore; ironworking",
                                   "Mine ore; specialist towns",
                                   "Underwater deposits; specialist knowledge",
                                   "Underwater pearls; diving knowledge",
                                   "Craft steel after steelworking",
                                   "Specialist town trade",
                                   "Underwater salvage; ancient knowledge"};
  for (int g = 0; g < kGoodTypes; ++g) {
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
    bool known = save.known_goods[g] || count > 0;
    rows.push_back(
        {known ? "good_" + std::to_string(g) : "book",
         known ? GoodName(g) : "Undiscovered resource",
         known ? kSources[g]
               : "Obtain this resource through exploration, trade or quests.",
         known ? count : -1});
  }
  return rows;
}
}  // namespace frontier
