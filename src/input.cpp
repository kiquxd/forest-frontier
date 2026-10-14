#include "input.hpp"

#include "raylib.h"
namespace frontier {
namespace {
Settings active_settings;
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
bool MenuConfirm() {
  return IsKeyPressed(KEY_ENTER) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
}
bool MenuBack() {
  return IsKeyPressed(KEY_ESCAPE) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
}
bool MenuUp() {
  return IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP));
}
bool MenuDown() {
  return IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN));
}
bool MenuLeft() {
  return IsKeyPressed(KEY_LEFT) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
}
bool MenuRight() {
  return IsKeyPressed(KEY_RIGHT) ||
         (IsGamepadAvailable(0) &&
          IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
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
