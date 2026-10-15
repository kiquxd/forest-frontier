#ifndef FOREST_FRONTIER_SRC_INPUT_HPP_
#define FOREST_FRONTIER_SRC_INPUT_HPP_
#include <string>

#include "raylib.h"
#include "save_slots.hpp"
namespace frontier {
const Settings& ActiveSettings();
void SetActiveSettings(const Settings& settings);
bool ActionPressed(InputAction action);
bool ActionDown(InputAction action);
std::string ActionKey(InputAction action);
// Tab opts into widget focus; native arrow-based menus remain usable before it.
void BeginUiNavigation(const std::string& context, bool enabled = true);
void SetUiNavigationLayer(bool enabled);
void EndUiNavigation();
bool UiActivate(Rectangle box, bool enabled = true, const std::string& id = "");
bool UiFocused(Rectangle box);
bool UiFocusActive();
void ClearUiFocus();
bool MenuConfirm();
bool MenuBack();
bool MenuUp();
bool MenuDown();
bool MenuLeft();
bool MenuRight();
void ApplyWindowSettings(const Settings& settings);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_INPUT_HPP_
