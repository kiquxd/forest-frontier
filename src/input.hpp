#ifndef FOREST_FRONTIER_SRC_INPUT_HPP_
#define FOREST_FRONTIER_SRC_INPUT_HPP_
#include <string>

#include "save_slots.hpp"
namespace frontier {
const Settings& ActiveSettings();
void SetActiveSettings(const Settings& settings);
bool ActionPressed(InputAction action);
bool ActionDown(InputAction action);
std::string ActionKey(InputAction action);
bool MenuConfirm();
bool MenuBack();
bool MenuUp();
bool MenuDown();
bool MenuLeft();
bool MenuRight();
void ApplyWindowSettings(const Settings& settings);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_INPUT_HPP_
