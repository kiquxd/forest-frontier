#include "controls.hpp"

#include <set>

namespace frontier {
const std::vector<ActionDefinition>& InputActions() {
  // raylib keyboard codes are serialized without a graphics dependency.
  static const std::vector<ActionDefinition> kActions = {
      {InputAction::kUp, "up", "Move up", {87, 265}},
      {InputAction::kDown, "down", "Move down", {83, 264}},
      {InputAction::kLeft, "left", "Move left", {65, 263}},
      {InputAction::kRight, "right", "Move right", {68, 262}},
      {InputAction::kSprint, "sprint", "Sprint", {340, 344}},
      {InputAction::kTalk, "talk", "Talk", {69, 0}},
      {InputAction::kHall, "hall", "Town hall", {80, 0}},
      {InputAction::kGather, "gather", "Gather (hold)", {70, 0}},
      {InputAction::kAttack, "attack", "Attack", {32, 0}},
      {InputAction::kCraft, "craft", "Crafting", {67, 0}},
      {InputAction::kTrade, "trade", "Trading", {84, 0}},
      {InputAction::kWeapons, "weapons", "Weapons", {73, 0}},
      {InputAction::kOutfits, "outfits", "Outfits", {79, 0}},
      {InputAction::kProgress, "progress", "Progress", {75, 0}},
      {InputAction::kQuests, "quests", "Quest journal", {76, 0}},
      {InputAction::kEat, "eat", "Eat", {81, 0}},
      {InputAction::kHeal, "heal", "Use potion", {72, 0}},
      {InputAction::kCampfire, "campfire", "Place campfire", {66, 0}},
      {InputAction::kVehicle, "vehicle", "Select vessel", {86, 0}},
      {InputAction::kDive, "dive", "Dive / surface", {82, 0}},
      {InputAction::kSonar, "sonar", "Sonar", {71, 0}},
      {InputAction::kRepair, "repair", "Repair hull", {74, 0}},
      {InputAction::kMap, "map", "World map", {77, 0}},
      {InputAction::kDebug, "debug", "Performance HUD", {292, 0}},
      {InputAction::kFullscreen, "fullscreen", "Fullscreen", {300, 0}},
      {InputAction::kAutoroute, "autoroute", "Debug auto-route", {258, 0}},
      {InputAction::kQuestAction,
       "quest_action",
       "Accept / finish quest",
       {49, 0}},
      {InputAction::kTeacher, "teacher", "Specialist lesson", {293, 0}},
      {InputAction::kResidentJob, "resident_job", "Resident job", {51, 0}},
      {InputAction::kResidentWeapon,
       "resident_weapon",
       "Resident weapon",
       {52, 0}},
      {InputAction::kResidentOutfit,
       "resident_outfit",
       "Resident outfit",
       {53, 0}},
      {InputAction::kResidentAssign,
       "resident_assign",
       "Assign resident",
       {54, 0}}};
  return kActions;
}
Bindings DefaultBindings() {
  Bindings result{};
  for (const auto& action : InputActions()) {
    result[static_cast<int>(action.action)] = action.defaults;
  }
  return result;
}
std::string KeyName(int key) {
  if (!key) {
    return "Unbound";
  }
  if (key >= 65 && key <= 90) {
    return std::string(1, static_cast<char>(key));
  }
  if (key >= 48 && key <= 57) {
    return std::string(1, static_cast<char>(key));
  }
  if (key >= 290 && key <= 301) {
    return "F" + std::to_string(key - 289);
  }
  switch (key) {
    case 32:
      return "Space";
    case 258:
      return "Tab";
    case 262:
      return "Right";
    case 263:
      return "Left";
    case 264:
      return "Down";
    case 265:
      return "Up";
    case 340:
      return "L Shift";
    case 344:
      return "R Shift";
    case 341:
      return "L Ctrl";
    case 345:
      return "R Ctrl";
    case 342:
      return "L Alt";
    case 346:
      return "R Alt";
    default:
      return key >= 33 && key <= 96 ? std::string(1, static_cast<char>(key))
                                    : "Key " + std::to_string(key);
  }
}
std::string BindingName(const Bindings& bindings, InputAction action) {
  const auto& binding = bindings[static_cast<int>(action)];
  return KeyName(binding.primary) +
         (binding.alternative ? " / " + KeyName(binding.alternative) : "");
}
bool ValidBindingKey(int key) {
  return key == 0 || (key >= 32 && key <= 96) || key == 258 ||
         (key >= 262 && key <= 265) || (key >= 290 && key <= 301) ||
         (key >= 340 && key <= 346);
}
bool ValidBindings(const Bindings& bindings) {
  std::set<int> used;
  for (const auto& binding : bindings) {
    if (!binding.primary || !ValidBindingKey(binding.primary) ||
        !ValidBindingKey(binding.alternative)) {
      return false;
    }
    for (int key : {binding.primary, binding.alternative}) {
      if (key && !used.insert(key).second) {
        return false;
      }
    }
  }
  return true;
}
int BindingConflict(const Bindings& bindings, InputAction action, int slot,
                    int key) {
  if (!key) {
    return -1;
  }
  for (int i = 0; i < kInputActions; ++i) {
    if (!(i == static_cast<int>(action) && slot == 0) &&
        bindings[i].primary == key) {
      return i;
    }
    if (!(i == static_cast<int>(action) && slot == 1) &&
        bindings[i].alternative == key) {
      return i;
    }
  }
  return -1;
}
bool SetBinding(Bindings& bindings, InputAction action, int slot, int key,
                bool swap_conflict) {
  if (slot < 0 || slot > 1 || !ValidBindingKey(key) || (!slot && !key)) {
    return false;
  }
  int index = static_cast<int>(action);
  if (index < 0 || index >= kInputActions) {
    return false;
  }
  int conflict = BindingConflict(bindings, action, slot, key);
  if (conflict >= 0 && !swap_conflict) {
    return false;
  }
  Bindings candidate = bindings;
  int& selected =
      slot ? candidate[index].alternative : candidate[index].primary;
  int previous = selected;
  if (conflict >= 0) {
    int& other = candidate[conflict].primary == key
                     ? candidate[conflict].primary
                     : candidate[conflict].alternative;
    other = previous;
  }
  selected = key;
  if (!ValidBindings(candidate)) {
    return false;
  }
  bindings = candidate;
  return true;
}
}  // namespace frontier
