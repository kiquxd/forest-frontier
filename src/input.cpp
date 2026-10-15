#include "input.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>
#include <vector>

#include "raylib.h"
namespace frontier {
namespace {
Settings active_settings;
std::string ui_context;
struct UiTarget {
  Rectangle box;
  std::string id;
};
std::vector<UiTarget> ui_targets;
std::string ui_focus_id;
Rectangle ui_focus_box{};
int ui_focus = -1;
bool ui_layer = true;
bool ui_activation_consumed = false;

int PadButton(InputAction action) {
  switch (action) {
    case InputAction::kTalk:
      return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    case InputAction::kHall:
    case InputAction::kQuestAction:
      return GAMEPAD_BUTTON_RIGHT_FACE_UP;
    case InputAction::kAttack:
    case InputAction::kTeacher:
      return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
    case InputAction::kGather:
      return GAMEPAD_BUTTON_LEFT_TRIGGER_2;
    case InputAction::kSprint:
      return GAMEPAD_BUTTON_LEFT_THUMB;
    case InputAction::kProgress:
      return GAMEPAD_BUTTON_MIDDLE_LEFT;
    case InputAction::kCraft:
      return GAMEPAD_BUTTON_LEFT_TRIGGER_1;
    case InputAction::kTrade:
      return GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
    case InputAction::kWeapons:
      return GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    case InputAction::kOutfits:
      return GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
    case InputAction::kHeal:
      return GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    case InputAction::kMap:
      return GAMEPAD_BUTTON_LEFT_FACE_UP;
    default:
      return -1;
  }
}
std::string PadName(InputAction action) {
  switch (PadButton(action)) {
    case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:
      return "Pad A";
    case GAMEPAD_BUTTON_RIGHT_FACE_UP:
      return "Pad Y";
    case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:
      return "Pad X";
    case GAMEPAD_BUTTON_LEFT_TRIGGER_2:
      return "Pad LT";
    case GAMEPAD_BUTTON_LEFT_THUMB:
      return "Pad L3";
    case GAMEPAD_BUTTON_MIDDLE_LEFT:
      return "Pad Select";
    case GAMEPAD_BUTTON_LEFT_TRIGGER_1:
      return "Pad LB";
    case GAMEPAD_BUTTON_RIGHT_TRIGGER_1:
      return "Pad RB";
    case GAMEPAD_BUTTON_LEFT_FACE_LEFT:
      return "Pad Left";
    case GAMEPAD_BUTTON_LEFT_FACE_RIGHT:
      return "Pad Right";
    case GAMEPAD_BUTTON_LEFT_FACE_UP:
      return "Pad Up";
    case GAMEPAD_BUTTON_LEFT_FACE_DOWN:
      return "Pad Down";
    default:
      return "";
  }
}
}  // namespace
const Settings& ActiveSettings() {
  return active_settings;
}
void SetActiveSettings(const Settings& settings) {
  active_settings = settings;
}
std::string ActionKey(InputAction action) {
  auto pad = IsGamepadAvailable(0) ? PadName(action) : std::string{};
  return (pad.empty() ? "" : pad + " / ") +
         BindingName(active_settings.bindings, action);
}
bool ActionPressed(InputAction action) {
  auto b = active_settings.bindings[static_cast<int>(action)];
  return (b.primary && IsKeyPressed(b.primary)) ||
         (b.alternative && IsKeyPressed(b.alternative)) ||
         (IsGamepadAvailable(0) && PadButton(action) >= 0 &&
          IsGamepadButtonPressed(0, PadButton(action)));
}
bool ActionDown(InputAction action) {
  auto b = active_settings.bindings[static_cast<int>(action)];
  return (b.primary && IsKeyDown(b.primary)) ||
         (b.alternative && IsKeyDown(b.alternative)) ||
         (IsGamepadAvailable(0) && PadButton(action) >= 0 &&
          IsGamepadButtonDown(0, PadButton(action)));
}
bool UiFocusActive() {
  return ui_focus >= 0;
}
void ClearUiFocus() {
  ui_focus = -1;
}
void SetUiNavigationLayer(bool enabled) {
  ui_layer = enabled;
}
void BeginUiNavigation(const std::string& context, bool enabled) {
  if (context != ui_context) {
    ui_targets.clear();
  }
  if (context != ui_context || !enabled ||
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    ui_focus = -1;
  }
  ui_context = context;
  ui_activation_consumed = false;
  ui_layer = enabled;
  if (enabled && !ui_targets.empty()) {
    if (IsKeyPressed(KEY_TAB)) {
      const int count = static_cast<int>(ui_targets.size());
      const bool reverse =
          IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
      ui_focus = ui_focus < 0 ? (reverse ? count - 1 : 0)
                              : (ui_focus + (reverse ? -1 : 1) + count) % count;
    } else if (ui_focus >= 0 &&
               ui_focus < static_cast<int>(ui_targets.size())) {
      const int dx = IsKeyPressed(KEY_RIGHT)  ? 1
                     : IsKeyPressed(KEY_LEFT) ? -1
                                              : 0;
      const int dy = IsKeyPressed(KEY_DOWN) ? 1 : IsKeyPressed(KEY_UP) ? -1 : 0;
      auto origin = ui_targets[ui_focus].box;
      const Vector2 center{origin.x + origin.width / 2,
                           origin.y + origin.height / 2};
      auto best = std::tuple{std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max()};
      int next = ui_focus;
      for (int i = 0; i < static_cast<int>(ui_targets.size()); ++i) {
        const auto& box = ui_targets[i].box;
        const float x = box.x + box.width / 2 - center.x;
        const float y = box.y + box.height / 2 - center.y;
        const float forward = x * dx + y * dy;
        if (forward <= 1) {
          continue;
        }
        const auto score = std::tuple{std::abs(x * dy - y * dx), forward};
        if (score < best) {
          best = score;
          next = i;
        }
      }
      ui_focus = next;
    }
  }
  if (ui_focus >= 0 && ui_focus < static_cast<int>(ui_targets.size())) {
    ui_focus_id = ui_targets[ui_focus].id;
    ui_focus_box = ui_targets[ui_focus].box;
  }
  ui_targets.clear();
}
bool UiFocused(Rectangle box) {
  return UiFocusActive() && ui_layer && box.x == ui_focus_box.x &&
         box.y == ui_focus_box.y && box.width == ui_focus_box.width &&
         box.height == ui_focus_box.height;
}
bool UiActivate(Rectangle box, bool enabled, const std::string& id) {
  if (box.width < 1 || box.height < 1) {
    return false;
  }
  const bool mouse = enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                     CheckCollisionPointRec(GetMousePosition(), box);
  if (!ui_layer) {
    return mouse;
  }
  const int index = static_cast<int>(ui_targets.size());
  const std::string key =
      id.empty() ? std::to_string(box.x) + ":" + std::to_string(box.y) : id;
  ui_targets.push_back({box, key});
  const bool keyboard = enabled && index == ui_focus && key == ui_focus_id &&
                        IsKeyPressed(KEY_ENTER) && !ui_activation_consumed &&
                        !IsKeyDown(KEY_LEFT_CONTROL) &&
                        !IsKeyDown(KEY_RIGHT_CONTROL);
  if (keyboard) {
    ui_activation_consumed = true;
  }
  return mouse || keyboard;
}
void EndUiNavigation() {
  if (ui_focus >= 0 && ui_focus < static_cast<int>(ui_targets.size()) &&
      ui_targets[ui_focus].id == ui_focus_id) {
    auto box = ui_targets[ui_focus].box;
    DrawRectangleRoundedLinesEx(
        {box.x - 2, box.y - 2, box.width + 4, box.height + 4}, .15f, 8, 3,
        {239, 199, 118, 255});
  } else if (ui_focus >= 0) {
    ui_focus = -1;
  }
}
bool MenuConfirm() {
  return !UiFocusActive() &&
         (IsKeyPressed(KEY_ENTER) ||
          (IsGamepadAvailable(0) &&
           IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)));
}
bool MenuBack() {
  return IsKeyPressed(KEY_ESCAPE) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
}
bool MenuUp() {
  return !UiFocusActive() &&
         (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) ||
          (IsGamepadAvailable(0) &&
           IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)));
}
bool MenuDown() {
  return !UiFocusActive() &&
         (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) ||
          (IsGamepadAvailable(0) &&
           IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)));
}
bool MenuLeft() {
  return !UiFocusActive() &&
         (IsKeyPressed(KEY_LEFT) ||
          (IsGamepadAvailable(0) &&
           IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)));
}
bool MenuRight() {
  return !UiFocusActive() &&
         (IsKeyPressed(KEY_RIGHT) ||
          (IsGamepadAvailable(0) &&
           IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)));
}
void ApplyWindowSettings(const Settings& settings) {
  if (IsWindowFullscreen() != settings.fullscreen) {
    if (settings.fullscreen) {
      SetWindowSize(GetMonitorWidth(GetCurrentMonitor()),
                    GetMonitorHeight(GetCurrentMonitor()));
    }
    ToggleFullscreen();
  }
  if (!settings.fullscreen) {
    SetWindowSize(settings.width, settings.height);
  }
  SetTargetFPS(settings.fps);
  SetActiveSettings(settings);
}
}  // namespace frontier
