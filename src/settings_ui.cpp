#include "settings_ui.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <vector>

#include "input.hpp"
namespace frontier {
namespace {
bool SettingButton(Renderer& renderer, Rectangle box, const std::string& text,
                   bool selected = false) {
  bool hover = CheckCollisionPointRec(GetMousePosition(), box);
  DrawRectangleRounded(
      box, .5f, 12,
      selected || hover ? Color{78, 103, 65, 255} : Color{31, 49, 40, 255});
  renderer.Wrapped(text, box.x + 12, box.y + 10, box.width - 24, 17,
                   {232, 237, 218, 255}, 1);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}  // namespace
bool DrawSettingsPanel(Renderer& renderer, SettingsPanel& panel,
                       Settings& settings, const std::filesystem::path& path) {
  float w = static_cast<float>(GetScreenWidth()),
        h = static_cast<float>(GetScreenHeight());
  Rectangle box{40, 110, w - 80, h - 185};
  DrawRectangleRounded(box, .03f, 8, {19, 33, 28, 255});
  renderer.Text(panel.controls ? "Controls" : "Settings", 60, 123, 26,
                {232, 193, 117, 255});
  if (panel.controls) {
    int rows = std::max(1, static_cast<int>((box.height - 140) / 48));
    if (!panel.capturing && panel.pending_key < 0) {
      if (MenuUp()) {
        panel.binding_row =
            (panel.binding_row + kInputActions - 1) % kInputActions;
      }
      if (MenuDown()) {
        panel.binding_row = (panel.binding_row + 1) % kInputActions;
      }
      if (MenuLeft() || MenuRight()) {
        panel.binding_slot = 1 - panel.binding_slot;
      }
      if (MenuConfirm()) {
        panel.capturing = true;
      }
      if (IsKeyPressed(KEY_DELETE) && panel.binding_slot) {
        SetBinding(panel.draft.bindings,
                   static_cast<InputAction>(panel.binding_row), 1, 0, false);
      }
    }
    int first = panel.binding_row / rows * rows;
    float key_width = std::min(220.f, box.width * .24f);
    for (int i = first; i < std::min(first + rows, kInputActions); ++i) {
      float y = box.y + 58 + (i - first) * 48;
      renderer.Wrapped(InputActions()[i].name, 60, y + 8,
                       box.width - 2 * key_width - 70, 17, {229, 234, 218, 255},
                       1);
      const auto& binding = panel.draft.bindings[i];
      for (int slot = 0; slot < 2; ++slot) {
        Rectangle key{box.x + box.width - 2 * key_width - 20 + slot * key_width,
                      y, key_width - 8, 40};
        if (SettingButton(
                renderer, key,
                KeyName(slot ? binding.alternative : binding.primary),
                panel.binding_row == i && panel.binding_slot == slot) &&
            !panel.capturing && panel.pending_key < 0) {
          panel.binding_row = i;
          panel.binding_slot = slot;
          panel.capturing = true;
        }
      }
    }
    if (panel.capturing) {
      renderer.Wrapped(
          "Press a key. Esc cancels. Enter, Esc, F5 and paging keys are "
          "reserved for menus.",
          60, box.y + box.height - 65, box.width - 40, 17, {239, 199, 118, 255},
          2);
      int key = GetKeyPressed();
      if (key == KEY_ESCAPE) {
        panel.capturing = false;
      } else if (key) {
        if (!ValidBindingKey(key)) {
          panel.notice = "This key is reserved for menus.";
        } else {
          panel.pending_key = key;
          panel.conflict = BindingConflict(
              panel.draft.bindings, static_cast<InputAction>(panel.binding_row),
              panel.binding_slot, key);
          if (panel.conflict < 0) {
            SetBinding(panel.draft.bindings,
                       static_cast<InputAction>(panel.binding_row),
                       panel.binding_slot, key, false);
            panel.pending_key = -1;
          }
          panel.capturing = false;
        }
      }
    } else {
      renderer.Wrapped(
          "Up/Down: action · Left/Right: primary/alternate · Enter: rebind · "
          "Del: clear alternate",
          60, box.y + box.height - 65, box.width - 40, 16, {165, 187, 157, 255},
          2);
    }
    if (panel.pending_key >= 0) {
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 190});
      Rectangle dialog{w * .5f - 280, h * .5f - 120, 560, 240};
      DrawRectangleRounded(dialog, .1f, 10, {24, 42, 33, 255});
      renderer.Wrapped(
          KeyName(panel.pending_key) + " is assigned to " +
              InputActions()[panel.conflict].name + ". Swap the two bindings?",
          dialog.x + 20, dialog.y + 20, 520, 20, {237, 223, 181, 255}, 3);
      if (SettingButton(renderer, {dialog.x + 20, dialog.y + 170, 240, 44},
                        "Swap · Enter") ||
          MenuConfirm()) {
        if (!SetBinding(panel.draft.bindings,
                        static_cast<InputAction>(panel.binding_row),
                        panel.binding_slot, panel.pending_key, true)) {
          panel.notice =
              "Cannot replace a required primary key with an unbound "
              "alternate.";
        }
        panel.pending_key = -1;
      }
      if (SettingButton(renderer, {dialog.x + 300, dialog.y + 170, 240, 44},
                        "Cancel · Esc") ||
          MenuBack()) {
        panel.pending_key = -1;
      }
    }
    if (!panel.capturing && panel.pending_key < 0 &&
        SettingButton(renderer, {w - 420, h - 65, 180, 44}, "Reset controls")) {
      panel.draft.bindings = DefaultBindings();
      panel.notice = "Default controls restored; Apply to save.";
    }
  } else {
    std::vector<std::string> labels = {
        std::string("Fullscreen: ") + (panel.draft.fullscreen ? "on" : "off"),
        "Window: " + std::to_string(panel.draft.width) + " x " +
            std::to_string(panel.draft.height),
        "Frame limit: " +
            (panel.draft.fps ? std::to_string(panel.draft.fps) : "uncapped"),
        "Camera zoom: " +
            std::to_string(static_cast<int>(panel.draft.zoom * 100)) + "%",
        std::string("Performance HUD: ") + (panel.draft.debug ? "on" : "off"),
        "Text size: " +
            std::to_string(static_cast<int>(panel.draft.text_scale * 100)) +
            "%",
        std::string("Tree contrast: ") +
            (panel.draft.high_contrast ? "high" : "standard"),
        "Keyboard controls"};
    if (MenuUp()) {
      panel.selection = (panel.selection + 7) % 8;
    }
    if (MenuDown()) {
      panel.selection = (panel.selection + 1) % 8;
    }
    for (int i = 0; i < 8; ++i) {
      if (SettingButton(
              renderer,
              {60, box.y + 50 + i * 48, std::min(box.width - 40, 740.f), 40},
              labels[i], panel.selection == i) ||
          (panel.selection == i &&
           (MenuConfirm() || MenuLeft() || MenuRight()))) {
        panel.selection = i;
        int delta = MenuLeft() ? -1 : 1;
        if (i == 0) {
          panel.draft.fullscreen = !panel.draft.fullscreen;
        }
        if (i == 1) {
          const std::vector<std::pair<int, int>> sizes = {
              {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}};
          int selected = 0;
          for (int j = 0; j < 4; ++j) {
            if (sizes[j].first == panel.draft.width) {
              selected = j;
            }
          }
          auto size = sizes[(selected + delta + 4) % 4];
          panel.draft.width = size.first;
          panel.draft.height = size.second;
        }
        if (i == 2) {
          const std::vector<int> caps = {30, 60, 120, 144, 0};
          auto it = std::find(caps.begin(), caps.end(), panel.draft.fps);
          int selected =
              it == caps.end() ? 1 : static_cast<int>(it - caps.begin());
          panel.draft.fps = caps[(selected + delta + 5) % 5];
        }
        if (i == 3) {
          panel.draft.zoom =
              std::clamp(panel.draft.zoom + delta * .1f, .65f, 1.7f);
        }
        if (i == 4) {
          panel.draft.debug = !panel.draft.debug;
        }
        if (i == 5) {
          int step =
              static_cast<int>(std::round((panel.draft.text_scale - 1) * 5));
          panel.draft.text_scale = 1 + ((step + delta + 3) % 3) * .2f;
        }
        if (i == 6) {
          panel.draft.high_contrast = !panel.draft.high_contrast;
        }
        if (i == 7) {
          panel.controls = true;
        }
      }
    }
  }
  if (!panel.capturing && panel.pending_key < 0) {
    if (SettingButton(renderer, {40, h - 65, 240, 44}, "Apply settings · F5") ||
        IsKeyPressed(KEY_F5)) {
      try {
        WriteSettings(path, panel.draft);
        settings = panel.draft;
        ApplyWindowSettings(settings);
        panel.notice = "Settings saved";
      } catch (const std::exception& error) {
        panel.notice = error.what();
      }
    }
    if (SettingButton(renderer, {w - 220, h - 65, 180, 44}, "Back · Esc") ||
        MenuBack()) {
      if (panel.controls) {
        panel.controls = false;
      } else {
        return true;
      }
    }
  }
  if (!panel.notice.empty()) {
    DrawRectangleRounded({40, h - 105, w - 80, 34}, .4f, 8, {12, 24, 18, 250});
    renderer.Wrapped(panel.notice, 52, h - 100, w - 104, 16,
                     {232, 198, 126, 255}, 1);
  }
  return false;
}
}  // namespace frontier
