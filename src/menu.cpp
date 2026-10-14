#include "menu.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "input.hpp"
#include "rlgl.h"
#include "settings_ui.hpp"

namespace frontier {
namespace {
constexpr Color kBackground{17, 26, 25, 255};
constexpr Color kPanel{27, 39, 35, 255};
constexpr Color kInk{228, 232, 214, 255};
constexpr Color kMuted{143, 164, 146, 255};
constexpr Color kGold{231, 191, 114, 255};
bool Button(Renderer& renderer, Rectangle box, const std::string& label,
            bool active = true, bool chosen = false) {
  bool hover = CheckCollisionPointRec(GetMousePosition(), box);
  DrawRectangleRounded({box.x, box.y + 3, box.width, box.height}, .65f, 16,
                       {7, 15, 14, 100});
  DrawRectangleRounded(box, .65f, 16,
                       chosen            ? Color{81, 99, 64, 255}
                       : hover && active ? Color{58, 76, 54, 255}
                                         : Color{36, 51, 44, 255});
  renderer.Wrapped(
      label, box.x + 16,
      box.y + std::round((box.height - Renderer::TextSize(18)) * .5f),
      box.width - 32, 18, active ? kInk : kMuted, 1);
  return active && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
void EditField(std::string& text, bool& selected, size_t limit,
               bool digits = false) {
  bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (control && IsKeyPressed(KEY_A)) {
    selected = true;
  }
  if (control && IsKeyPressed(KEY_C)) {
    SetClipboardText(text.c_str());
  }
  if (control && IsKeyPressed(KEY_X)) {
    SetClipboardText(text.c_str());
    text.clear();
    selected = false;
  }
  auto append = [&](const std::string& value) {
    if (selected) {
      text.clear();
      selected = false;
    }
    for (unsigned char code : value) {
      if (code >= 32 && code < 127 && (!digits || std::isdigit(code)) &&
          text.size() < limit) {
        text += static_cast<char>(code);
      }
    }
  };
  if (control && IsKeyPressed(KEY_V)) {
    const char* clipboard = GetClipboardText();
    if (clipboard) {
      append(clipboard);
    }
  }
  for (int code = GetCharPressed(); code; code = GetCharPressed()) {
    if (!control && code >= 32 && code < 127) {
      append(std::string(1, static_cast<char>(code)));
    }
  }
  if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE) ||
      IsKeyPressed(KEY_DELETE)) {
    if (selected) {
      text.clear();
      selected = false;
    } else if (!text.empty()) {
      text.pop_back();
    }
  }
}
void Capture(const std::filesystem::path& path) {
  if (path.empty()) {
    return;
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  Image screenshot{rlReadScreenPixels(GetRenderWidth(), GetRenderHeight()),
                   GetRenderWidth(), GetRenderHeight(), 1,
                   PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  if (!screenshot.data) {
    throw std::runtime_error("Cannot capture menu");
  }
  bool ok = ExportImage(screenshot, path.c_str());
  UnloadImage(screenshot);
  if (!ok) {
    throw std::runtime_error("Cannot export menu screenshot");
  }
}
std::string UpdatedAt(std::filesystem::file_time_type value) {
  auto time = std::chrono::system_clock::to_time_t(
      std::chrono::file_clock::to_sys(value));
  std::tm local{};
  localtime_r(&time, &local);
  std::ostringstream out;
  out << std::put_time(&local, "%Y-%m-%d %H:%M");
  return out.str();
}
int MenuScene() {
  const char* names[] = {"forest", "coast", "town", "underwater"};
  if (const char* requested = std::getenv("FRONTIER_MENU_SCENE")) {
    for (int i = 0; i < 4; i++) {
      if (std::string(requested) == names[i]) {
        return i;
      }
    }
  }
  static int previous = -1;
  unsigned roll = std::random_device{}();
  int chosen = previous < 0 ? static_cast<int>(roll % 4)
                            : (previous + 1 + static_cast<int>(roll % 3)) % 4;
  previous = chosen;
  return chosen;
}
void LandscapeTriangle(Vector2 a, Vector2 b, Vector2 c, Color color) {
  // Screen-space Y is downward; keep all procedural faces front-facing.
  if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0) {
    std::swap(b, c);
  }
  DrawTriangle(a, b, c, color);
}
void Landscape(int scene, float w, float h) {
  const Color skies[] = {Color{36, 70, 67, 255}, Color{48, 82, 111, 255},
                         Color{83, 65, 87, 255}, Color{15, 64, 78, 255}};
  DrawRectangleGradientV(0, 0, static_cast<int>(w), static_cast<int>(h),
                         skies[scene], {12, 25, 28, 255});
  float horizon = h * .52f;
  if (scene != 3) {
    DrawCircleV(
        {w * .79f, h * .2f}, h * .07f,
        scene == 2 ? Color{238, 184, 125, 255} : Color{213, 226, 203, 255});
    for (int i = 0; i < 8; i++) {
      float x = i * w / 6.f, peak = h * (.26f + .07f * std::sin(i * 2.f));
      LandscapeTriangle({x, horizon}, {x + w * .32f, horizon},
                        {x + w * .16f, peak}, {39, 64, 69, 255});
    }
    DrawRectangleGradientV(0, static_cast<int>(horizon), static_cast<int>(w),
                           static_cast<int>(h - horizon), {30, 72, 82, 255},
                           {14, 35, 40, 255});
    for (int i = 0; i < 30; i++) {
      float x = w * .45f + std::sin(i * 4.f) * w * .45f,
            y = horizon + static_cast<float>(i % 12) * h * .036f;
      DrawLineEx({x, y}, {x + 30 + i % 5 * 15.f, y}, 1, {86, 132, 135, 60});
    }
  }
  if (scene == 0) {
    for (int i = 0; i < 26; i++) {
      float x = i * w / 24.f, base = h * (.59f + .045f * std::sin(i * 2.3f)),
            height = h * (.1f + .06f * (i % 3));
      DrawRectangle(static_cast<int>(x - 3),
                    static_cast<int>(base - height * .4f), 6,
                    static_cast<int>(height * .5f), {48, 54, 35, 255});
      LandscapeTriangle({x, base - height}, {x + height * .37f, base},
                        {x - height * .37f, base}, {21, 48, 38, 255});
      LandscapeTriangle(
          {x, base - height * 1.15f}, {x + height * .29f, base - height * .24f},
          {x - height * .29f, base - height * .24f}, {31, 65, 45, 255});
    }
    DrawEllipse(static_cast<int>(w * .75f), static_cast<int>(h * .91f),
                w * .46f, h * .13f, {17, 39, 31, 255});
  } else if (scene == 1) {
    LandscapeTriangle({w * .88f, h * .54f}, {w, h}, {w * .65f, h},
                      {42, 58, 52, 255});
    float x = w * .72f, y = h * .7f;
    LandscapeTriangle({x - 90, y}, {x + 95, y}, {x + 40, y + 30},
                      {103, 71, 44, 255});
    DrawLineEx({x, y}, {x, y - 155}, 4, {148, 116, 70, 255});
    LandscapeTriangle({x + 5, y - 145}, {x + 85, y - 12}, {x + 5, y - 12},
                      {210, 208, 173, 255});
    LandscapeTriangle({x - 5, y - 122}, {x - 5, y - 12}, {x - 70, y - 12},
                      {174, 190, 160, 255});
    float lx = w * .93f, ly = h * .49f;
    DrawRectangle(static_cast<int>(lx - 15), static_cast<int>(ly - 105), 30,
                  110, {172, 166, 132, 255});
    DrawRectangle(static_cast<int>(lx - 19), static_cast<int>(ly - 110), 38, 20,
                  {220, 183, 97, 255});
    LandscapeTriangle({lx, ly - 135}, {lx + 25, ly - 111}, {lx - 25, ly - 111},
                      {86, 69, 52, 255});
  } else if (scene == 2) {
    for (int i = 0; i < 11; i++) {
      float x = w * .48f + i * w * .05f, y = horizon - 10 + (i % 3) * 18;
      float bh = 55 + (i % 4) * 17.f;
      DrawRectangle(static_cast<int>(x), static_cast<int>(y - bh),
                    static_cast<int>(w * .052f), static_cast<int>(bh),
                    {88, 94, 73, 255});
      LandscapeTriangle({x - 5, y - bh}, {x + w * .056f, y - bh},
                        {x + w * .026f, y - bh - 30}, {121, 82, 57, 255});
      DrawRectangle(static_cast<int>(x + 12), static_cast<int>(y - bh + 20), 8,
                    13, {224, 180, 104, 255});
      DrawRectangle(static_cast<int>(x + 30), static_cast<int>(y - bh + 20), 8,
                    13, {224, 180, 104, 255});
    }
    DrawRectangle(static_cast<int>(w * .46f), static_cast<int>(horizon + 8),
                  static_cast<int>(w * .54f), 28, {63, 78, 69, 255});
    for (int i = 0; i < 18; i++) {
      DrawRectangle(static_cast<int>(w * .46f + i * w * .033f),
                    static_cast<int>(horizon - 2), 18, 14, {79, 90, 73, 255});
    }
    DrawEllipse(static_cast<int>(w * .8f), static_cast<int>(h * .84f), w * .28f,
                h * .12f, {25, 43, 39, 255});
  } else {
    for (int i = 0; i < 7; i++) {
      LandscapeTriangle({w * (.46f + i * .085f), 0}, {w * (.5f + i * .085f), 0},
                        {w * (.22f + i * .12f), h}, {53, 124, 133, 24});
    }
    DrawEllipse(static_cast<int>(w * .74f), static_cast<int>(h * 1.02f),
                w * .6f, h * .28f, {19, 44, 48, 255});
    for (int i = 0; i < 16; i++) {
      float x = w * (.4f + i * .04f), y = h * (.87f + .06f * std::sin(i));
      DrawLineEx({x, y}, {x + 10, y - 55 - (i % 3) * 22.f}, 5,
                 {39, 106, 79, 255});
      DrawCircleV({x + 10, y - 55 - (i % 3) * 22.f}, 6, {54, 130, 96, 255});
      DrawCircleLines(static_cast<int>(x),
                      static_cast<int>(y - 140 - (i % 4) * 55), 3 + i % 4,
                      {94, 168, 169, 120});
    }
    float x = w * .77f, y = h * .48f;
    DrawEllipse(static_cast<int>(x), static_cast<int>(y), 96, 31,
                {165, 151, 83, 255});
    DrawRectangleRounded({x - 31, y - 51, 61, 28}, .4f, 8, {124, 126, 78, 255});
    DrawLineEx({x + 4, y - 49}, {x + 4, y - 78}, 6, {173, 171, 108, 255});
    for (int i = 0; i < 4; i++) {
      DrawCircleV({x - 58 + i * 34.f, y}, 11, {50, 73, 76, 255});
      DrawCircleV({x - 58 + i * 34.f, y}, 7, {93, 161, 170, 255});
    }
  }
  // Keep controls readable while retaining the landscape on the right.
  DrawRectangleGradientH(0, 0, static_cast<int>(w), static_cast<int>(h),
                         {12, 22, 21, 245}, {12, 22, 21, 35});
}
}  // namespace
std::optional<std::filesystem::path> MainMenu(MenuOptions& options) {
  Renderer renderer;
  const int scene = MenuScene();
  auto slots = ScanSaveSlots(options.directory);
  int selected = 0;
  for (int i = 0; i < static_cast<int>(slots.size()); i++) {
    if (slots[i].path == options.preferred) {
      selected = i;
    }
  }
  enum class Page { kHome, kSaves, kNew, kSettings, kRecovery };
  Page page = options.preview == "newgame"    ? Page::kNew
              : options.preview == "saves"    ? Page::kSaves
              : options.preview == "settings" ? Page::kSettings
                                              : Page::kHome;
  int home_choice = 0, frames = 0;
  bool confirm_delete = false, edit_seed = false;
  bool expanded_summary = false;
  SummaryView summary;
  int background_choice = 0;
  for (int i = 0; i < static_cast<int>(GetContent().characters.size()); i++) {
    if (GetContent().characters[i].id == "traveler") {
      background_choice = i;
    }
  }
  SettingsPanel settings_panel;
  settings_panel.draft = options.settings;
  std::vector<SaveSlot> recovered;
  int recovered_choice = 0;
  std::string operation, operation_text, operation_name;
  bool operation_name_focus = false;
  bool field_selected = false;
  std::filesystem::path operation_source;
  std::string world_name = "New world", seed_text, notice;
  double notice_until = 0;
  auto notify = [&](const std::string& s) {
    notice = s;
    notice_until = GetTime() + 5;
  };
  auto refresh = [&] {
    slots = ScanSaveSlots(options.directory);
    selected = std::clamp(selected, 0,
                          std::max(0, static_cast<int>(slots.size()) - 1));
    summary.scroll = summary.horizontal = 0;
  };
  auto valid = [&] {
    return !slots.empty() && slots[selected].save.has_value();
  };
  SetTargetFPS(options.settings.fps);
  while (!WindowShouldClose()) {
    float w = static_cast<float>(GetScreenWidth()),
          h = static_cast<float>(GetScreenHeight());
    if (MenuBack() && page != Page::kSettings) {
      if (expanded_summary) {
        expanded_summary = false;
      } else if (!operation.empty()) {
        operation.clear();
      } else if (confirm_delete) {
        confirm_delete = false;
      } else if (page != Page::kHome) {
        page = Page::kHome;
      }
    }
    bool operation_opened_this_frame = false;
    const auto frame_page = page;
    BeginDrawing();
    ClearBackground(kBackground);
    Landscape(scene, w, h);
    const float top = 120, left = 40,
                list_width = std::clamp(w * .25f, 230.f, 330.f),
                right = left + list_width + 24, detail_width = w - right - 40;
    if (frame_page == Page::kHome) {
      const std::vector<std::string> actions = {"Continue", "Saved worlds",
                                                "New game", "Settings", "Quit"};
      if (MenuUp()) {
        home_choice = (home_choice + 4) % 5;
      }
      if (MenuDown()) {
        home_choice = (home_choice + 1) % 5;
      }
      for (int i = 0; i < 5; i++) {
        bool active = i != 0 || valid();
        if (Button(renderer, {left, top + i * 62, 280, 50}, actions[i], active,
                   home_choice == i) ||
            (active && home_choice == i && MenuConfirm())) {
          if (i == 0) {
            EndDrawing();
            return slots[selected].path;
          }
          if (i == 1) {
            page = Page::kSaves;
          }
          if (i == 2) {
            page = Page::kNew;
          }
          if (i == 3) {
            settings_panel = {};
            settings_panel.draft = options.settings;
            page = Page::kSettings;
          }
          if (i == 4) {
            EndDrawing();
            return std::nullopt;
          }
        }
      }
      if (valid()) {
        auto& slot = slots[selected];
        auto& save = *slot.save;
        ConfigureWorld(save.seed);
        renderer.Text("Last selected world", right, top, 20, kMuted);
        renderer.Text(slot.path.stem().string(), right, top + 34, 28, kInk);
        renderer.Text(std::to_string(slot.discovered) + " known towns · " +
                          std::to_string(slot.owned) + " owned · " +
                          std::to_string(save.skills.size()) + " skills",
                      right, top + 77, 18, kGold);
        float size = std::min(detail_width, h - top - 185);
        renderer.DrawSavedMap(save, {right, top + 115, size, size});
      } else {
        renderer.Text("Your journey starts here.", right, top + 30, 28, kInk);
        renderer.Wrapped(
            "Create a new world. Start with one town, no equipment and "
            "everything to discover.",
            right, top + 80, detail_width, 20, kMuted, 4);
      }
    }
    if (frame_page == Page::kSaves) {
      renderer.Text("Saved worlds", left, top, 24, kInk);
      int visible = std::max(1, static_cast<int>((h - top - 115) / 58));
      if (!confirm_delete && operation.empty() && !expanded_summary) {
        if (MenuUp() && !slots.empty()) {
          selected = (selected + static_cast<int>(slots.size()) - 1) %
                     static_cast<int>(slots.size());
          summary.scroll = summary.horizontal = 0;
        }
        if (MenuDown() && !slots.empty()) {
          selected = (selected + 1) % static_cast<int>(slots.size());
          summary.scroll = summary.horizontal = 0;
        }
        auto mouse = GetMousePosition();
        if (mouse.x < right && !slots.empty()) {
          selected =
              std::clamp(selected - static_cast<int>(GetMouseWheelMove()), 0,
                         static_cast<int>(slots.size()) - 1);
        }
      }

      int first = selected / visible * visible;
      for (int i = first;
           i < std::min(first + visible, static_cast<int>(slots.size())); i++) {
        if (Button(renderer,
                   {left, top + 43 + (i - first) * 58, list_width, 48},
                   slots[i].path.stem().string() +
                       (slots[i].save ? "" : " [invalid]"),
                   !confirm_delete && operation.empty() && !expanded_summary,
                   selected == i)) {
          selected = i;
          summary.scroll = summary.horizontal = 0;
        }
      }
      if (slots.empty()) {
        renderer.Wrapped("No saved worlds. Create a new game to get started.",
                         left, top + 50, list_width, 18, kMuted, 4);
      }
      if (Button(renderer, {left, h - 76, list_width, 44}, "New game",
                 !confirm_delete && operation.empty() && !expanded_summary)) {
        page = Page::kNew;
      }
      if (!slots.empty()) {
        auto& slot = slots[selected];
        DrawRectangleRounded({right, top, detail_width, h - top - 90}, .03f, 6,
                             kPanel);
        renderer.Text(slot.path.stem().string(), right + 20, top + 17, 26,
                      kGold);
        if (valid() && operation.empty() && !confirm_delete &&
            (Button(renderer, {right + detail_width - 122, top + 15, 110, 34},
                    "Expand · F2", !expanded_summary) ||
             IsKeyPressed(KEY_F2))) {
          expanded_summary = !expanded_summary;
        }
        if (!slot.save) {
          renderer.Wrapped("Cannot load this world: " + slot.error, right + 20,
                           top + 67, detail_width - 40, 18, kInk, 5);
        } else {
          auto& save = *slot.save;
          ConfigureWorld(save.seed);
          summary.caption = "Updated: " + UpdatedAt(slot.modified);
          Rectangle summary_box{right + 20, top + 57, detail_width - 40,
                                h - top - 215};
          if (!confirm_delete && operation.empty() && !expanded_summary) {
            renderer.SummaryInput(summary, summary_box, false);
          }
          summary.keyboard = false;
          renderer.DrawSummary(
              save, summary, summary_box,
              !confirm_delete && operation.empty() && !expanded_summary);
        }
        if (Button(renderer, {right, h - 76, 170, 44}, "Load world",
                   valid() && !confirm_delete && operation.empty() &&
                       !expanded_summary) ||
            (!confirm_delete && operation.empty() && !expanded_summary &&
             valid() && MenuConfirm())) {
          EndDrawing();
          return slot.path;
        }
        if (Button(renderer, {right + 184, h - 76, 180, 44}, "Delete · DEL",
                   !confirm_delete && operation.empty() && !expanded_summary) ||
            (!confirm_delete && operation.empty() && !expanded_summary &&
             IsKeyPressed(KEY_DELETE))) {
          confirm_delete = true;
        }
      }
    }
    if (frame_page == Page::kNew) {
      float available = std::min(w - 80, 1150.f);
      DrawRectangleRounded({left, top, available, h - top - 85}, .03f, 6,
                           kPanel);
      renderer.Text("Choose your background", left + 20, top + 12, 26, kGold);
      renderer.Text("LEFT / RIGHT · " + std::to_string(background_choice + 1) +
                        " / " + std::to_string(GetContent().characters.size()),
                    left + available - 285, top + 19, 16, kMuted);
      float field_width = (available - 60) * .5f;
      renderer.Text("World name", left + 20, top + 54, 16, kMuted);
      renderer.Text("Seed · empty = random", left + 40 + field_width, top + 54,
                    16, kMuted);
      Rectangle name_box{left + 20, top + 78, field_width, 40},
          seed_box{left + 40 + field_width, top + 78, field_width, 40};
      if (CheckCollisionPointRec(GetMousePosition(), name_box) &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        edit_seed = false;
      }
      if (CheckCollisionPointRec(GetMousePosition(), seed_box) &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        edit_seed = true;
      }
      DrawRectangleRounded(name_box, .1f, 4, {40, 55, 45, 255});
      DrawRectangleRounded(seed_box, .1f, 4, {40, 55, 45, 255});
      renderer.Text(world_name + (!edit_seed ? "|" : ""), name_box.x + 10,
                    name_box.y + 7, 18, kInk);
      renderer.Text(seed_text + (edit_seed ? "|" : ""), seed_box.x + 10,
                    seed_box.y + 7, 18, kInk);
      if (IsKeyPressed(KEY_TAB)) {
        edit_seed = !edit_seed;
      }
      auto& input = edit_seed ? seed_text : world_name;
      EditField(input, field_selected, edit_seed ? 20u : 48u, edit_seed);
      int count = static_cast<int>(GetContent().characters.size());
      if (MenuLeft()) {
        background_choice = (background_choice + count - 1) % count;
      }
      if (MenuRight()) {
        background_choice = (background_choice + 1) % count;
      }
      int visible_cards = std::min(
          count, std::max(1, static_cast<int>((available - 40) / 215)));
      int first_card = background_choice / visible_cards * visible_cards;
      float card_width = (available - 40) / visible_cards;
      for (int i = first_card; i < std::min(first_card + visible_cards, count);
           i++) {
        auto& hero = GetContent().characters[i];
        Rectangle card{left + 20 + (i - first_card) * card_width, top + 134,
                       card_width - 8, 260};
        if (Button(renderer, card, "", true, background_choice == i)) {
          background_choice = i;
        }
        float cx = card.x + card.width * .5f;
        Color coat =
            i % 2 ? Color{107, 154, 124, 255} : Color{179, 141, 95, 255};
        DrawRectangle(static_cast<int>(cx - 10), static_cast<int>(card.y + 34),
                      20, 25, coat);
        DrawCircleV({cx, card.y + 23}, 8, {207, 170, 129, 255});
        DrawRectangle(static_cast<int>(cx - 9), static_cast<int>(card.y + 59),
                      6, 12, {74, 82, 71, 255});
        DrawRectangle(static_cast<int>(cx + 3), static_cast<int>(card.y + 59),
                      6, 12, {74, 82, 71, 255});
        renderer.Wrapped(hero.name, card.x + 10, card.y + 78, card.width - 20,
                         19, kGold, 1);
        renderer.Wrapped("+ " + hero.pros, card.x + 10, card.y + 108,
                         card.width - 20, 16, kInk, 3);
        renderer.Wrapped("- " + hero.cons, card.x + 10, card.y + 198,
                         card.width - 20, 16, kMuted, 2);
      }
      renderer.Wrapped(GetContent().characters[background_choice].lore,
                       left + 20, top + 405, available - 40, 17, kInk,
                       h < 760 ? 1 : 2);
      if (Button(renderer, {left + 20, h - 76, 240, 44}, "Create and play") ||
          MenuConfirm()) {
        try {
          uint64_t seed =
              seed_text.empty()
                  ? (static_cast<uint64_t>(std::random_device{}()) << 32) ^
                        std::random_device{}()
                  : std::stoull(seed_text);
          auto path =
              CreateSaveSlot(options.directory, world_name, seed,
                             GetContent().characters[background_choice].id);
          EndDrawing();
          return path;
        } catch (const std::exception& e) {
          notify(e.what());
        }
      }
    }
    if (frame_page == Page::kSettings) {
      if (DrawSettingsPanel(renderer, settings_panel, options.settings,
                            options.settings_path)) {
        page = Page::kHome;
      }
    }
    if (frame_page == Page::kSaves) {
      const std::vector<std::string> commands = {"Rename", "Copy", "Export",
                                                 "Import", "Recover"};
      float width = (w - 80) / commands.size();
      for (int i = 0; i < static_cast<int>(commands.size()); ++i) {
        if (Button(renderer, {40 + i * width, h - 127, width - 8, 40},
                   commands[i] + " · F" + std::to_string(6 + i),
                   operation.empty() && !expanded_summary && !confirm_delete &&
                       (i >= 3 || valid())) ||
            (operation.empty() && !expanded_summary && !confirm_delete &&
             (i >= 3 || valid()) && IsKeyPressed(KEY_F6 + i))) {
          if (i == 4) {
            recovered = ScanRecoveredSlots(options.directory);
            recovered_choice = 0;
            page = Page::kRecovery;
          } else {
            operation = commands[i];
            operation_name.clear();
            operation_name_focus = false;
            field_selected = false;
            operation_source =
                i < 3 ? slots[selected].path : std::filesystem::path{};
            operation_text =
                i == 0   ? operation_source.stem().string()
                : i == 1 ? operation_source.stem().string() + " copy"
                : i == 2 ? (options.directory / "exports" /
                            (operation_source.stem().string() + "-" +
                             std::to_string(std::chrono::system_clock::now()
                                                .time_since_epoch()
                                                .count()) +
                             ".json"))
                               .string()
                         : "";
          }
        }
      }
    }
    if (frame_page == Page::kRecovery) {
      renderer.Text("Recover deleted worlds and backups", 40, 120, 26, kGold);
      int shown = std::max(1, static_cast<int>((h - 240) / 60));
      if (operation.empty() && !recovered.empty()) {
        if (MenuUp()) {
          recovered_choice =
              (recovered_choice + static_cast<int>(recovered.size()) - 1) %
              static_cast<int>(recovered.size());
        }
        if (MenuDown()) {
          recovered_choice =
              (recovered_choice + 1) % static_cast<int>(recovered.size());
        }
      }
      int first = recovered_choice / shown * shown;
      for (int i = first;
           i < std::min(first + shown, static_cast<int>(recovered.size()));
           ++i) {
        if (Button(renderer, {40, 175 + (i - first) * 60.f, w - 80, 50},
                   recovered[i].path.filename().string() +
                       (recovered[i].save ? "" : " [invalid]"),
                   operation.empty(), i == recovered_choice)) {
          recovered_choice = i;
        }
      }
      if (recovered.empty()) {
        renderer.Text("No deleted worlds or migration backups.", 40, 185, 20,
                      kMuted);
      }
      if (!recovered.empty() && recovered[recovered_choice].save &&
          operation.empty() &&
          (Button(renderer, {40, h - 76, 320, 44}, "Restore as a new world") ||
           MenuConfirm())) {
        operation = "Restore";
        operation_opened_this_frame = true;
        operation_source = recovered[recovered_choice].path;
        operation_text = "Restored world";
        operation_name.clear();
        operation_name_focus = false;
      }
    }
    renderer.Text("FOREST FRONTIER", 40, 30, 36, kGold);
    if (page != Page::kHome &&
        Button(renderer, {w - 155, 32, 115, 44}, "Back",
               !confirm_delete && operation.empty() && !expanded_summary)) {
      page = Page::kHome;
    }
    if (confirm_delete && !slots.empty()) {
      DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h),
                    {0, 0, 0, 185});
      Rectangle box{(w - 560) * .5f, (h - 230) * .5f, 560, 230};
      DrawRectangleRounded(box, .06f, 8, kPanel);
      renderer.Text("Delete this saved world?", box.x + 24, box.y + 24, 24,
                    kGold);
      renderer.Wrapped(
          slots[selected].path.stem().string() +
              " will be removed from your saved worlds. You can recover "
              "it using Recover in Saved worlds.",
          box.x + 24, box.y + 71, 512, 18, kInk, 3);
      if (Button(renderer, {box.x + 24, box.y + 166, 230, 44},
                 "Confirm · ENTER") ||
          MenuConfirm()) {
        try {
          DeleteSaveSlot(options.directory, slots[selected].path);
          confirm_delete = false;
          refresh();
          notify("World deleted");
        } catch (const std::exception& e) {
          confirm_delete = false;
          notify(e.what());
        }
      }
      if (Button(renderer, {box.x + 278, box.y + 166, 230, 44}, "Cancel")) {
        confirm_delete = false;
      }
    }
    if (expanded_summary && valid()) {
      ConfigureWorld(slots[selected].save->seed);
      DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h),
                    {0, 0, 0, 195});
      DrawRectangleRounded({24, 24, w - 48, h - 48}, .04f, 12, kPanel);
      renderer.Text("Saved world: " + slots[selected].path.stem().string() +
                        " · read-only",
                    44, 40, 24, kGold);
      renderer.SummaryInput(summary, Renderer::ProgressSummaryBounds());
      renderer.DrawSummary(*slots[selected].save, summary,
                           Renderer::ProgressSummaryBounds());
      if (Button(renderer, Renderer::ProgressCloseButton(), "Close · Esc")) {
        expanded_summary = false;
      }
    }
    if (!operation.empty()) {
      DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h),
                    {0, 0, 0, 200});
      Rectangle dialog{(w - std::min(760.f, w - 60)) * .5f, (h - 320) * .5f,
                       std::min(760.f, w - 60), 320};
      DrawRectangleRounded(dialog, .06f, 10, kPanel);
      renderer.Text(operation + " world", dialog.x + 20, dialog.y + 20, 26,
                    kGold);
      bool importing = operation == "Import";
      renderer.Wrapped(operation == "Export" || importing ? "File path (.json)"
                                                          : "World name",
                       dialog.x + 20, dialog.y + 64, dialog.width - 40, 18,
                       kInk, 1);
      Rectangle field{dialog.x + 20, dialog.y + 96, dialog.width - 40, 42};
      DrawRectangleRounded(field, .2f, 8, {39, 60, 45, 255});
      renderer.Wrapped(operation_text + (!operation_name_focus ? "|" : ""),
                       field.x + 10, field.y + 8, field.width - 20, 17, kInk,
                       1);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(GetMousePosition(), field)) {
        operation_name_focus = false;
      }
      if (importing) {
        Rectangle name{dialog.x + 20, dialog.y + 153, dialog.width - 40, 42};
        DrawRectangleRounded(name, .2f, 8, {39, 60, 45, 255});
        renderer.Wrapped("Name (optional): " + operation_name +
                             (operation_name_focus ? "|" : ""),
                         name.x + 10, name.y + 8, name.width - 20, 17, kInk, 1);
        if (IsKeyPressed(KEY_TAB) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
             CheckCollisionPointRec(GetMousePosition(), name))) {
          operation_name_focus = !operation_name_focus;
        }
      }
      auto& text = operation_name_focus ? operation_name : operation_text;
      EditField(text, field_selected, 512);
      renderer.Wrapped(
          operation == "Restore"
              ? "Restores a separate slot. Existing worlds and the recovery "
                "file remain available."
              : "Existing files are never overwritten. Esc cancels.",
          dialog.x + 20, dialog.y + 210, dialog.width - 40, 16, kMuted, 2);
      if (Button(renderer,
                 {dialog.x + 20, dialog.y + 268, dialog.width * .5f - 30, 40},
                 "Confirm · Enter") ||
          (!operation_opened_this_frame && MenuConfirm())) {
        try {
          std::filesystem::path result;
          if (operation == "Rename") {
            result = RenameSaveSlot(options.directory, operation_source,
                                    operation_text);
          }
          if (operation == "Copy") {
            result = CopySaveSlot(options.directory, operation_source,
                                  operation_text);
          }
          if (operation == "Import") {
            result = ImportSaveSlot(options.directory, operation_text,
                                    operation_name);
          }
          if (operation == "Restore") {
            result = RestoreSaveSlot(options.directory, operation_source,
                                     operation_text);
          }
          if (operation == "Export") {
            ExportSaveSlot(options.directory, operation_source, operation_text);
          }
          if (!result.empty()) {
            options.preferred = result;
            options.settings.last_save =
                std::filesystem::absolute(result).string();
            SetActiveSettings(options.settings);
            try {
              WriteSettings(options.settings_path, options.settings);
            } catch (const std::exception& error) {
              // The world operation succeeded; do not offer a duplicate retry.
              operation += " (preferences could not be saved: " +
                           std::string(error.what()) + ")";
            }
          }
          notify(operation + " completed" +
                 (result.empty() ? "" : " · " + result.filename().string()));
          operation.clear();
          refresh();
          page = Page::kSaves;
          for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
            if (slots[i].path == result) {
              selected = i;
            }
          }
        } catch (const std::exception& error) {
          notify(error.what());
        }
      }
      if (Button(renderer,
                 {dialog.x + dialog.width * .5f + 10, dialog.y + 268,
                  dialog.width * .5f - 30, 40},
                 "Cancel · Esc")) {
        operation.clear();
      }
    }
    if (!notice.empty() && GetTime() < notice_until) {
      DrawRectangle(30, static_cast<int>(h - 32), static_cast<int>(w - 60), 27,
                    kPanel);
      renderer.Wrapped(notice, 40, h - 29, w - 80, 16, kGold, 1);
    }
    EndDrawing();
    frames++;
    if (options.frames > 0 && frames >= options.frames) {
      Capture(options.screenshot);
      return std::nullopt;
    }
  }
  return std::nullopt;
}
}  // namespace frontier
