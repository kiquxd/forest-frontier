#ifndef FOREST_FRONTIER_SRC_PROGRESS_HPP_
#define FOREST_FRONTIER_SRC_PROGRESS_HPP_
#include <string>
#include <vector>

#include "summary.hpp"
namespace frontier {
void RememberResources(Save& save);
void RecordHistory(Save& save, const std::string& kind,
                   const std::string& description, int city = -1);
Action UnlockProgress(Save& save, const std::string& id);
struct TownReport {
  int city{};
  int residents{};
  int visitors{};
  int workers{};
  int walls{};
  int wall_health{};
  bool owned{};
  std::string name;
  std::string specialty;
  std::string technology;
};
std::vector<TownReport> TownReports(const Save& save,
                                    const std::vector<Npc>& npcs);
std::vector<SummaryEntry> ResourceCatalog(const Save& save);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_PROGRESS_HPP_
