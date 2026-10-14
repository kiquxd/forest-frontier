#include "render.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "adventure.hpp"
#include "input.hpp"
#include "json.hpp"
#include "settlement_system.hpp"
#include "vessels.hpp"

namespace frontier {
static constexpr Color kInk{222, 231, 213, 255};
static constexpr Color kMuted{151, 174, 150, 255};
static constexpr Color kGold{231, 189, 115, 255};
static constexpr Color kPanel{20, 31, 29, 238};
namespace {
Vector2 ToVector2(Vec position) {
  return {position.x, position.y};
}
}  // namespace

Renderer::Renderer() {
  // One texture atlas means grass, trees, people and huts share raylib's draw
  // batch.
  const int outfit_start = 64 + static_cast<int>(GetContent().npcs.size()) * 5;
  const int building_start =
      outfit_start + static_cast<int>(GetContent().outfits.size()) * 5;
  int count = building_start + 12;
  sprites_.resize(count);
  Image atlas = GenImageColor(1024, ((count + 7) / 8) * 160, BLANK);
  for (int index = 0; index < count; index++) {
    const int ox = (index % 8) * 128, oy = (index / 8) * 160;
    sprites_[index] = {static_cast<float>(ox), static_cast<float>(oy), 128,
                       160};
    int base = index, frame = 0;
    if (index >= 28 && index < 44) {
      base = 13 + (index - 28) / 4;
      frame = (index - 28) % 4 + 1;
    }
    if (index >= 44 && index < 48) {
      base = 17;
      frame = (index - 44) % 4 + 1;
    }
    if (index >= 48 && index < 52) {
      base = 25;
      frame = (index - 48) % 4 + 1;
    }
    int npc = index >= 64 && index < outfit_start ? (index - 64) / 5 : -1;
    int clothing = index >= outfit_start && index < building_start
                       ? (index - outfit_start) / 5
                       : -1;
    int building = index >= building_start ? index - building_start : -1;
    if (building >= 0) {
      base = 18 + building % 2;
    }
    if (clothing >= 0) {
      base = 16;
      frame = (index - outfit_start) % 5;
    }
    if (npc >= 0) {
      base = 13;
      frame = (index - 64) % 5;
    }
    int stride = frame ? std::array<int, 4>{-3, 0, 3, 0}[(frame - 1) % 4] : 0;
    int bob = frame ? (frame % 2) : 0;
    auto rect = [&](int x, int y, int w, int h, Color color) {
      ImageDrawRectangle(&atlas, ox + x, oy + y, w, h, color);
    };
    auto circle = [&](int x, int y, int r, Color color) {
      ImageDrawCircle(&atlas, ox + x, oy + y, r, color);
    };
    auto line = [&](int x, int y, int xx, int yy, Color color) {
      ImageDrawLine(&atlas, ox + x, oy + y, ox + xx, oy + yy, color);
    };
    auto tri = [&](int x, int y, int xx, int yy, int xxx, int yyy,
                   Color color) {
      ImageDrawTriangle(
          &atlas, {static_cast<float>(ox + x), static_cast<float>(oy + y)},
          {static_cast<float>(ox + xx), static_cast<float>(oy + yy)},
          {static_cast<float>(ox + xxx), static_cast<float>(oy + yyy)}, color);
    };
    if (base < 8 || (base >= 20 && base <= 22) || base == 52) {
      Color c =
          base < 6 ? Color{static_cast<uint8_t>(65 + base * 2),
                           static_cast<uint8_t>(91 + base * 3),
                           static_cast<uint8_t>(58 + base * 2), 255}
                   : Color{static_cast<uint8_t>(123 + (base - 6) * 5),
                           static_cast<uint8_t>(111 + (base - 6) * 4), 80, 255};
      if (base == 20) {
        c = {179, 162, 112, 255};
      }
      if (base == 21) {
        c = {69, 137, 143, 255};
      }
      if (base == 22) {
        c = {43, 97, 126, 255};
      }
      if (base == 52) {
        c = {63, 105, 113, 255};
      }
      tri(22, 120, 64, 96, 106, 120, c);
      tri(22, 120, 106, 120, 64, 144, c);
      for (int i = 0; i < 36; i++) {
        uint64_t h = Hash(index * 200 + i);
        int x = 32 + h % 64, y = 102 + (h >> 16) % 35;
        if (std::abs(x - 64) / 42.f + std::abs(y - 120) / 24.f > .8f) {
          continue;
        }
        if (base < 6) {
          line(x, y, x + 2, y - 3, Color{91, 119, 73, 150});
          line(x + 2, y - 3, x + 4, y - 1, Color{91, 119, 73, 150});
        } else if (base <= 20) {
          rect(x, y, 2, 1, Color{150, 137, 102, 170});
        } else if (i % 5 == 0) {
          line(x, y, x + 9, y, {116, 179, 183, 125});
        }
      }
    } else if (base <= 10) {
      tri(29, 143, 91, 143, 69, 155, {18, 32, 25, 75});
      rect(59, 111, 10, 34, {88, 65, 45, 255});
      rect(59, 111, 3, 34, {116, 83, 52, 255});
      int shift = (index - 8) * 5;
      tri(22, 125, 64, 41 - shift, 106, 125, {37, 67, 46, 255});
      tri(30, 99, 64, 24 - shift, 98, 99, {44, 81, 51, 255});
      tri(40, 72, 64, 9, 88, 72, {54, 94, 58, 255});
      tri(40, 72, 64, 9, 61, 72, {68, 110, 68, 255});
      line(45, 103, 78, 103, {73, 109, 63, 255});
      line(51, 79, 69, 79, {79, 118, 72, 255});
      for (int branch = 0; branch < 12; branch++) {
        int px = 42 + (branch * 13) % 38, py = 64 + (branch * 17) % 61;
        line(px, py, px + 9, py + 3, {65, 100, 56, 255});
      }
      for (int bark = 0; bark < 6; bark++) {
        line(61 + bark % 3, 118 + bark * 4, 64 + bark % 3, 122 + bark * 4,
             {63, 51, 40, 255});
      }
    } else if (base == 11) {
      tri(41, 146, 85, 146, 73, 154, {18, 32, 25, 65});
      tri(42, 144, 45, 125, 71, 117, {127, 138, 131, 255});
      tri(42, 144, 71, 117, 89, 140, {108, 120, 115, 255});
      tri(42, 144, 89, 140, 70, 148, {81, 96, 91, 255});
      line(47, 125, 69, 119, {165, 174, 157, 255});
    } else if (base == 12) {
      circle(53, 135, 13, {44, 76, 48, 255});
      circle(70, 133, 16, {49, 90, 53, 255});
      circle(62, 123, 12, {59, 103, 57, 255});
      for (int i = 0; i < 7; i++) {
        circle(48 + (i * 13) % 34, 121 + (i * 7) % 21, 2, {187, 75, 87, 255});
      }
    } else if (base <= 16) {
      Color coat = base == 16   ? Color{216, 176, 98, 255}
                   : base == 13 ? Color{115, 151, 137, 255}
                   : base == 14 ? Color{171, 110, 89, 255}
                                : Color{143, 135, 171, 255};
      if (npc >= 0) {
        auto color = GetContent().npcs[npc].color;
        coat = {static_cast<uint8_t>(color[0]), static_cast<uint8_t>(color[1]),
                static_cast<uint8_t>(color[2]), 255};
      }
      if (clothing >= 0) {
        auto color = GetContent().outfits[clothing].color;
        coat = {static_cast<uint8_t>(color[0]), static_cast<uint8_t>(color[1]),
                static_cast<uint8_t>(color[2]), 255};
      }
      rect(53, 141, 24, 4, {13, 26, 25, 100});
      rect(57 - stride, 127, 5, 16 - stride, {52, 56, 54, 255});
      rect(68 + stride, 127, 5, 16 + stride, {52, 56, 54, 255});
      rect(55, 103 - bob, 21, 27, coat);
      rect(51, 106 - bob + stride, 4, 21, coat);
      rect(76, 106 - bob - stride, 4, 21, coat);
      rect(58, 91 - bob, 15, 15, {206, 168, 128, 255});
      rect(57, 87 - bob, 17, 6, {65, 55, 45, 255});
      rect(65, 97 - bob, 2, 2, {45, 44, 34, 255});
      rect(58, 116 - bob, 16, 3, {74, 66, 47, 255});
      rect(58, 109 - bob, 5, 5, {89, 87, 62, 255});
      rect(68, 109 - bob, 5, 5, {89, 87, 62, 255});
      line(64, 106 - bob, 64, 115 - bob, {190, 171, 117, 255});
      rect(58 - stride, 141 - stride, 7, 3, {75, 58, 43, 255});
      rect(68 + stride, 141 + stride, 7, 3, {75, 58, 43, 255});
      if (base == 16) {
        rect(53, 105 - bob, 6, 19, {107, 80, 48, 255});
        rect(59, 83 - bob, 16, 5, {173, 128, 68, 255});
      }
      if (clothing >= 0 && GetContent().outfits[clothing].id == "guard") {
        rect(52, 101 - bob, 26, 5, {184, 192, 190, 255});
        rect(58, 81 - bob, 17, 10, {127, 148, 159, 255});
      }
      if (clothing >= 0 && GetContent().outfits[clothing].id == "scholar") {
        rect(54, 126, 22, 12, coat);
      }
      if (npc >= 0 && GetContent().npcs[npc].rarity >= 1) {
        rect(54, 86 - bob, 23, 3, coat);
        rect(60, 80 - bob, 11, 7, coat);
      }
    } else if (base == 17) {
      rect(51, 143, 29, 4, {14, 27, 22, 70});
      circle(62, 136 - bob * 2, 10, {188, 176, 146, 255});
      circle(74, 128 - bob * 2, 6, {198, 184, 150, 255});
      rect(73, 112 - bob * 2, 3, 14, {187, 171, 140, 255});
      rect(78, 114 - bob * 2, 3, 13, {187, 171, 140, 255});
      circle(76, 127 - bob * 2, 1, {39, 43, 32, 255});
      circle(51, 135 - bob * 2, 4, {224, 216, 194, 255});
      if (frame) {
        rect(59 - stride, 139, 4, 5, {170, 160, 132, 255});
        rect(69 + stride, 139, 4, 5, {170, 160, 132, 255});
      }
    } else if (base == 18 || base == 19 || base == 58) {
      auto quad = [&](int a, int b, int c, int d, int e, int f, int g, int h,
                      Color color) {
        tri(a, b, c, d, e, f, color);
        tri(a, b, e, f, g, h, color);
      };
      // Two wall planes, a hip roof, stone plinth and layered cast shadow.
      quad(8, 129, 63, 154, 124, 124, 68, 104, {15, 27, 23, 85});
      quad(18, 78, 64, 101, 64, 146, 18, 123, {174, 133, 83, 255});
      quad(64, 101, 110, 78, 110, 123, 64, 146, {110, 85, 60, 255});
      quad(18, 118, 64, 141, 64, 146, 18, 123, {111, 124, 115, 255});
      quad(64, 141, 110, 118, 110, 123, 64, 146, {73, 88, 83, 255});
      for (int i = 0; i < 5; i++) {
        line(20, 84 + i * 8, 62, 105 + i * 8, {119, 87, 55, 255});
        line(66, 105 + i * 8, 108, 84 + i * 8, {78, 64, 47, 255});
      }
      line(20, 79, 20, 118, {84, 63, 44, 255});
      line(63, 101, 63, 141, {86, 63, 42, 255});
      line(107, 80, 107, 119, {72, 60, 43, 255});
      Color roof = base == 19   ? Color{85, 124, 105, 255}
                   : base == 58 ? Color{172, 92, 60, 255}
                                : Color{153, 99, 61, 255};
      tri(9, 79, 64, 34, 64, 108, roof);
      tri(64, 34, 120, 79, 64, 108,
          {static_cast<uint8_t>(roof.r * .7f),
           static_cast<uint8_t>(roof.g * .7f),
           static_cast<uint8_t>(roof.b * .7f), 255});
      line(9, 79, 64, 108, {204, 151, 87, 255});
      line(64, 108, 120, 79, {96, 69, 46, 255});
      line(64, 35, 64, 107, {194, 138, 79, 255});
      for (int i = 0; i < 5; i++) {
        line(24 + i * 7, 70 - i * 5, 61, 100 - i * 9, {115, 75, 47, 255});
        line(67, 99 - i * 9, 105 - i * 6, 78 - i * 5, {86, 67, 47, 255});
      }
      // Weathered siding, nail heads, brick joints and varied paint colors.
      int variant = building < 0 ? base - 18 : building;
      for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 4; col++) {
          int px = 22 + col * 10, py = 88 + row * 8 + col * 5;
          circle(px, py, 1, {203, 171, 119, 255});
          if (variant % 3 == 1) {
            line(px, py - 2, px, py + 5, {103, 79, 64, 255});
          }
        }
      }
      Color trim = variant % 3 == 0   ? Color{122, 174, 165, 255}
                   : variant % 3 == 1 ? Color{224, 180, 114, 255}
                                      : Color{191, 123, 109, 255};
      line(19, 78, 63, 100, trim);
      line(65, 100, 109, 78, trim);
      if (variant % 3 == 2) {
        quad(25, 110, 48, 120, 48, 126, 25, 116, {83, 108, 61, 255});
        circle(31, 111, 3, {193, 116, 119, 255});
        circle(43, 117, 3, {208, 164, 94, 255});
      }
      if (variant % 4 == 1) {
        quad(68, 107, 84, 99, 96, 107, 80, 115, trim);
        line(68, 108, 80, 115, {231, 216, 167, 255});
      }
      if (variant % 4 == 2) {
        rect(48, 48, 10, 27, {124, 110, 92, 255});
        rect(46, 45, 14, 5, {171, 157, 121, 255});
      }
      // Windows sit on their respective isometric wall planes.
      quad(28, 89, 45, 98, 45, 112, 28, 103, {62, 111, 125, 255});
      quad(31, 92, 43, 98, 43, 108, 31, 102, {148, 198, 188, 255});
      line(36, 94, 36, 106, {203, 181, 125, 255});
      line(29, 98, 44, 105, {203, 181, 125, 255});
      quad(87, 95, 101, 88, 101, 101, 87, 108, {48, 87, 96, 255});
      line(94, 91, 94, 104, {161, 156, 107, 255});
      quad(68, 115, 81, 108, 81, 133, 68, 140, {49, 45, 33, 255});
      quad(70, 117, 79, 112, 79, 131, 70, 136, {112, 77, 43, 255});
      circle(77, 124, 1, {227, 193, 115, 255});
      quad(67, 141, 82, 133, 91, 137, 75, 146, {147, 151, 129, 255});
      line(70, 143, 85, 136, {202, 196, 157, 255});
      quad(88, 57, 96, 53, 96, 71, 88, 75, {112, 105, 88, 255});
      quad(96, 53, 102, 57, 102, 74, 96, 71, {79, 82, 73, 255});
      rect(88, 51, 14, 5, {159, 151, 124, 255});
      if (base == 58) {
        quad(54, 19, 68, 25, 68, 68, 54, 61, {190, 170, 128, 255});
        quad(68, 25, 80, 19, 80, 61, 68, 68, {116, 118, 98, 255});
        tri(50, 21, 67, 4, 85, 21, {87, 65, 47, 255});
        circle(62, 37, 5, {231, 213, 154, 255});
        line(62, 37, 62, 33, {65, 71, 60, 255});
        line(62, 37, 65, 39, {65, 71, 60, 255});
        line(83, 24, 83, 48, {199, 177, 120, 255});
        tri(84, 25, 102, 31, 84, 38, {186, 78, 51, 255});
      }
    } else if (base == 23) {
      for (int i = 0; i < 7; i++) {
        int x = 46 + i * 6, y = 108 + (i * 7) % 22;
        line(x, 144, x - 3, y, {120, 144, 66, 255});
        rect(x - 5, y - 7, 4, 12, {144, 113, 66, 255});
      }
    } else if (base == 24) {
      line(44, 141, 83, 141, {165, 207, 199, 190});
      line(49, 146, 77, 146, {129, 189, 190, 150});
      tri(49, 133, 77, 133, 66, 139, {160, 193, 193, 190});
      tri(73, 136, 82, 130, 82, 141, {172, 206, 202, 190});
    } else if (base == 25) {
      rect(40, 144, 54, 4, {14, 26, 27, 90});
      rect(46 - stride, 127, 6, 17 - stride, {88, 100, 100, 255});
      rect(69 + stride, 127, 6, 17 + stride, {88, 100, 100, 255});
      rect(42, 119 - bob, 40, 17, {106, 121, 121, 255});
      circle(83, 117 - bob, 10, {126, 137, 130, 255});
      tri(76, 110 - bob, 77, 101 - bob, 83, 110 - bob, {105, 119, 113, 255});
      tri(85, 110 - bob, 90, 101 - bob, 92, 113 - bob, {105, 119, 113, 255});
      rect(88, 117 - bob, 10, 6, {127, 135, 130, 255});
      rect(89, 115 - bob, 2, 2, {228, 164, 85, 255});
      tri(42, 122, 28, 113, 36, 128, {97, 113, 112, 255});
    } else if (base == 26) {
      tri(26, 130, 103, 130, 89, 153, {125, 82, 48, 255});
      tri(26, 130, 89, 153, 40, 153, {155, 102, 55, 255});
      rect(35, 128, 60, 4, {205, 156, 82, 255});
      line(40, 138, 89, 138, {84, 66, 41, 255});
    } else if (base == 27) {
      rect(44, 137, 42, 7, {97, 70, 42, 255});
      line(48, 147, 81, 133, {139, 91, 42, 255});
      tri(48, 138, 65, 105, 84, 138, {228, 114, 48, 255});
      tri(58, 138, 65, 119, 73, 138, {244, 195, 80, 255});
    } else if (base == 53) {
      tri(43, 144, 50, 120, 75, 117, {120, 128, 137, 255});
      tri(43, 144, 75, 117, 91, 142, {90, 100, 111, 255});
      rect(59, 125, 5, 5, {192, 139, 92, 255});
      rect(75, 136, 6, 4, {192, 139, 92, 255});
    } else if (base == 54) {
      tri(51, 145, 61, 105, 73, 143, {131, 182, 213, 255});
      tri(73, 143, 80, 116, 89, 144, {151, 113, 220, 255});
      line(60, 116, 62, 137, {212, 225, 239, 255});
    } else if (base == 55) {
      circle(64, 138, 14, {105, 117, 148, 255});
      circle(64, 131, 7, {240, 229, 198, 255});
      line(50, 143, 78, 143, {163, 177, 180, 255});
    } else if (base == 56) {
      rect(44, 122, 40, 23, {104, 104, 91, 255});
      rect(49, 127, 30, 4, {68, 83, 84, 255});
      rect(62, 132, 5, 7, {195, 154, 85, 255});
    } else if (base == 57) {
      for (int i = 0; i < 6; i++) {
        int x = 46 + i * 7;
        line(x, 144, x - 4, 110 + (i * 9) % 20, {154, 107, 131, 255});
        circle(x - 4, 110 + (i * 9) % 20, 4, {174, 131, 141, 255});
      }
    }
  }
  // Deterministic fine grain enriches every material without new GPU textures.
  for (int index = 0; index < count; index++) {
    for (int grain = 0; grain < 180; grain++) {
      auto bits = Hash(index * 701 + grain);
      int x = (index % 8) * 128 + bits % 128,
          y = (index / 8) * 160 + (bits / 128) % 160;
      Color c = GetImageColor(atlas, x, y);
      if (c.a != 255) {
        continue;
      }
      int shade = static_cast<int>((bits >> 20) % 11) - 5;
      c.r = static_cast<unsigned char>(std::clamp(c.r + shade, 0, 255));
      c.g = static_cast<unsigned char>(std::clamp(c.g + shade, 0, 255));
      c.b = static_cast<unsigned char>(std::clamp(c.b + shade, 0, 255));
      ImageDrawPixel(&atlas, x, y, c);
    }
  }
  atlas_ = LoadTextureFromImage(atlas);
  UnloadImage(atlas);
  SetTextureFilter(atlas_, TEXTURE_FILTER_POINT);
  const std::string candidates[] = {
      std::string(FRONTIER_ROOT) + "/assets/NotoSans.ttf",
      "/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
  std::vector<int> points;
  for (int c = 32; c < 127; c++) {
    points.push_back(c);
  }
  for (int c = 0x400; c <= 0x45f; c++) {
    points.push_back(c);
  }
  points.push_back(0x2014);
  points.push_back(0x2026);
  points.push_back(0x00b7);
  std::string font_path;
  for (auto& path : candidates) {
    if (std::filesystem::exists(path)) {
      font_path = path;
      break;
    }
  }
  // Bake every used physical size; never rescale a nearby glyph atlas.
  for (int size = 20; size <= 64; size++) {
    Font loaded = font_path.empty()
                      ? GetFontDefault()
                      : LoadFontEx(font_path.c_str(), size, points.data(),
                                   static_cast<int>(points.size()));
    bool owned = !font_path.empty() && loaded.texture.id != 0;
    if (!owned) {
      loaded = GetFontDefault();
    }
    SetTextureFilter(loaded.texture, TEXTURE_FILTER_POINT);
    fonts_.push_back({size, loaded, owned});
  }
  const std::string icon_root = std::string(FRONTIER_ROOT) + "/assets/ui/";
  icons_ = LoadTexture((icon_root + "icons.png").c_str());
  SetTextureFilter(icons_, TEXTURE_FILTER_POINT);
  std::ifstream manifest(icon_root + "icons.json");
  auto ids = nlohmann::json::parse(manifest).get<std::vector<std::string>>();
  for (int i = 0; i < static_cast<int>(ids.size()); i++) {
    icon_ids_[ids[i]] = i;
  }
  items_.reserve(4096);
  ground_details_.reserve(4096);
}
Renderer::~Renderer() {
  for (auto& f : fonts_) {
    if (f.owned) {
      UnloadFont(f.font);
    }
  }
  for (auto& map : maps_) {
    if (map.texture.id) {
      UnloadTexture(map.texture);
    }
  }
  UnloadTexture(icons_);
  UnloadTexture(atlas_);
}
const Font& Renderer::GetFont(float size) const {
  return std::min_element(fonts_.begin(), fonts_.end(),
                          [&](auto& a, auto& b) {
                            return std::abs(a.size - size) <
                                   std::abs(b.size - size);
                          })
      ->font;
}
float Renderer::TextSize(float size) {
  return std::round(std::max(16.f, size) * 1.25f * ActiveSettings().text_scale);
}
float Renderer::Measure(const std::string& s, float size) const {
  size = TextSize(size);
  return MeasureTextEx(GetFont(size), s.c_str(), size, 1).x;
}
void Renderer::Text(const std::string& s, float x, float y, float size,
                    Color color) const {
  size = TextSize(size);
  DrawTextEx(GetFont(size), s.c_str(), {std::round(x), std::round(y)}, size, 1,
             color);
}
void Renderer::Wrapped(const std::string& s, float x, float y, float width,
                       float size, Color color, int max_lines) const {
  const float line_height = TextSize(size) + 6;
  std::string line;
  int lines = 0;
  auto emit = [&] {
    Text(line, x, y + lines * line_height, size, color);
    line.clear();
    lines++;
  };
  for (size_t i = 0; i < s.size();) {
    int bytes = 0;
    GetCodepointNext(s.c_str() + i, &bytes);
    if (bytes <= 0) {
      break;
    }
    auto ch = s.substr(i, bytes);
    i += bytes;
    if (ch == "\n") {
      emit();
      if (lines >= max_lines) {
        return;
      }
      continue;
    }
    if (Measure(line + ch, size) > width && !line.empty()) {
      auto space = line.find_last_of(' ');
      if (space != std::string::npos) {
        auto tail = line.substr(space + 1);
        line.resize(space);
        emit();
        line = tail;
      } else {
        emit();
      }
      if (lines >= max_lines) {
        Text("…", x + width - 20, y + (lines - 1) * line_height, size, color);
        return;
      }
    }
    line += ch;
  }
  if (!line.empty() && lines < max_lines) {
    emit();
  }
}
void Renderer::Sprite(int index, Vec position, Color tint, bool flip) const {
  auto src = sprites_[index];
  if (index < 8 || (index >= 20 && index <= 22) || index == 52) {
    src.x += 22;
    src.y += 96;
    src.width = 84;
    src.height = 48;
    DrawTexturePro(atlas_, src,
                   {position.x, position.y, kTileSize * 1.7320508f, kTileSize},
                   {kTileSize * .8660254f, kTileSize * .5f}, 0, tint);
  } else {
    if (flip) {
      src.width = -src.width;
    }
    DrawTextureRec(atlas_, src, {position.x - 64, position.y - 144}, tint);
  }
}
bool Renderer::Visible(Vec position, const Camera2D& c, float margin) const {
  auto screen = GetWorldToScreen2D(ToVector2(position), c);
  return screen.x > -margin && screen.y > -margin &&
         screen.x < GetScreenWidth() + margin &&
         screen.y < GetScreenHeight() + margin;
}
size_t Renderer::DrawWorld(const World& world, const Save& save,
                           const Gameplay& game, const Camera2D& camera,
                           double time, float alpha,
                           const Resource* highlighted, float gathering) {
  const int outfit_start = 64 + static_cast<int>(GetContent().npcs.size()) * 5;
  items_.clear();
  ground_details_.clear();
  size_t count = 0;
  for (auto& [coord, chunk] : world.chunks) {
    // Chunk bounds first, then individual tile bounds, all in projected
    // coordinates.
    Vec mid =
        Project({(coord.x + .5f) * kChunkSize, (coord.y + .5f) * kChunkSize});
    if (!Visible(mid, camera, kChunkSize * 1.5f * camera.zoom)) {
      continue;
    }
    for (int y = 0; y < kChunkTiles; y++) {
      for (int x = 0; x < kChunkTiles; x++) {
        Vec position = Project({(coord.x * kChunkTiles + x + .5f) * kTileSize,
                                (coord.y * kChunkTiles + y + .5f) * kTileSize});
        if (!Visible(position, camera, 55)) {
          continue;
        }
        auto t = chunk.tiles[y * kChunkTiles + x];
        Vec tile_world = Unproject(position);
        t.path = t.path || (!save.submerged && RoadAt(save, tile_world));
        if (save.submerged && t.terrain != Terrain::kDeep &&
            t.terrain != Terrain::kShallow) {
          continue;
        }
        int image = t.path ? 6 + t.variant % 2 : t.variant % 6;
        if (t.terrain != Terrain::kForest) {
          image = 20 + static_cast<int>(t.terrain) - 1;
        }
        if (!save.submerged && BridgeAt(save, tile_world)) {
          image = 6;
        }
        if (save.submerged) {
          image = 52;
        }
        Color tint = WHITE;
        if (!save.submerged &&
            (t.terrain == Terrain::kDeep || t.terrain == Terrain::kShallow)) {
          uint8_t light = static_cast<uint8_t>(
              243 + std::sin(time * 1.5 + (coord.x * kChunkTiles + x) * .8 +
                             (coord.y * kChunkTiles + y) * .6) *
                        12);
          tint = {light, light, 255, 255};
        }
        if (save.submerged) {
          Vec cell_world{(coord.x * kChunkTiles + x + .5f) * kTileSize,
                         (coord.y * kChunkTiles + y + .5f) * kTileSize};
          auto cell = MapCell(cell_world);
          if (ObjectId(cell.x, cell.y, save.seed) % 31 == 0) {
            tint = {209, 137, 160, 255};
          }
          if (Length(cell_world - save.player) > 650) {
            tint = {115, 139, 147, 255};
          }
        }
        Sprite(image, position, tint);
        uint64_t detail = ObjectId(coord.x * kChunkTiles + x,
                                   coord.y * kChunkTiles + y, save.seed);
        if (!save.submerged) {
          ground_details_.push_back({position, detail, t.terrain, t.path});
        }
        count++;
      }
    }
    for (auto& resource : save.submerged ? chunk.underwater : chunk.resources) {
      if (save.harvested.contains(resource.id) ||
          (!save.submerged &&
           (RoadAt(save, resource.pos) ||
            std::any_of(save.facilities.begin(), save.facilities.end(),
                        [&](const auto& f) {
                          return Length(resource.pos - f.position) < 75;
                        })))) {
        continue;
      }
      auto position = Project(resource.pos);
      if (!Visible(position, camera)) {
        continue;
      }
      int image = resource.kind == ResourceKind::kTree    ? 8 + resource.variant
                  : resource.kind == ResourceKind::kStone ? 11
                  : resource.kind == ResourceKind::kBerry ? 12
                  : resource.kind == ResourceKind::kReed  ? 23
                  : resource.kind == ResourceKind::kFish  ? 24
                  : resource.kind == ResourceKind::kOre   ? 53
                  : resource.kind == ResourceKind::kCrystal ? 54
                  : resource.kind == ResourceKind::kPearl   ? 55
                                                            : 56;
      items_.push_back({position, image, WHITE});
    }
    for (auto& a : chunk.animals) {
      if (save.submerged) {
        continue;
      }
      auto position = Project(a.previous + (a.pos - a.previous) * alpha);
      bool moving = Length(a.pos - a.previous) > .01f;
      if (Visible(position, camera)) {
        items_.push_back({position,
                          moving ? 44 + static_cast<int>(a.walk) % 4 : 17,
                          WHITE, Project(a.pos - a.previous).x < 0});
      }
    }
    for (auto& enemy : chunk.enemies) {
      if (save.submerged) {
        continue;
      }
      if (save.defeated.contains(enemy.id)) {
        continue;
      }
      auto position =
          Project(enemy.previous + (enemy.pos - enemy.previous) * alpha);
      int image = enemy.kind ? (Length(enemy.pos - enemy.previous) > .01f
                                    ? 32 + static_cast<int>(enemy.walk) % 4
                                    : 14)
                             : (Length(enemy.pos - enemy.previous) > .01f
                                    ? 48 + static_cast<int>(enemy.walk) % 4
                                    : 25);
      if (Visible(position, camera)) {
        items_.push_back({position, image,
                          enemy.flash > 0    ? Color{255, 160, 135, 255}
                          : enemy.windup > 0 ? Color{236, 188, 130, 255}
                                             : WHITE,
                          Project(enemy.pos - enemy.previous).x < 0});
      }
    }
  }
  // Draw decoration after all terrain sprites to keep the atlas batch intact.
  for (const auto& tile : ground_details_) {
    Vec position = tile.position;
    uint64_t detail = tile.id;
    if ((tile.terrain == Terrain::kDeep || tile.terrain == Terrain::kShallow)) {
      float phase = static_cast<float>(time) * 1.7f + (detail % 97) * .1f;
      Vec ripple = position + Vec{std::sin(phase) * 8,
                                  static_cast<float>(detail % 13) - 6};
      DrawLineEx(
          ToVector2(ripple - Vec{12, 0}),
          ToVector2(ripple + Vec{8, std::sin(phase) * 2}), 1.5f,
          {135, 201, 206,
           static_cast<unsigned char>(60 + 45 * (1 + std::sin(phase)) * .5f)});
      if (tile.terrain == Terrain::kShallow && detail % 3 == 0) {
        DrawEllipseLines(static_cast<int>(ripple.x), static_cast<int>(ripple.y),
                         20, 5, {204, 224, 196, 100});
      }
    } else if (tile.terrain == Terrain::kForest && !tile.path &&
               detail % 3 == 0) {
      for (int blade = 0; blade < 3; blade++) {
        DrawLineEx(
            ToVector2(position + Vec{static_cast<float>(blade * 3), 3}),
            ToVector2(position + Vec{static_cast<float>(blade * 3 - 2), -3}), 1,
            {78, 122, 65, 180});
      }
      if (detail % 11 == 0) {
        DrawCircleV(ToVector2(position + Vec{5, -3}), 2, {225, 192, 130, 240});
      }
    }
  }
  if (!save.submerged) {
    for (const auto& road : save.roads) {
      for (size_t i = 1; i < road.points.size(); ++i) {
        float length = Length(road.points[i] - road.points[i - 1]);
        int steps = static_cast<int>(std::ceil(length / 24));
        for (int j = 0; j < steps; ++j) {
          Vec a = road.points[i - 1] + (road.points[i] - road.points[i - 1]) *
                                           (static_cast<float>(j) / steps);
          Vec b = road.points[i - 1] + (road.points[i] - road.points[i - 1]) *
                                           (static_cast<float>(j + 1) / steps);
          if (!Visible(Project(a), camera, 70)) {
            continue;
          }
          bool bridge = BridgeAt(save, (a + b) * .5f);
          Vec normal = Normalized(Vec{-(b - a).y, (b - a).x}) * 22;
          Color road_color =
              bridge ? Color{149, 106, 65, 255} : Color{151, 129, 88, 255};
          auto p0 = ToVector2(Project(a - normal)),
               p1 = ToVector2(Project(a + normal));
          auto p2 = ToVector2(Project(b + normal)),
               p3 = ToVector2(Project(b - normal));
          auto triangle = [&](Vector2 x, Vector2 y, Vector2 z) {
            if ((y.x - x.x) * (z.y - x.y) - (y.y - x.y) * (z.x - x.x) > 0) {
              std::swap(y, z);
            }
            DrawTriangle(x, y, z, road_color);
          };
          triangle(p0, p1, p2);
          triangle(p0, p2, p3);
          if (bridge) {
            Vec normal = Normalized(Vec{-(b - a).y, (b - a).x}) * 19;
            DrawLineEx(ToVector2(Project(a - normal)),
                       ToVector2(Project(a + normal)), 2, {72, 55, 36, 255});
          }
        }
      }
    }
  }
  if (!save.submerged) {
    for (auto& b : world.buildings) {
      auto position = Project(b.pos);
      if (Visible(position, camera)) {
        const int building_start =
            outfit_start + static_cast<int>(GetContent().outfits.size()) * 5;
        Color tint = WHITE;
        auto head = Project(save.player) + Vec{0, -35};
        if (head.x > position.x - 58 && head.x < position.x + 58 &&
            head.y > position.y - 135 && head.y < position.y - 15) {
          tint.a = 110;
        }
        items_.push_back(
            {position,
             b.hall ? 58
                    : building_start +
                          static_cast<int>(ObjectId(static_cast<int>(b.pos.x),
                                                    static_cast<int>(b.pos.y),
                                                    save.seed) %
                                           12),
             tint});
      }
    }
  }
  for (auto& n : world.npcs) {
    if (save.submerged || n.ruler || !ResidentActive(save, n.id)) {
      continue;
    }
    auto position = Project(n.previous + (n.pos - n.previous) * alpha);
    int image =
        64 + n.type * 5 +
        (Length(n.pos - n.previous) > .01f ? 1 + static_cast<int>(n.walk) % 4
                                           : 0);
    if (auto resident = GetCitizen(save, n.id);
        resident && !resident->outfit.empty()) {
      for (int i = 0; i < static_cast<int>(GetContent().outfits.size()); i++) {
        if (GetContent().outfits[i].id == resident->outfit) {
          image = outfit_start + i * 5 +
                  (Length(n.pos - n.previous) > .01f
                       ? 1 + static_cast<int>(n.walk) % 4
                       : 0);
        }
      }
    }
    bool flip = n.id == world.talking_npc ? Project(save.player - n.pos).x < 0
                                          : Project(n.pos - n.previous).x < 0;
    if (Visible(position, camera)) {
      items_.push_back({position, image, WHITE, flip, n.id});
    }
  }
  if (!save.submerged) {
    for (auto fire : save.campfires) {
      auto position = Project(fire);
      if (Visible(position, camera)) {
        items_.push_back(
            {position,
             27,
             {255, static_cast<uint8_t>(232 + std::sin(time * 9) * 20), 210,
              255}});
      }
    }
  }
  Vec player = Project(save.player);
  auto terrain = TerrainAt(save.player, save.seed);
  bool afloat = Watercraft(save) && !BridgeAt(save, save.player) &&
                (terrain == Terrain::kDeep || terrain == Terrain::kShallow);
  int player_image = game.moving && game.swing <= 0
                         ? 40 + static_cast<int>(game.walk) % 4
                         : 16;
  if (save.adventure) {
    for (int i = 0; i < static_cast<int>(GetContent().outfits.size()); i++) {
      if (GetContent().outfits[i].id == save.outfit_id) {
        player_image = outfit_start + i * 5 +
                       (game.moving && game.swing <= 0
                            ? 1 + static_cast<int>(game.walk) % 4
                            : 0);
      }
    }
  }
  if (!afloat) {
    items_.push_back({player, player_image,
                      game.invulnerable > 0 && static_cast<int>(time * 12) % 2
                          ? Color{255, 158, 145, 210}
                          : WHITE,
                      Project(game.facing).x < 0});
  }
  if (highlighted) {
    auto position = Project(highlighted->pos);
    DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y), 20,
                10, {228, 195, 116, 160});
  }
  DrawEllipse(static_cast<int>(player.x), static_cast<int>(player.y), 17, 8,
              {237, 202, 123, 60});
  std::sort(items_.begin(), items_.end(),
            [](auto& a, auto& b) { return a.pos.y < b.pos.y; });
  for (auto& item : items_) {
    Sprite(item.sprite, item.pos, item.tint, item.flip);
    if (item.worker >= 0) {
      auto resident = GetCitizen(save, item.worker);
      if (resident && resident->trained && resident->job != "idle") {
        float phase = std::sin(time * 5 + item.worker);
        auto hand = ToVector2(item.pos + Vec{14, -28 + phase * 3});
        DrawLineEx({item.pos.x + 9, item.pos.y - 31}, hand, 4,
                   {206, 165, 122, 255});
        if (resident->job == "fisher") {
          DrawLineEx(hand, {hand.x + 16, hand.y - 32}, 2, {181, 144, 83, 255});
        } else if (resident->job == "scholar" ||
                   resident->job == "ambassador" || resident->job == "trader") {
          DrawRectangle(static_cast<int>(hand.x - 5),
                        static_cast<int>(hand.y - 8), 10, 12,
                        {201, 187, 135, 255});
        } else {
          DrawLineEx(hand, {hand.x + phase * 13, hand.y - 20}, 3,
                     {149, 116, 70, 255});
          DrawCircleV({hand.x + phase * 13, hand.y - 20}, 4,
                      {162, 180, 177, 255});
        }
      }
    }
  }
  if (!save.submerged) {
    for (auto& resident : save.citizens) {
      if (!resident.weapon.empty() &&
          resident.npc < static_cast<int>(world.npcs.size())) {
        auto& npc = world.npcs[resident.npc];
        auto position =
            Project(npc.previous + (npc.pos - npc.previous) * alpha);
        if (!Visible(position, camera)) {
          continue;
        }
        DrawLineEx({position.x + 12, position.y - 26},
                   {position.x + 23, position.y - 42}, 3, {186, 194, 183, 255});
        DrawLineEx({position.x + 9, position.y - 29},
                   {position.x + 17, position.y - 24}, 3, {189, 153, 91, 255});
      }
    }
  }
  if (afloat) {
    Vehicle(save, game, player, time);
  }
  if (!save.submerged) {
    for (auto& wall : save.walls) {
      if (wall.hp > 0) {
        auto a = Project(wall.a), b = Project(wall.b);
        if (!Visible(a, camera) && !Visible(b, camera)) {
          continue;
        }
        Color wall_color = save.technologies.contains("fortification")
                               ? Color{142, 155, 147, 255}
                               : Color{153, 111, 66, 255};
        DrawLineEx(ToVector2(a), ToVector2(b), 12, {85, 69, 47, 255});
        int posts = static_cast<int>(Length(wall.b - wall.a) / 13);
        for (int i = 0; i <= posts; i++) {
          auto position = a + (b - a) * (static_cast<float>(i) / posts);
          DrawLineEx({position.x, position.y}, {position.x, position.y - 27}, 8,
                     wall_color);
        }
        DrawLineEx({a.x, a.y - 14}, {b.x, b.y - 14}, 4, {93, 67, 43, 255});
      }
    }
  }
  if (gathering > 0 && highlighted) {
    float phase = std::sin(time * 10);
    Vec direction = Normalized(Project(highlighted->pos - save.player));
    Vec hand = player + Vec{direction.x * 15, -26},
        tip = hand + Vec{phase * 18, -18 + phase * 15};
    DrawLineEx(ToVector2(player + Vec{0, -26}), ToVector2(hand), 4,
               {205, 167, 124, 255});
    DrawLineEx(ToVector2(hand), ToVector2(tip), 3, {161, 120, 64, 255});
    DrawLineEx(ToVector2(tip - Vec{7, 2}), ToVector2(tip + Vec{7, -2}), 6,
               {183, 194, 176, 255});
  }
  for (const auto& effect : game.hit_effects) {
    auto p = Project(effect.position);
    if (effect.defeated) {
      auto image = sprites_[effect.kind ? 14 : 25];
      image.x += effect.kind ? 50 : 28;
      image.y += effect.kind ? 89 : 98;
      image.width = effect.kind ? 34 : 84;
      image.height = effect.kind ? 59 : 48;
      DrawTexturePro(
          atlas_, image, {p.x - 28, p.y - 14, 56, 24}, {0, 0}, 0,
          {129, 137, 118,
           static_cast<unsigned char>(200 * (1 - effect.age / 1.5f))});
    } else {
      for (int i = 0; i < 6; i++) {
        float angle = i * 6.2831853f / 6;
        DrawCircleV({p.x + std::cos(angle) * effect.age * 80,
                     p.y - 24 + std::sin(angle) * effect.age * 45},
                    2,
                    {239, 194, 105,
                     static_cast<unsigned char>(220 * (1 - effect.age / .3f))});
      }
    }
  }
  if (game.swing > 0) {
    auto& weapon = game.SwingingWeapon();
    float progress =
        game.attack_elapsed / std::max(game.attack_duration, .001f);
    float angle = progress < .3f    ? -1.3f - progress
                  : progress < .58f ? -1.6f + (progress - .3f) / .28f * 2.9f
                                    : 1.3f * (1 - (progress - .58f) / .42f);
    float aim = std::atan2(game.attack_facing.y, game.attack_facing.x);
    if (weapon.kind == "spear" || weapon.kind == "bow" ||
        weapon.kind == "fists") {
      angle = 0;
    }
    Vec direction{std::cos(aim + angle), std::sin(aim + angle)};
    float reach = std::min(weapon.reach, 150.f) * .75f;
    Vec hand = player + Vec{0, -25} + Project(direction * 17),
        tip = player + Vec{0, -25} + Project(direction * reach);
    DrawLineEx({player.x, player.y - 24}, ToVector2(hand), 5,
               {205, 165, 121, 255});
    if (weapon.kind == "fists") {
      DrawCircleV(ToVector2(hand + Project(direction * 12)), 5,
                  {208, 166, 123, 255});
    } else if (weapon.kind == "bow") {
      auto normal = Normalized(Project(direction));
      Vec cross{-normal.y, normal.x};
      float degrees = std::atan2(normal.y, normal.x) * 180 / 3.14159265f;
      DrawCircleSectorLines(ToVector2(hand), 20, degrees - 90, degrees + 90, 12,
                            {192, 154, 85, 255});
      DrawLineEx(ToVector2(hand + cross * 20),
                 ToVector2(hand - normal * (progress < .3f ? 10.f : 0)), 1,
                 kInk);
      DrawLineEx(ToVector2(hand - cross * 20),
                 ToVector2(hand - normal * (progress < .3f ? 10.f : 0)), 1,
                 kInk);
    } else {
      Color blade = weapon.rarity >= 2 ? Color{166, 207, 237, 255}
                                       : Color{222, 224, 208, 255};
      auto unit = Normalized(tip - hand);
      Vec cross{-unit.y, unit.x};
      DrawLineEx(ToVector2(hand), ToVector2(tip),
                 weapon.kind == "hammer" ? 6.f : 4.f, blade);
      DrawLineEx(ToVector2(hand + cross * 8), ToVector2(hand - cross * 8), 3,
                 kGold);
      if (weapon.kind == "axe") {
        DrawLineEx(ToVector2(tip + cross * 13), ToVector2(tip - cross * 3), 9,
                   blade);
      }
      if (weapon.kind == "hammer") {
        DrawLineEx(ToVector2(tip + cross * 10), ToVector2(tip - cross * 10), 12,
                   blade);
      }
      if (progress >= .3f && progress < .58f) {
        for (int i = 1; i <= 3; i++) {
          Vec d{std::cos(aim + angle - i * .15f),
                std::sin(aim + angle - i * .15f)};
          auto trail = player + Vec{0, -25} + Project(d * reach);
          DrawLineEx(
              ToVector2(player + Vec{0, -25} + Project(d * reach * .45f)),
              ToVector2(trail), 2,
              {184, 207, 209, static_cast<uint8_t>(100 / i)});
        }
      }
    }
  }
  for (auto& arrow : game.projectiles) {
    auto position = Project(arrow.pos),
         d = Project(Normalized(arrow.velocity) * 23);
    DrawLineEx({position.x, position.y - 24},
               {position.x - d.x, position.y - d.y - 24}, 2, kGold);
  }
  for (auto& [coord, chunk] : world.chunks) {
    for (auto& enemy : chunk.enemies) {
      if (save.submerged || save.defeated.contains(enemy.id)) {
        continue;
      }
      auto position =
          Project(enemy.previous + (enemy.pos - enemy.previous) * alpha);
      if (!Visible(position, camera, 30)) {
        continue;
      }
      if (enemy.windup > 0) {
        DrawEllipseLines(static_cast<int>(position.x),
                         static_cast<int>(position.y), 26, 13,
                         {241, 143, 93, 255});
      }
      int max_hp = enemy.kind ? 100 : 60;
      if (enemy.hp == max_hp) {
        continue;
      }
      DrawRectangle(static_cast<int>(position.x - 20),
                    static_cast<int>(position.y - 52), 40, 5,
                    {30, 38, 33, 240});
      DrawRectangle(static_cast<int>(position.x - 20),
                    static_cast<int>(position.y - 52),
                    std::max(0, enemy.hp) * 40 / max_hp, 5, {207, 98, 79, 255});
    }
  }
  // Player marker remains visible behind trees.
  DrawCircleV({player.x, player.y - 64}, 3, {246, 213, 143, 255});
  return count + items_.size();
}
void Renderer::DrawLabels(const World& world, const Save& save, const Ui& ui,
                          const Camera2D& camera) const {
  if (save.submerged) {
    return;
  }
  for (size_t city = 0; city < Settlements().size(); city++) {
    auto& s = Settlements()[city];
    auto iso = Project(s.pos) + Vec{0, -330};
    if (!Visible(iso, camera)) {
      continue;
    }
    auto screen = GetWorldToScreen2D(ToVector2(iso), camera);
    Vec position{screen.x, screen.y};
    std::string city_label =
        s.name + (OwnsCity(save, static_cast<int>(city)) ? " · your town" : "");
    float width = Measure(city_label, 22);
    DrawRectangleRounded(
        {position.x - width * .5f - 15, position.y - 6, width + 30, 35}, .25f,
        4, {22, 35, 29, 200});
    Color color = save.relations[city].treaty == Treaty::kWar
                      ? Color{228, 131, 110, 255}
                      : kInk;
    Text(city_label, position.x - width * .5f, position.y, 22, color);
  }
  for (auto& npc : world.npcs) {
    if (npc.ruler || npc.id == ui.npc) {
      continue;
    }
    auto point = Project(npc.pos) + Vec{0, -83};
    if (!Visible(point, camera)) {
      continue;
    }
    auto quest = NpcQuest(save, npc);
    if (!quest) {
      continue;
    }
    auto found = save.quests.find(npc.id);
    if (found != save.quests.end() && found->second.completed) {
      continue;
    }
    auto screen = GetWorldToScreen2D(ToVector2(point), camera);
    DrawCircleV({screen.x, screen.y}, 10, {38, 52, 39, 230});
    Text(found == save.quests.end() ? "?" : "!", screen.x - 4, screen.y - 9, 18,
         kGold);
  }
  if (ui.npc >= 0 && ui.npc < static_cast<int>(world.npcs.size()) &&
      !world.npcs[ui.npc].ruler) {
    auto& npc = world.npcs[ui.npc];
    auto iso = Project(npc.pos) + Vec{0, -90};
    auto screen = GetWorldToScreen2D(ToVector2(iso), camera);
    DrawEllipse(static_cast<int>(screen.x), static_cast<int>(screen.y), 28, 18,
                {238, 235, 212, 255});
    DrawTriangle({screen.x - 10, screen.y + 12}, {screen.x + 3, screen.y + 29},
                 {screen.x + 10, screen.y + 12}, {238, 235, 212, 255});
    for (int i = -1; i <= 1; i++) {
      DrawCircleV(
          {screen.x + i * 9.f, screen.y},
          ui.busy ? 2.5f + static_cast<float>(std::sin(ui.time * 4 + i)) * .6f
                  : 2.5f,
          {54, 69, 56, 255});
    }
  }
}
void Renderer::Vehicle(const Save& save, const Gameplay& game, Vec position,
                       double time) const {
  int type = ActiveVehicle(save);
  Vec direction = Normalized(Project(game.facing)),
      side{-direction.y, direction.x};
  const auto& vessel = Vessel(type);
  float size = vessel.length, width = vessel.width;
  auto point = [&](float x, float y) {
    return ToVector2(position + direction * x + side * y);
  };
  auto triangle = [](Vector2 a, Vector2 b, Vector2 c, Color color) {
    if ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0) {
      std::swap(b, c);
    }
    DrawTriangle(a, b, c, color);
  };
  if (save.submerged) {
    for (int bubble = 0; bubble < 9; bubble++) {
      float age =
          std::fmod(static_cast<float>(time) * .65f + bubble * .13f, 1.f);
      auto p = position -
               direction * (size + age * (game.moving ? 120.f : 25.f)) +
               side * (std::sin(bubble * 3.f) * 12) + Vec{0, -age * 28};
      DrawCircleLines(
          static_cast<int>(p.x), static_cast<int>(p.y), 2 + age * 5,
          {161, 218, 224, static_cast<unsigned char>(170 * (1 - age))});
    }
  }
  if (game.moving && !save.submerged) {
    for (int i = 0; i < 5; i++) {
      auto trail = position - direction * (size + i * 13.f);
      DrawLineEx(ToVector2(trail - side * (width + i * 5)),
                 ToVector2(trail + side * (width + i * 5)), 1,
                 {159, 215, 211, static_cast<uint8_t>(110 - i * 20)});
    }
  }
  Color hull = type == 9   ? Color{61, 87, 114, 255}
               : type == 8 ? Color{97, 66, 46, 255}
               : type == 4 ? Color{113, 72, 42, 255}
               : type == 3 ? Color{133, 154, 155, 255}
               : type == 2 ? Color{122, 82, 50, 255}
                           : Color{162, 108, 59, 255};
  std::vector<Vector2> outline;
  if (type == 3) {
    for (int i = 0; i < 12; i++) {
      float a = i * 6.2831853f / 12;
      outline.push_back(point(std::cos(a) * size, std::sin(a) * width));
    }
  } else {
    outline = {point(size, 0),
               point(size * .4f, width),
               point(-size * .65f, width * .85f),
               point(-size, width * .35f),
               point(-size, -width * .35f),
               point(-size * .65f, -width * .85f),
               point(size * .4f, -width)};
  }
  Color rim = type == 3 ? Color{181, 195, 191, 255} : Color{213, 171, 103, 255};
  for (size_t i = 0; i < outline.size(); i++) {
    triangle(ToVector2(position), outline[i], outline[(i + 1) % outline.size()],
             hull);
    DrawLineEx(outline[i], outline[(i + 1) % outline.size()], 3, rim);
  }
  for (int i = -3; i <= 3; i++) {
    float x = i * size * .21f;
    DrawLineEx(point(x, -width * .66f), point(x, width * .66f),
               type == 3 ? 2 : 1,
               type == 3 ? Color{86, 108, 117, 255} : Color{81, 54, 35, 255});
    for (int sign : {-1, 1}) {
      DrawCircleV(point(x, sign * width * .7f), 1.5f, {205, 190, 151, 255});
    }
  }
  if (type == 3) {
    for (int sign : {-1, 1}) {
      triangle(point(-size * .55f, sign * width * .6f),
               point(-size * .9f, sign * width * 1.4f),
               point(-size * .2f, sign * width * .8f), {93, 125, 135, 255});
    }
    DrawLineEx(point(-size, 0), point(-size - 13, 0), 4, {109, 118, 114, 255});
    float spin = game.moving ? std::sin(time * 22) * 12 : 9;
    DrawLineEx(point(-size - 10, -spin), point(-size - 10, spin), 3,
               {192, 159, 91, 255});
    DrawCircleV(point(size * .65f, 0), 5, {247, 222, 137, 255});
    for (int i : {-1, 1}) {
      DrawCircleV(point(i * size * .4f, -3), 10, {65, 81, 87, 255});
      DrawCircleV(point(i * size * .4f, -3), 7, {104, 208, 221, 255});
      DrawCircleV(point(i * size * .4f - 2, -5), 2, {215, 250, 245, 255});
    }
    DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y - 5),
                22, 15, {107, 126, 131, 255});
    DrawCircleV(ToVector2(position + direction * 24 + Vec{0, -9}), 7,
                {116, 211, 218, 255});
    DrawCircleV(ToVector2(position - direction * 19 + Vec{0, -9}), 7,
                {116, 211, 218, 255});
    if (!save.submerged) {
      DrawLineEx({position.x, position.y - 5}, {position.x, position.y - 30}, 4,
                 {179, 183, 171, 255});
    } else {
      DrawCircleLines(static_cast<int>(position.x),
                      static_cast<int>(position.y),
                      75 + std::sin(time * 1.4) * 5, {110, 205, 213, 100});
      if (game.sonar_cooldown > 6) {
        DrawCircleLines(static_cast<int>(position.x),
                        static_cast<int>(position.y),
                        (10 - game.sonar_cooldown) * 180, {113, 219, 224, 140});
      }
    }
    return;
  }
  if (type == 2 || type >= 7) {
    for (int sign : {-1, 1}) {
      DrawLineEx(point(-size * .8f, sign * width * .7f),
                 point(size * .55f, sign * width * .7f), 2,
                 {224, 199, 145, 255});
      for (int i = -3; i <= 2; i++) {
        DrawLineEx(point(i * size * .22f, sign * width * .7f),
                   point(i * size * .22f, sign * width * .7f - 9), 2,
                   {188, 152, 94, 255});
      }
    }
    triangle(point(-size * .45f, -width * .45f), point(-size * .8f, 0),
             point(-size * .4f, width * .45f), {100, 54, 34, 255});
    DrawCircleV(point(-size * .6f, 0), 6, {202, 176, 112, 255});
    DrawLineEx({position.x, position.y - 114}, point(-size * .7f, -width * .6f),
               1, {197, 184, 150, 255});
    DrawLineEx({position.x, position.y - 114}, point(size * .7f, 0), 1,
               {197, 184, 150, 255});
  }
  // Seated pilot is part of the vehicle; no walking legs are drawn on water.
  auto coat = PlayerOutfit(save).color;
  DrawRectangle(static_cast<int>(position.x - 9),
                static_cast<int>(position.y - 23), 18, 18,
                {static_cast<uint8_t>(coat[0]), static_cast<uint8_t>(coat[1]),
                 static_cast<uint8_t>(coat[2]), 255});
  DrawCircleV({position.x, position.y - 32}, 7, {208, 169, 131, 255});
  if (type == 5) {
    for (int i = -3; i <= 3; i++) {
      DrawLineEx(point(-size, i * width / 4), point(size, i * width / 4), 5,
                 {169, 130, 69, 255});
      DrawLineEx(point(-size * .6f, -width), point(-size * .6f, width), 2,
                 {81, 75, 49, 255});
      DrawLineEx(point(size * .6f, -width), point(size * .6f, width), 2,
                 {81, 75, 49, 255});
    }
  }
  if (type == 6) {
    DrawCircleV(point(-size * .5f, 0), 9, {161, 116, 67, 255});
    DrawCircleV(point(size * .5f, 0), 5, {230, 204, 127, 255});
  }
  if (type == 1 || type == 4 || type == 6 || type == 5) {
    float stroke = game.moving ? std::sin(time * 5) * 16 : 0;
    for (int sign : {-1, 1}) {
      auto pivot = position + side * (static_cast<float>(sign) * 13) -
                   direction * 5,
           blade = position + side * (static_cast<float>(sign) * 43) +
                   direction * stroke;
      DrawLineEx(ToVector2(pivot), ToVector2(blade), 3, {213, 177, 113, 255});
      DrawCircleV(ToVector2(blade), 4, {213, 177, 113, 255});
    }
  } else {
    const int masts = type == 9 ? 3 : type == 8 ? 2 : 1;
    for (int mast = 0; mast < masts; mast++) {
      auto foot = point((mast - (masts - 1) * .5f) * size * .62f, 0);
      float height = type == 9   ? 132.f
                     : type == 8 ? 120.f
                     : type == 7 ? 96.f
                                 : 116.f;
      Vector2 top{foot.x, foot.y - height};
      DrawLineEx(foot, top, 4, {154, 120, 75, 255});
      float wind = std::sin(time * 1.3 + mast) * 5;
      Color cloth =
          type == 9 ? Color{232, 224, 193, 255} : Color{223, 212, 178, 255};
      triangle({top.x + 4, top.y + 6}, {top.x + 50 + wind, foot.y - 28},
               {top.x + 4, foot.y - 20}, cloth);
      for (int seam = 0; seam < 4; seam++) {
        DrawLineEx({top.x + 6, top.y + 20 + seam * 17},
                   {top.x + 14 + seam * 10 + wind, top.y + 28 + seam * 17}, 1,
                   {170, 158, 123, 200});
      }
      DrawLineEx(top, point(size * .8f, 0), 1, {189, 174, 136, 255});
      DrawLineEx(top, point(-size * .8f, 0), 1, {189, 174, 136, 255});
      triangle({top.x + 2, top.y}, {top.x + 25, top.y + 6},
               {top.x + 2, top.y + 12},
               type == 9 ? Color{75, 107, 159, 255} : Color{180, 84, 59, 255});
      if (type == 9) {
        DrawCircleV({top.x + 13, top.y + 6}, 3, {230, 190, 98, 255});
      }
    }
    if (type >= 8) {
      for (int i = -3; i <= 3; i++) {
        for (int sign : {-1, 1}) {
          DrawCircleV(point(i * size * .2f, sign * width * .83f), 3,
                      {237, 198, 116, 255});
        }
      }
      auto stern = point(-size * .65f, 0);
      DrawRectangleRounded({stern.x - 15, stern.y - 30, 30, 22}, .1f, 4,
                           {106, 75, 47, 255});
      for (int i = -1; i <= 1; i++) {
        DrawRectangle(static_cast<int>(stern.x + i * 8 - 2),
                      static_cast<int>(stern.y - 27), 4, 8,
                      {172, 218, 219, 255});
      }
    }
  }
}
int Renderer::MenuPageRows() {
  return std::clamp(static_cast<int>((GetScreenHeight() - 40 - kMenuListTop -
                                      kMenuFooterHeight) /
                                     kMenuRowHeight),
                    1, 8);
}
Rectangle Renderer::PauseBounds() {
  float width = std::min(720.f, GetScreenWidth() - 48.f),
        height = std::min(470.f, GetScreenHeight() - 48.f);
  return {(GetScreenWidth() - width) * .5f, (GetScreenHeight() - height) * .5f,
          width, height};
}
Rectangle Renderer::PauseSummaryBounds() {
  return ProgressSummaryBounds();
}
Rectangle Renderer::ProgressSummaryBounds() {
  return {44, 94, GetScreenWidth() - 88.f, GetScreenHeight() - 154.f};
}
Rectangle Renderer::ProgressCloseButton() {
  return {GetScreenWidth() - 180.f, 38, 130, 40};
}
Rectangle Renderer::PauseButton(int choice) {
  auto box = PauseBounds();
  return {box.x + 24, box.y + 80 + choice * 58.f, box.width - 48, 48};
}
Rectangle Renderer::MenuBounds(int rows) {
  float width = std::min(960.f, GetScreenWidth() - 40.f),
        height = std::min(rows, 8) * kMenuRowHeight + kMenuListTop +
                 kMenuFooterHeight;
  return {(GetScreenWidth() - width) * .5f, (GetScreenHeight() - height) * .5f,
          width, height};
}
void Renderer::DrawUi(const World& world, Save& save, Ui& ui,
                      const Camera2D& camera) {
  float w = static_cast<float>(GetScreenWidth()),
        h = static_cast<float>(GetScreenHeight());
  DrawRectangleRounded({20, 20, 390, 52}, .12f, 6, kPanel);
  Text("FOREST FRONTIER", 38, 29, 28, kGold);
  DrawRectangleRounded({20, 84, 390, 188}, .1f, 6, kPanel);
  Text("Health " + std::to_string(save.health) + " / 100", 38, 96, 16, kInk);
  DrawRectangle(38, 122, 224, 6, {45, 59, 45, 255});
  DrawRectangle(
      38, 122, 224 * save.health / 100, 6,
      save.health < 30 ? Color{220, 104, 83, 255} : Color{137, 185, 121, 255});
  Text("Coins " + std::to_string(save.coins), 38, 138, 16, kGold);
  Wrapped("Wood " + std::to_string(ui.wood) + "   Stone " +
              std::to_string(ui.stone) + "   Berries " +
              std::to_string(ui.berries),
          38, 163, 354, 14, kInk, 1);
  Wrapped("Reeds " + std::to_string(save.inventory.reeds) + "   Fish " +
              std::to_string(save.inventory.fish) + "   Leather " +
              std::to_string(save.inventory.leather),
          38, 185, 354, 14, kInk, 1);
  Text("Potions " + std::to_string(save.inventory.potions) + "   Campfires " +
           std::to_string(save.inventory.campfires),
       38, 207, 14, kMuted);
  Text(save.submerged ? "Underwater world"
                      : TerrainName(TerrainAt(save.player, save.seed)),
       38, 242, 15, kGold);
  DrawRectangleRounded({20, 284, 390, 86}, .1f, 6, kPanel);
  std::string equipment = CurrentWeapon(save).name;
  if (save.adventure) {
    equipment += " · " + PlayerOutfit(save).name;
  } else if (save.inventory.armor) {
    equipment += " · Armor";
  }
  if (Watercraft(save)) {
    equipment += " · " + Vessel(ActiveVehicle(save)).name;
  }
  Wrapped(equipment, 38, 291, 354, 16, kInk, 2);
  DrawRectangleRounded({20, 372, 390, 102}, .1f, 6, kPanel);
  Text("Iron " + std::to_string(GoodCount(save.inventory, 6)) + " · Copper " +
           std::to_string(GoodCount(save.inventory, 7)) + " · Steel " +
           std::to_string(GoodCount(save.inventory, 10)),
       38, 384, 16, kInk);
  Text("Crystals " + std::to_string(GoodCount(save.inventory, 8)) +
           " · Pearls " + std::to_string(GoodCount(save.inventory, 9)),
       38, 410, 16, kInk);
  Text(save.submerged ? "Air " + std::to_string(static_cast<int>(save.oxygen)) +
                            " · Hull " + std::to_string(save.hull)
                      : "Arrows " + std::to_string(save.inventory.ammo),
       38, 443, 16, kGold);

  float mx = w - 280, my = 20;
  DrawRectangleRounded({mx, my, 260, 270}, .08f, 6, kPanel);
  Text(save.submerged ? "Seafloor map" : "Explored world", mx + 16, my + 12, 16,
       kInk);
  DrawMap(save, ui, camera, {mx + 15, my + 42, 230, 196}, false);
  Text("Bright area: currently visible", mx + 16, my + 246, 13, kMuted);

  if (ui.debug) {
    DrawRectangleRounded({w - 300, 302, 280, 138}, .1f, 6, kPanel);
    std::ostringstream perf;
    perf << std::fixed << std::setprecision(1) << "FPS "
         << (ui.fps > 0 ? ui.fps : GetFPS()) << "   p95 " << ui.p95 << " ms";
    Text(perf.str(), w - 282, 315, 17, ui.p95 > 20 ? kGold : kInk);
    std::ostringstream work;
    work << std::fixed << std::setprecision(2) << "CPU " << ui.work_ms
         << " ms · p99 " << ui.p99;
    Text(work.str(), w - 282, 342, 13, kMuted);
    Text("Chunks " + std::to_string(ui.chunks) + " / pending " +
             std::to_string(ui.pending),
         w - 282, 364, 14, kMuted);
    Text("Visible " + std::to_string(ui.visible) + " / animals " +
             std::to_string(ui.animals),
         w - 282, 385, 14, kMuted);
    Text(ui.benchmark ? "Auto-route enabled"
                      : ActionKey(InputAction::kDebug) + ": performance",
         w - 282, 409, 13, ui.benchmark ? kGold : kMuted);
  }
  for (const auto& f : save.facilities) {
    auto projected = Project(f.position);
    auto pos = GetWorldToScreen2D(ToVector2(projected + Vec{0, -145}), camera);
    if (pos.x < -100 || pos.x > w + 100 || pos.y < 0 || pos.y > h - 150) {
      continue;
    }
    std::string label = f.kind;
    float width = Measure(label, 14) + 16;
    DrawRectangleRounded({pos.x - width * .5f, pos.y, width, TextSize(14) + 8},
                         .3f, 6, kPanel);
    Text(label, pos.x - width * .5f + 8, pos.y + 3, 14, kGold);
  }
  if (!ui.Blocked()) {
    int patron = -1;
    for (const auto& [coord, chunk] : world.chunks) {
      for (const auto& enemy : chunk.enemies) {
        if (enemy.kind && !save.defeated.contains(enemy.id) &&
            save.relations[enemy.city].treaty == Treaty::kWar &&
            Length(enemy.pos - save.player) < 360) {
          patron = enemy.city;
          break;
        }
      }
      if (patron >= 0) {
        break;
      }
    }
    if (patron >= 0) {
      Rectangle surrender{w - 340, h * .5f, 320, 72};
      DrawRectangleRounded(surrender, .2f, 8, {97, 60, 37, 245});
      Wrapped("Surrender to " + Settlements()[patron].name +
                  " · 30 coins + tribute",
              surrender.x + 10, surrender.y + 8, 300, 17, kInk, 2);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(GetMousePosition(), surrender)) {
        int local = CityAt(save, save.player);
        ui.pending_notice =
            SurrenderTown(save, OwnsCity(save, local) ? local : save.home_city,
                          patron)
                .message;
      }
    }
  }
  // Context actions are keycaps on an opaque panel, independent of terrain.
  const float chip_height = 36;
  float usable = w - 64, x = 0, y = 0;
  for (const auto& [key, description] : ui.actions) {
    float width = Measure(key, 17) + Measure(description, 17) + 40;
    if (x + width > usable && x > 0) {
      x = 0;
      y += chip_height + 7;
    }
    x += width + 8;
  }
  float panel_height = y + chip_height + 52;
  ui.action_bar_top = h - panel_height - 16;
  DrawRectangleRounded({20, h - panel_height - 16, w - 40, panel_height}, .1f,
                       10, {13, 26, 19, 245});
  x = 32;
  float row_y = h - panel_height - 4;
  for (const auto& [key, description] : ui.actions) {
    float key_width = Measure(key, 17) + 18,
          width = key_width + Measure(description, 17) + 22;
    if (x + width > w - 32 && x > 32) {
      x = 32;
      row_y += chip_height + 7;
    }
    DrawRectangleRounded({x, row_y, key_width, chip_height}, .3f, 8,
                         {74, 99, 61, 255});
    Text(key, x + 9, row_y + 6, 17, kGold);
    Text(description, x + key_width + 8, row_y + 6, 17, kInk);
    x += width + 8;
  }
  Wrapped(
      ui.hint.empty() ? "Explore, gather and learn through quests." : ui.hint,
      32, h - 49, w - 64, 17, kGold, 1);
  if (!ui.notice.empty()) {
    float tw = Measure(ui.notice, 17), width = std::min(w - 40, tw + 36);
    DrawRectangleRounded(
        {(w - width) * .5f, 28, width, tw > width - 36 ? 62.f : 38.f}, .2f, 4,
        kPanel);
    Wrapped(ui.notice, (w - width) * .5f + 18, 37, width - 36, 17, kGold, 2);
  }
  if (ui.harvest_progress > 0) {
    DrawRectangle(static_cast<int>(w * .5f - 75),
                  static_cast<int>(h * .5f + 50), 150, 6, {20, 32, 24, 230});
    DrawRectangle(static_cast<int>(w * .5f - 75),
                  static_cast<int>(h * .5f + 50),
                  static_cast<int>(150 * ui.harvest_progress), 6, kGold);
  }
  if (save.tracked_quest >= 0 &&
      save.tracked_quest < static_cast<int>(world.npcs.size()) &&
      save.quests.contains(save.tracked_quest)) {
    auto& progress = save.quests.at(save.tracked_quest);
    auto& task = Quest(progress.id);
    auto& giver = world.npcs[save.tracked_quest];
    float y = ui.debug ? 452.f : 302.f;
    DrawRectangleRounded({w - 300, y, 280, 92}, .08f, 6, kPanel);
    Text(task.title, w - 282, y + 10, 17, kGold);
    Text(std::to_string(
             std::min(task.target, GetQuestProgress(save, progress))) +
             " / " + std::to_string(task.target),
         w - 282, y + 36, 16, kInk);
    Text(giver.name + " · " +
             std::to_string(static_cast<int>(Length(giver.pos - save.player) /
                                             kTileSize)) +
             " tiles",
         w - 282, y + 61, 16, kMuted);
  }
  if (ui.npc >= 0) {
    float pw = std::min(720.f, w - 50), ph = 500, px = (w - pw) * .5f,
          py = h - ph - 96;
    auto& npc = world.npcs.at(ui.npc);
    if (npc.ruler) {
      float room_y = std::max(20.f, py - 125);
      DrawRectangle(static_cast<int>(px - 15), static_cast<int>(room_y),
                    static_cast<int>(pw + 30),
                    static_cast<int>(py + ph - room_y + 15), {73, 58, 43, 255});
      for (float y = room_y; y < py + ph; y += 45) {
        DrawLineEx({px - 15, y}, {px + pw + 15, y}, 2, {105, 82, 54, 255});
      }
      DrawRectangle(static_cast<int>(px + 35), static_cast<int>(room_y + 35),
                    70, 55, {123, 77, 43, 255});
      Sprite(64 + npc.type * 5, {px + 70, room_y + 100});
      DrawRectangleRounded({px + 44, room_y + 7, 52, 23}, .4f, 8,
                           {236, 227, 190, 255});
      for (int i = 0; i < 3; i++) {
        DrawCircleV({px + 57 + i * 12.f, room_y + 18}, 2, {80, 99, 75, 255});
      }
      Text("Audience in the town hall", px + 125, room_y + 43, 20, kGold);
    }
    DrawRectangleRounded({px, py, pw, ph}, .04f, 8, {18, 29, 28, 250});
    DrawRectangle(static_cast<int>(px), static_cast<int>(py),
                  static_cast<int>(pw), 3, kGold);
    Text(npc.name + " · " + npc.role, px + 24, py + 20, 24, kGold);
    Text("ESC: close", px + pw - 125, py + 25, 16, kMuted);
    Text(Settlements()[npc.settlement].name, px + 24, py + 53, 16, kMuted);
    Wrapped(NpcIntroduction(save, npc), px + 24, py + 79, pw - 48, 17, kInk, 3);
    Wrapped(QuestStatus(save, npc), px + 24, py + 155, pw - 48, 17, kGold, 5);
    auto quest = NpcQuest(save, npc);
    auto found = save.quests.find(npc.id);
    DrawRectangleRounded(
        {px + 20, py + 269, pw - 40, 32}, .1f, 4,
        quest ? Color{64, 84, 53, 255} : Color{40, 51, 43, 255});
    Text(!quest ? "No quest offered"
         : found == save.quests.end()
             ? ActionKey(InputAction::kQuestAction) + ": accept quest"
         : found->second.completed
             ? "Quest completed"
             : ActionKey(InputAction::kQuestAction) + ": turn in quest",
         px + 32, py + 275, 17, kInk);
    Text(ActionKey(InputAction::kTeacher) + ": learn from the town master",
         px + 24, py + 315, 16, kMuted);
    if (auto resident = GetCitizen(save, npc.id)) {
      auto weapons = OwnedWeapons(save);
      auto clothes = OwnedOutfits(save);
      clothes.insert(clothes.begin(), "");
      int wi = std::clamp(ui.resident_weapon, 0,
                          static_cast<int>(weapons.size()) - 1),
          oi = std::clamp(ui.resident_outfit, 0,
                          static_cast<int>(clothes.size()) - 1);
      Text("Resident · current job: " + resident->job, px + 24, py + 349, 17,
           kGold);
      Text(ActionKey(InputAction::kResidentJob) + ": job · " +
               ResidentJobs()[ui.resident_job],
           px + 24, py + 377, 16, kInk);
      Text(ActionKey(InputAction::kResidentWeapon) + ": weapon · " +
               (weapons[wi].empty() ? std::string("None")
                                    : Weapon(weapons[wi]).name),
           px + 24, py + 402, 16, kInk);
      Text(ActionKey(InputAction::kResidentOutfit) + ": clothes · " +
               (clothes[oi].empty() ? std::string("None")
                                    : Outfit(clothes[oi]).name),
           px + 24, py + 427, 16, kInk);
      Text(ActionKey(InputAction::kResidentAssign) + ": assign · " +
               ActionKey(InputAction::kProgress) + ": requirements",
           px + 24, py + 463, 16, kGold);
    } else {
      Wrapped(
          "Not your resident. Trade, quests and knowledge can still build "
          "trust.",
          px + 24, py + 363, pw - 48, 18, kMuted, 3);
    }
  }
  if (ui.crafting || ui.trading || ui.civic || ui.equipment || ui.wardrobe ||
      ui.journal) {
    auto weapons = OwnedWeapons(save);
    auto clothes = OwnedOutfits(save);
    std::vector<int> quests;
    for (auto& [id, q] : save.quests) {
      quests.push_back(id);
    }
    std::sort(quests.begin(), quests.end());
    if (quests.empty()) {
      quests.push_back(-1);
    }
    int total = ui.crafting  ? static_cast<int>(Recipes().size())
                : ui.trading ? kGoodTypes
                : ui.civic   ? static_cast<int>(CityActions().size())
                : ui.progression
                    ? static_cast<int>(Skills().size() + ResearchNodes().size())
                : ui.wardrobe ? std::max(1, static_cast<int>(clothes.size()))
                : ui.journal  ? static_cast<int>(quests.size())
                              : static_cast<int>(weapons.size());
    int page_size = MenuPageRows(), rows = std::min(total, page_size),
        page = ui.selection / page_size * page_size;
    auto box = MenuBounds(rows);
    float px = box.x, py = box.y;
    DrawRectangleRounded(box, .035f, 6, {18, 29, 28, 250});
    DrawRectangle(static_cast<int>(px), static_cast<int>(py),
                  static_cast<int>(box.width), 3, kGold);
    if (ui.civic) {
      float room_x = box.x - 110, room_y = box.y - 80;
      DrawRectangle(static_cast<int>(room_x), static_cast<int>(room_y),
                    static_cast<int>(box.width + 220),
                    static_cast<int>(box.height + 130), {73, 58, 43, 255});
      for (int i = 0; i < 12; i++) {
        DrawLineEx({room_x, room_y + i * 55},
                   {room_x + box.width + 220, room_y + i * 55}, 2,
                   {105, 82, 54, 255});
      }
      DrawRectangle(static_cast<int>(room_x + 30),
                    static_cast<int>(room_y + 20), 74, 48, {123, 77, 43, 255});
      if (!OwnsCity(save, ui.merchant)) {
        for (auto& npc : world.npcs) {
          if (npc.ruler && npc.settlement == ui.merchant) {
            Sprite(64 + npc.type * 5, {room_x + 65, room_y + 85});
          }
        }
      }
      Text(OwnsCity(save, ui.merchant)
               ? "Your town hall · the ruler's seat is vacant"
               : "Town hall · the ruler welcomes visitors",
           box.x, room_y + 22, 18, kGold);
      DrawRectangleRounded(box, .035f, 6, {18, 29, 28, 250});
    }
    std::string title = ui.crafting      ? "Crafting"
                        : ui.trading     ? "Market"
                        : ui.civic       ? "Town hall · lessons and diplomacy"
                        : ui.progression ? "Skills and resident training"
                        : ui.wardrobe    ? "Wardrobe"
                        : ui.journal     ? "Quest journal"
                                         : "Weapons";
    Wrapped(title, px + 24, py + 18, box.width - 205, 24, kGold, 1);
    Text("ESC: close", px + box.width - 157, py + 26, 16, kMuted);
    if (ui.civic) {
      auto& city = Settlements()[ui.merchant];
      auto relation = save.relations[ui.merchant];
      Text(
          city.name + " · " + RarityName(city.rarity) + " · " +
              TreatyName(relation.treaty) + " · Trust " +
              std::to_string(relation.opinion),
          px + 24, py + 59, 16,
          relation.treaty == Treaty::kWar ? Color{226, 134, 110, 255} : kMuted);
    } else if (ui.progression || ui.wardrobe || ui.journal) {
      Text("Select a row · ENTER: learn / equip / track", px + 24, py + 61, 16,
           kMuted);
    } else if (ui.crafting) {
      Text(CraftWait(save) > 0
               ? "Cooldown: " +
                     std::to_string(
                         static_cast<int>(std::ceil(CraftWait(save)))) +
                     " game seconds"
               : "Town: local / road-connected activity building required",
           px + 24, py + 61, 16, kMuted);
    } else if (ui.equipment) {
      Text("Weapon                              Damage / reach / rarity",
           px + 24, py + 61, 16, kMuted);
    } else {
      Text("Goods", px + 24, py + 61, 16, kMuted);
      Text("Owned", px + box.width * .31f, py + 61, 16, kMuted);
      Text("Stock", px + box.width * .48f, py + 61, 16, kMuted);
      Text("Buy / sell", px + box.width - 250, py + 61, 16, kMuted);
    }
    for (int row = 0; row < rows && page + row < total; row++) {
      int i = page + row;
      float y = py + kMenuListTop + row * kMenuRowHeight;
      if (ui.selection == i) {
        DrawRectangleRounded({px + 16, y - 5, box.width - 32, 47}, .12f, 4,
                             {56, 70, 52, 255});
      }
      if (ui.crafting) {
        auto& r = Recipes()[i];
        bool ready = true;
        std::string costs;
        for (int g = 0; g < kGoodTypes; g++) {
          if (r.cost[g]) {
            if (!costs.empty()) {
              costs += " · ";
            }
            costs += std::string(GoodName(g)) + " " + std::to_string(r.cost[g]);
            if (GoodCount(save.inventory, g) < r.cost[g]) {
              ready = false;
            }
          }
        }
        bool skill_locked =
            !HasSkill(save, "crafting") || !HasSkill(save, RecipeSkill(r));
        bool facility_locked =
            !FacilityAccess(save, CityAt(save, save.player), RecipeFacility(r));
        bool locked = facility_locked || CraftWait(save) > 0 ||
                      !RecipeKnown(save, r) || skill_locked ||
                      (!r.technology.empty() &&
                       !save.technologies.contains(r.technology));
        Text(r.name, px + 24, y, 17,
             Equipped(save, i) || locked ? kMuted : kInk);
        Wrapped(
            Equipped(save, i) ? "Crafted"
            : locked
                ? "Requires: " +
                      (CraftWait(save) > 0     ? std::string("cooldown")
                       : facility_locked       ? RecipeFacility(r)
                       : !RecipeKnown(save, r) ? std::string("NPC quest reward")
                       : skill_locked
                           ? SkillName(!HasSkill(save, "crafting")
                                           ? "crafting"
                                           : RecipeSkill(r))
                           : std::string(TechnologyName(r.technology)))
                : costs + " · " + RecipeFacility(r) + " · " +
                      std::to_string(static_cast<int>(CraftDelay(r))) + "s",
            px + box.width * .38f, y + 2, box.width * .62f - 24, 16,
            ready && !locked ? kGold : kMuted, 1);
      } else if (ui.progression) {
        if (i < static_cast<int>(Skills().size())) {
          Wrapped(Skills()[i].name + " · " + GetLessonState(save, i).detail,
                  px + 24, y, box.width - 48, 16,
                  GetLessonState(save, i).learned ? kGold : kInk, 1);
        } else {
          auto& node = ResearchNodes()[i - static_cast<int>(Skills().size())];
          Text(node.name + " · " +
                   (save.technologies.contains(node.id) ? "researched"
                                                        : "locked"),
               px + 24, y, 17, kInk);
          Wrapped(node.description, px + 24, y + 26, box.width - 48, 16, kMuted,
                  1);
        }
      } else if (ui.wardrobe) {
        if (clothes.empty()) {
          Text("No clothes yet", px + 24, y, 17, kMuted);
        } else {
          auto& item = Outfit(clothes[i]);
          Text(item.name + (save.outfit_id == item.id ? " · worn" : "") +
                   " · free copies " +
                   std::to_string(AvailableOutfit(save, item.id)),
               px + 24, y, 17, kInk);
        }
      } else if (ui.journal) {
        int id = quests[i];
        Text(id < 0
                 ? "Meet residents to find quests"
                 : Quest(save.quests.at(id).id).title + " · " +
                       (save.quests.at(id).completed ? "completed" : "active"),
             px + 24, y, 17, kInk);
      } else if (ui.trading) {
        Text(GoodName(i), px + 24, y, 17, kInk);
        Text(std::to_string(GoodCount(save.inventory, i)),
             px + box.width * .34f, y, 17, kInk);
        Text(std::to_string(save.stocks[ui.merchant][i]), px + box.width * .5f,
             y, 17, kMuted);
        DrawRectangleRounded({px + box.width - 254, y - 2, 102, 29}, .16f, 4,
                             {70, 88, 60, 255});
        DrawRectangleRounded({px + box.width - 130, y - 2, 102, 29}, .16f, 4,
                             {90, 76, 52, 255});
        Text("Buy " + std::to_string(BuyPrice(ui.merchant, i)),
             px + box.width - 246, y + 3, 16, kInk);
        Text("Sell " + std::to_string(SellPrice(ui.merchant, i)),
             px + box.width - 125, y + 3, 16, kInk);
      } else if (ui.civic) {
        bool lesson = i >= 10 && i < 10 + static_cast<int>(Skills().size());
        std::string label = lesson ? Skills()[i - 10].name + " · " +
                                         GetLessonState(save, i - 10).detail
                                   : CityActions()[i];
        if (i == 11 + static_cast<int>(Skills().size())) {
          int level =
              static_cast<int>((CityRadius(save, ui.merchant) - 350) / 150) + 1;
          label = "Expand territory · wood " + std::to_string(level * 15) +
                  ", stone " + std::to_string(level * 10) + ", coins " +
                  std::to_string(level * 25);
        }
        Wrapped(label, px + 24, y, box.width - 48, 16, kInk, 1);
      } else {
        Save display;
        display.inventory.weapon_id = weapons[i];
        auto& item = CurrentWeapon(display);
        Text(item.name, px + 24, y, 17,
             save.inventory.weapon_id == item.id ? kGold : kInk);
        Text(std::to_string(item.damage) + " / " +
                 std::to_string(static_cast<int>(item.reach)) + " / " +
                 RarityName(item.rarity),
             px + 300, y + 2, 16, kMuted);
      }
    }
    float bottom = py + kMenuListTop + rows * kMenuRowHeight;
    std::string detail =
        ui.crafting  ? Recipes()[ui.selection].effect
        : ui.trading ? "Coins " + std::to_string(save.coins) + " · Treasury " +
                           std::to_string(save.treasury[ui.merchant])
        : ui.civic   ? "Town technology: " +
                           std::string(TechnologyName(
                               GetContent()
                                   .towns[Settlements()[ui.merchant].type]
                                   .technology))
                     : "Choose a weapon; bows and crossbows consume arrows";
    if (ui.civic && ui.selection >= 10 &&
        ui.selection < 10 + static_cast<int>(Skills().size())) {
      detail = GetLessonState(save, ui.selection - 10).detail;
    }
    if (ui.civic && ui.selection == 10 + static_cast<int>(Skills().size()) &&
        save.campaign) {
      float y = 35;
      DrawRectangle(360, 20, GetScreenWidth() - 690, 210, {18, 29, 28, 250});
      Text("Attitude to visitors: culture, distance and character", 375, y, 14,
           kGold);
      y += 25;
      for (int city = 0;
           city < static_cast<int>(Settlements().size()) && y < 220; city++) {
        if (city != ui.merchant && save.relations[city].discovered) {
          Text(Settlements()[city].name + " · " +
                   std::to_string(save.city_opinions[ui.merchant][city]),
               375, y, 16, kInk);
          y += 23;
        }
      }
    }
    if (ui.civic && ui.selection == 7) {
      detail = "Offer: " + CityWeapon(ui.merchant, save.seed).name + " · " +
               RarityName(CityWeapon(ui.merchant, save.seed).rarity);
    }
    if (ui.progression) {
      if (ui.selection < static_cast<int>(Skills().size())) {
        detail = GetLessonState(save, ui.selection).detail;
      } else {
        auto& node =
            ResearchNodes()[ui.selection - static_cast<int>(Skills().size())];
        detail = ResearchRequirements(save, node);
      }
    }
    if (ui.wardrobe && !clothes.empty()) {
      detail = Outfit(clothes[ui.selection]).description;
    }
    if (ui.journal && quests[ui.selection] >= 0) {
      int id = quests[ui.selection];
      detail = "Giver: " + world.npcs.at(id).name + " · " +
               Settlements()[world.npcs.at(id).settlement].name +
               " · progress " +
               std::to_string(GetQuestProgress(save, save.quests.at(id))) +
               " / " + std::to_string(Quest(save.quests.at(id).id).target);
    }
    int selected_lesson =
        ui.progression && ui.selection < static_cast<int>(Skills().size())
            ? ui.selection
        : ui.civic && ui.selection >= 10 &&
                ui.selection < 10 + static_cast<int>(Skills().size())
            ? ui.selection - 10
            : -1;
    bool learned =
        selected_lesson >= 0 && GetLessonState(save, selected_lesson).learned;
    Wrapped(detail, px + 24, bottom + 8, box.width - 48, 16, kMuted, 2);
    if (selected_lesson >= 0) {
      auto state = GetLessonState(save, selected_lesson);
      float bar_width = std::min(250.f, box.width * .32f);
      DrawRectangleRounded({px + 24, bottom + 65, bar_width, 7}, 1, 12,
                           {45, 59, 45, 255});
      float fill = static_cast<float>(state.progress) / state.target;
      if (fill > 0) {
        DrawRectangleRounded({px + 24, bottom + 65, bar_width * fill, 7}, 1, 12,
                             kGold);
      }
      Text("Skills learned: " + std::to_string(save.skills.size()) + " / " +
               std::to_string(Skills().size()),
           px + bar_width + 40, bottom + 56, 16, kGold);
    }
    if (!ui.notice.empty()) {
      Wrapped(ui.notice, px + 24, bottom + 83, box.width - 48, 16, kGold, 1);
    }
    Text("Page " + std::to_string(page / page_size + 1) + " / " +
             std::to_string((total + page_size - 1) / page_size) +
             " · PGUP / PGDN",
         px + 24, bottom + 119, 16, kMuted);
    if (!ui.trading) {
      DrawRectangleRounded(
          {px + box.width - 274, bottom + 108, 250, 40}, .65f, 12,
          learned ? Color{43, 57, 47, 255} : Color{75, 91, 60, 255});
      Text(learned       ? "Learned"
           : ui.crafting ? "ENTER: craft"
           : ui.civic
               ? (selected_lesson >= 0 ? "ENTER: learn" : "ENTER: select")
           : ui.progression ? "ENTER: learn"
           : ui.journal     ? "ENTER: track"
                            : "ENTER: equip",
           px + box.width - 259, bottom + 117, 16, learned ? kMuted : kInk);
    } else {
      Text("ENTER / SHIFT+ENTER", px + box.width - 285, bottom + 119, 16, kInk);
    }
  }
  if (ui.cartography) {
    float side = std::min(w - 120, h - 160);
    Rectangle map{(w - side) * .5f, (h - side) * .5f, side, side};
    DrawRectangleRounded(
        {map.x - 28, map.y - 54, map.width + 56, map.height + 106}, .035f, 6,
        {18, 29, 28, 252});
    Text(save.submerged ? "Seafloor map" : "Map of the explored world", map.x,
         map.y - 38, 24, kGold);
    DrawMap(save, ui, camera, map, true);
    Text("Wheel: zoom · Middle drag: pan · Click: note · Tab: journal · " +
             ActionKey(InputAction::kMap) + " / Esc: close",
         map.x, map.y + map.height + 16, 16, kMuted);
  }
  if (ui.cartography) {
    DrawMapJournal(save, ui);
  }
  if (ui.progression) {
    DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h),
                  {0, 0, 0, 190});
    DrawRectangleRounded({24, 24, w - 48, h - 48}, .04f, 12, {20, 35, 28, 255});
    Text("Progress · paused", 44, 40, 24, kGold);
    auto close = ProgressCloseButton();
    DrawRectangleRounded(close, .5f, 10, {51, 76, 51, 255});
    Text("Close · Esc", close.x + 10, close.y + 8, 16, kInk);
    DrawSummary(save, ui.summary, ProgressSummaryBounds(), true, &world.npcs);
    Wrapped(ActionKey(InputAction::kProgress) +
                " / Esc: close · arrows: tabs · up/down: tree or towns · "
                "Enter: action",
            44, h - 53, w - 88, 16, kMuted, 1);
  }
  if (ui.paused && !ui.settings_open) {
    DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h),
                  {0, 0, 0, 185});
    auto box = PauseBounds();
    DrawRectangleRounded(box, .06f, 12, {22, 35, 31, 255});
    Text("World paused", box.x + 24, box.y + 22, 26, kGold);
    const char* actions[] = {"Resume · Esc", "Save world",
                             "Save & return to main menu", "Settings"};
    for (int i = 0; i < 4; ++i) {
      auto button = PauseButton(i);
      bool hover = CheckCollisionPointRec(GetMousePosition(), button);
      DrawRectangleRounded(button, .6f, 12,
                           ui.pause_selection == i || hover
                               ? Color{81, 99, 64, 255}
                               : Color{36, 51, 44, 255});
      Wrapped(actions[i], button.x + 18, button.y + 12, button.width - 36, 18,
              kInk, 1);
    }
    Wrapped(ui.pause_error.empty()
                ? (ui.temporary_session ? "Temporary world; saving is disabled."
                                        : "Save writes this world. Return "
                                          "saves before leaving. Esc resumes.")
                : ui.pause_error,
            box.x + 24, box.y + box.height - 90, box.width - 48, 17, kGold, 3);
  }
}
}  // namespace frontier
