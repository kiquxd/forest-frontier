#ifndef FOREST_FRONTIER_SRC_CONTROLS_HPP_
#define FOREST_FRONTIER_SRC_CONTROLS_HPP_

#include <array>
#include <string>
#include <vector>

namespace frontier {
enum class InputAction {
  kUp,
  kDown,
  kLeft,
  kRight,
  kSprint,
  kTalk,
  kHall,
  kGather,
  kAttack,
  kCraft,
  kTrade,
  kWeapons,
  kOutfits,
  kProgress,
  kQuests,
  kEat,
  kHeal,
  kCampfire,
  kVehicle,
  kDive,
  kSonar,
  kRepair,
  kMap,
  kDebug,
  kFullscreen,
  kAutoroute,
  kQuestAction,
  kTeacher,
  kResidentJob,
  kResidentWeapon,
  kResidentOutfit,
  kResidentAssign,
  kCount
};
constexpr int kInputActions = static_cast<int>(InputAction::kCount);
struct KeyBinding {
  int primary{};
  int alternative{};
};
using Bindings = std::array<KeyBinding, kInputActions>;
struct ActionDefinition {
  InputAction action;
  std::string id;
  std::string name;
  KeyBinding defaults;
};
const std::vector<ActionDefinition>& InputActions();
Bindings DefaultBindings();
std::string KeyName(int key);
std::string BindingName(const Bindings& bindings, InputAction action);
bool ValidBindingKey(int key);
bool ValidBindings(const Bindings& bindings);
// Return the conflicting action, excluding the selected slot.
int BindingConflict(const Bindings& bindings, InputAction action, int slot,
                    int key);
bool SetBinding(Bindings& bindings, InputAction action, int slot, int key,
                bool swap_conflict);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_CONTROLS_HPP_
