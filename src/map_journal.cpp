#include <algorithm>
#include <exception>
#include <string>

#include "atlas_notes.hpp"
#include "input.hpp"
#include "render.hpp"
namespace frontier {
namespace {
std::string Utf8(int code) {
  if (code < 32 || code > 0x45f) {
    return {};
  }
  if (code < 128) {
    return std::string(1, static_cast<char>(code));
  }
  return {static_cast<char>(0xc0 | (code >> 6)),
          static_cast<char>(0x80 | (code & 63))};
}
void EraseCharacter(std::string& value) {
  if (value.empty()) {
    return;
  }
  size_t i = value.size() - 1;
  while (i > 0 && (static_cast<unsigned char>(value[i]) & 0xc0) == 0x80) {
    --i;
  }
  value.resize(i);
}
}  // namespace
void Renderer::DrawMapJournal(Save& save, Ui& ui) {
  float w = GetScreenWidth(), h = GetScreenHeight(),
        side = std::min(w - 120, h - 160);
  Rectangle map{(w - side) * .5f, (h - side) * .5f, side, side};
  auto button = [&](Rectangle r, const std::string& text,
                    bool selected = false) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(
        r, .4f, 8, selected ? Color{83, 107, 63, 255} : Color{36, 57, 43, 255});
    Wrapped(text, r.x + 8, r.y + 6, r.width - 16, 16, {231, 231, 204, 255}, 1);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  };
  auto open = [&](int id, Vec point, bool underwater) {
    ui.map_journal = true;
    ui.note_edit = true;
    ui.note_id = id;
    ui.note_position = point;
    ui.note_underwater = underwater;
    ui.note_body_focus = false;
    ui.note_select_all = false;
    ui.note_error.clear();
    ui.note_scroll = 0;
    ui.note_title = id < 0 ? "New note" : save.map_notes[id].title;
    ui.note_text = id < 0 ? "" : save.map_notes[id].text;
  };
  if (!ui.map_journal) {
    if (button({map.x + map.width - 190, map.y - 46, 190, 34},
               "Journal · Tab") ||
        IsKeyPressed(KEY_TAB)) {
      ui.map_journal = true;
      ui.note_edit = false;
      return;
    }
    for (int i = 0; i < 3; i++) {
      if (button({map.x + i * 132, map.y + map.height + 44, 126, 34},
                 std::string(i == 0   ? "Terrain"
                             : i == 1 ? "Politics"
                                      : "Economy") +
                     " · " + std::to_string(i + 1),
                 ui.map_overlay == i) ||
          IsKeyPressed(KEY_ONE + i)) {
        ui.map_overlay = i;
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(GetMousePosition(), map)) {
      const auto& cache = maps_[1];
      auto mouse = GetMousePosition();
      Vec point{
          cache.center.x + (mouse.x - map.x) / map.width * cache.range * 2 -
              cache.range,
          cache.center.y + (mouse.y - map.y) / map.height * cache.range * 2 -
              cache.range};
      bool layer = save.submerged;
      int selected = -1;
      for (size_t i = 0; i < save.map_notes.size(); i++) {
        if (save.map_notes[i].underwater == layer &&
            Length(save.map_notes[i].position - point) <
                cache.range * 2 / map.width * 14) {
          selected = i;
        }
      }
      if (selected >= 0) {
        open(selected, save.map_notes[selected].position, layer);
      } else if (ExploredAt(layer ? save.explored_underwater : save.explored,
                            point)) {
        open(-1, point, layer);
      }
    }
    return;
  }
  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 195});
  Rectangle box{40, 70, w - 80, h - 130};
  DrawRectangleRounded(box, .04f, 12, {23, 37, 29, 255});
  Text(ui.note_edit ? "Map journal · edit note" : "Map journal", box.x + 20,
       box.y + 18, 24, {234, 196, 125, 255});
  if (button({w - 230, box.y + 16, 160, 36}, "Close · Esc")) {
    ui.map_journal = false;
    ui.note_edit = false;
    return;
  }
  if (ui.note_edit) {
    Text(std::string(ui.note_underwater ? "Seafloor" : "Surface") + " · " +
             std::to_string(static_cast<int>(ui.note_position.x)) + ", " +
             std::to_string(static_cast<int>(ui.note_position.y)),
         box.x + 20, box.y + 52, 16, {163, 181, 146, 255});
  }
  if (!ui.note_edit) {
    int rows = std::max(1, static_cast<int>((box.height - 150) / 64));
    int pages = std::max(
        1, (static_cast<int>(save.map_notes.size()) + rows - 1) / rows);
    ui.note_page = std::clamp(ui.note_page, 0, pages - 1);
    for (int i = ui.note_page * rows;
         i < std::min((ui.note_page + 1) * rows,
                      static_cast<int>(save.map_notes.size()));
         i++) {
      const auto& note = save.map_notes[i];
      float y = box.y + 78 + (i - ui.note_page * rows) * 64;
      if (button({box.x + 20, y, box.width - 40, 54},
                 note.title + " · " +
                     (note.underwater ? "Seafloor" : "Surface") + " · " +
                     std::to_string(static_cast<int>(note.position.x)) + ", " +
                     std::to_string(static_cast<int>(note.position.y)))) {
        open(i, note.position, note.underwater);
      }
    }
    if (save.map_notes.empty()) {
      Wrapped("Click an explored location on the map to write your first note.",
              box.x + 20, box.y + 90, box.width - 40, 18, {218, 226, 207, 255},
              2);
    }
    if (button({box.x + 20, box.y + box.height - 54, 140, 36}, "Previous") ||
        IsKeyPressed(KEY_PAGE_UP)) {
      ui.note_page = std::max(0, ui.note_page - 1);
    }
    if (button({box.x + 170, box.y + box.height - 54, 140, 36}, "Next") ||
        IsKeyPressed(KEY_PAGE_DOWN)) {
      ui.note_page = std::min(pages - 1, ui.note_page + 1);
    }
    return;
  }
  Rectangle title{box.x + 20, box.y + 80, box.width - 40, 48},
      body{box.x + 20, box.y + 144, box.width - 40, box.height - 244};
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (CheckCollisionPointRec(GetMousePosition(), title)) {
      ui.note_body_focus = false;
    }
    if (CheckCollisionPointRec(GetMousePosition(), body)) {
      ui.note_body_focus = true;
    }
  }
  if (IsKeyPressed(KEY_TAB)) {
    ui.note_body_focus = !ui.note_body_focus;
    ui.note_select_all = false;
  }
  auto& field = ui.note_body_focus ? ui.note_text : ui.note_title;
  size_t cap = ui.note_body_focus ? 2048 : 64;
  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (ctrl && IsKeyPressed(KEY_A)) {
    ui.note_select_all = true;
  }
  if (ctrl && IsKeyPressed(KEY_C)) {
    SetClipboardText(field.c_str());
  }
  auto append = [&](std::string text) {
    if (ui.note_select_all) {
      field.clear();
    }
    ui.note_select_all = false;
    if (field.size() + text.size() <= cap) {
      field += text;
    }
  };
  if (ctrl && IsKeyPressed(KEY_V)) {
    const char* text = GetClipboardText();
    if (text) {
      std::string value = text;
      while (value.size() > cap) {
        EraseCharacter(value);
      }
      append(value);
    }
  }
  if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
    if (ui.note_select_all) {
      field.clear();
    } else {
      EraseCharacter(field);
    }
    ui.note_select_all = false;
  }
  if (!ctrl) {
    for (int code = GetCharPressed(); code; code = GetCharPressed()) {
      append(Utf8(code));
    }
  }
  if (ui.note_body_focus && !ctrl && IsKeyPressed(KEY_ENTER)) {
    append("\n");
  }
  for (auto r : {title, body}) {
    DrawRectangleRounded(r, r.height > 100 ? .025f : .12f, 8,
                         {34, 54, 40, 255});
  }
  DrawRectangleRoundedLinesEx(ui.note_body_focus ? body : title,
                              ui.note_body_focus ? .025f : .12f, 8, 2,
                              {207, 177, 108, 255});
  Wrapped(ui.note_title, title.x + 10, title.y + 9, title.width - 20, 18,
          {239, 226, 194, 255}, 1);
  int estimated_lines =
      static_cast<int>(ui.note_text.size() /
                       std::max(1.f, body.width / (TextSize(17) * .6f))) +
      static_cast<int>(
          std::count(ui.note_text.begin(), ui.note_text.end(), '\n')) +
      2;
  ui.note_scroll = std::clamp(
      ui.note_scroll - GetMouseWheelMove() * 45, 0.f,
      std::max(0.f, estimated_lines * (TextSize(17) + 6) - body.height));
  BeginScissorMode(static_cast<int>(body.x), static_cast<int>(body.y),
                   static_cast<int>(body.width), static_cast<int>(body.height));
  Wrapped(ui.note_text, body.x + 10, body.y + 10 - ui.note_scroll,
          body.width - 20, 17, {219, 228, 206, 255}, 128);
  EndScissorMode();
  float bottom = box.y + box.height - 78;
  if (button({box.x + 20, bottom, 220, 36}, "Save · Ctrl+Enter") ||
      (ctrl && IsKeyPressed(KEY_ENTER))) {
    try {
      ui.note_id =
          StoreMapNote(save, ui.note_id, ui.note_position, ui.note_underwater,
                       ui.note_title, ui.note_text);
      ui.note_edit = false;
      ui.note_error = "Note saved";
    } catch (const std::exception& error) {
      ui.note_error = error.what();
    }
  }
  if (ui.note_id >= 0 &&
      button({box.x + 250, bottom, 160, 36}, "Delete note")) {
    DeleteMapNote(save, ui.note_id);
    ui.note_edit = false;
    ui.note_error = "Note deleted";
  }
  Wrapped(
      ui.note_error.empty()
          ? "Tab: title/text · Ctrl+A/C/V: select/copy/paste · Enter: new line"
          : ui.note_error,
      box.x + 20, box.y + box.height - 32, box.width - 40, 16,
      {213, 190, 137, 255}, 1);
}
}  // namespace frontier
