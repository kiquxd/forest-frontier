#ifndef FOREST_FRONTIER_SRC_SUMMARY_HPP_
#define FOREST_FRONTIER_SRC_SUMMARY_HPP_

#include <string>
#include <vector>

#include "adventure.hpp"

namespace frontier {
struct SummaryEntry {
  std::string icon;
  std::string name;
  std::string detail;
  int count{-1};
};
struct ProgressCard {
  std::string id;
  std::string name;
  std::string icon;
  std::string detail;
  std::vector<std::string> dependencies;
  int column{};
  int row{};
  bool learned{};
  bool ready{};
  bool activity{};
  int amount{};
};
std::string RecipeIcon(const Recipe& recipe);
std::vector<SummaryEntry> InventorySummary(const Save& save);
std::vector<ProgressCard> ProgressCards(const Save& save);
// Select the nearest card in a grid direction; never change progression.
int AdjacentProgressCard(const std::vector<ProgressCard>& cards, int selected,
                         int dx, int dy);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_SUMMARY_HPP_
