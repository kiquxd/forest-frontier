#pragma once
#include "adventure.hpp"

namespace frontier {
struct SummaryEntry {std::string icon,name,detail;int count{-1};};
struct ProgressCard {
  std::string id,name,icon,detail;
  std::vector<std::string> dependencies;
  int column{},row{};
  bool learned{},ready{};
  bool activity{};
  int amount{};
};
std::vector<SummaryEntry> inventorySummary(const Save& save);
std::vector<ProgressCard> progressCards(const Save& save);
}
