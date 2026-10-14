#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "atlas_notes.hpp"
#include "gameplay.hpp"
#include "render.hpp"

namespace frontier {
namespace {
Color LandColor(Vec position, const Save& save, bool submerged) {
  auto terrain = TerrainAt(position, save.seed);
  if (submerged) {
    if (terrain != Terrain::kDeep && terrain != Terrain::kShallow) {
      return {16, 24, 27, 255};
    }
    return terrain == Terrain::kDeep ? Color{48, 93, 106, 255}
                                     : Color{83, 132, 130, 255};
  }
  if (terrain == Terrain::kDeep) {
    return {43, 100, 134, 255};
  }
  if (terrain == Terrain::kShallow) {
    return {72, 146, 160, 255};
  }
  if (terrain == Terrain::kSand) {
    return {189, 168, 113, 255};
  }
  if (PathAt(position)) {
    return {154, 127, 81, 255};
  }
  return {65, 105, 65, 255};
}
}  // namespace

void Renderer::DrawMap(const Save& save, const Ui& ui, const Camera2D&,
                       Rectangle box, bool large, bool show_viewport) {
  const bool submerged = ui.map_layer_override < 0
                             ? save.submerged
                             : static_cast<bool>(ui.map_layer_override);
  auto& cache = maps_[large ? 1 : 0];
  Vec desired = large ? ui.map_center
                      : Vec{std::floor(save.player.x / 384) * 384,
                            std::floor(save.player.y / 384) * 384};
  float range = large ? ui.map_range : 2100;
  const int kPixels = large ? (range < 1800 ? 768 : 512) : 256;
  if (cache.seed != save.seed || cache.refresh < 0 ||
      ui.time - cache.refresh > .3 || cache.submerged != submerged ||
      cache.range != range || cache.pixels != kPixels ||
      cache.overlay != ui.map_overlay || Length(cache.center - desired) > 1) {
    if (cache.pixels != kPixels && cache.texture.id) {
      UnloadTexture(cache.texture);
      cache.texture = {};
    }
    cache.pixels = kPixels;
    cache.overlay = ui.map_overlay;
    cache.seed = save.seed;
    cache.center = desired;
    cache.range = range;
    cache.refresh = ui.time;
    cache.submerged = submerged;
    Image image = GenImageColor(kPixels, kPixels, {11, 19, 22, 255});
    auto& explored = submerged ? save.explored_underwater : save.explored;
    auto point = [&](Vec position) {
      return Vector2{
          (position.x - cache.center.x + range) / (2 * range) * kPixels,
          (position.y - cache.center.y + range) / (2 * range) * kPixels};
    };
    if (large) {
      constexpr int block = 4;
      for (int y = 0; y < kPixels; y += block) {
        for (int x = 0; x < kPixels; x += block) {
          Vec position{
              cache.center.x - range + (x + block * .5f) / kPixels * range * 2,
              cache.center.y - range + (y + block * .5f) / kPixels * range * 2};
          if (ExploredAt(explored, position)) {
            ImageDrawRectangle(&image, x, y, block, block,
                               LandColor(position, save, submerged));
          }
        }
      }
    }
    auto low = MapCell(cache.center - Vec{range, range}),
         high = MapCell(cache.center + Vec{range, range});
    for (int y = low.y; y <= high.y; y++) {
      for (int x = low.x; x <= high.x; x++) {
        Coord coord{x, y};
        Vec world{(x + .5f) * kMapCellSize, (y + .5f) * kMapCellSize};
        if (!explored.contains(coord)) {
          continue;
        }
        auto a = point({x * kMapCellSize, y * kMapCellSize}),
             b = point({(x + 1) * kMapCellSize, (y + 1) * kMapCellSize});
        Color color = LandColor(world, save, submerged);
        int left = static_cast<int>(std::floor(a.x)),
            top = static_cast<int>(std::floor(a.y)),
            right = static_cast<int>(std::ceil(b.x)),
            bottom = static_cast<int>(std::ceil(b.y));
        if (!large) {
          ImageDrawRectangle(&image, left, top, right - left, bottom - top,
                             color);
        }
        auto terrain = TerrainAt(world, save.seed);
        if (!submerged && terrain == Terrain::kForest && !PathAt(world) &&
            ObjectId(x, y, save.seed) % 5 == 0 && right - left >= 4) {
          int cx = (left + right) / 2, cy = (top + bottom) / 2;
          ImageDrawTriangle(
              &image, {static_cast<float>(cx), static_cast<float>(cy - 2)},
              {static_cast<float>(cx - 2), static_cast<float>(cy + 2)},
              {static_cast<float>(cx + 2), static_cast<float>(cy + 2)},
              {41, 74, 48, 255});
        }
        // Neighboring cells share edges; shore boundaries follow the actual
        // terrain.
        if (!large &&
            (terrain == Terrain::kDeep || terrain == Terrain::kShallow)) {
          auto above = TerrainAt(world - Vec{0, kMapCellSize}, save.seed),
               side = TerrainAt(world - Vec{kMapCellSize, 0}, save.seed);
          if (above == Terrain::kForest || above == Terrain::kSand) {
            ImageDrawLine(&image, left, top, right, top, {220, 203, 154, 255});
          }
          if (side == Terrain::kForest || side == Terrain::kSand) {
            ImageDrawLine(&image, left, top, left, bottom,
                          {220, 203, 154, 255});
          }
        }
      }
    }
    if (!submerged) {
      ImageDrawLineEx(&image, point(Settlements()[0].pos),
                      point(Settlements()[1].pos),
                      std::max(1, static_cast<int>(64 / (range * 2) * kPixels)),
                      {154, 127, 81, 255});
    }
    if (!submerged) {
      for (size_t city = 0; city < Settlements().size(); city++) {
        auto& town = Settlements()[city];
        auto center = point(town.pos);
        if (center.x < -25 || center.y < -25 || center.x > kPixels + 25 ||
            center.y > kPixels + 25) {
          continue;
        }
        auto& type = GetContent().towns[town.type];
        Color roof{static_cast<uint8_t>(type.color[0]),
                   static_cast<uint8_t>(type.color[1]),
                   static_cast<uint8_t>(type.color[2]), 255};
        float unit = std::max(2.f, 190.f / (range * 2) * kPixels);
        ImageDrawRectangle(&image, static_cast<int>(center.x - unit * 2.5f),
                           static_cast<int>(center.y - 1),
                           static_cast<int>(unit * 5), 2, {181, 151, 101, 255});
        for (int i = 0; i < 5 + town.rarity; i++) {
          float angle = i * 6.2831853f / (5 + town.rarity);
          int x = static_cast<int>(center.x + std::cos(angle) * unit * 2),
              y = static_cast<int>(center.y + std::sin(angle) * unit * 2);
          ImageDrawRectangle(&image, x - 2, y, 5, 4, {202, 179, 137, 255});
          ImageDrawTriangle(
              &image, {static_cast<float>(x - 3), static_cast<float>(y)},
              {static_cast<float>(x + 2), static_cast<float>(y - 3)},
              {static_cast<float>(x + 5), static_cast<float>(y)}, roof);
        }
        ImageDrawRectangle(
            &image, static_cast<int>(center.x) - unit,
            static_cast<int>(center.y) - unit, unit * 2, unit * 2,
            OwnsCity(save, static_cast<int>(city)) ? Color{224, 204, 131, 255}
                                                   : roof);
        for (auto& wall : save.walls) {
          if (wall.city == static_cast<int>(city) && wall.hp > 0) {
            ImageDrawLineV(&image, point(wall.a), point(wall.b),
                           {207, 193, 153, 255});
          }
        }
      }
    }
    // Apply fog last, so icons, walls and lakes cannot disclose unexplored
    // pieces.
    for (int y = 0; y < kPixels; y++) {
      for (int x = 0; x < kPixels; x++) {
        Vec world{cache.center.x - range + (x + .5f) / kPixels * range * 2,
                  cache.center.y - range + (y + .5f) / kPixels * range * 2};
        if (!ExploredAt(explored, world)) {
          ImageDrawPixel(&image, x, y, {11, 19, 22, 255});
        } else {
          bool current = show_viewport && VisibleAt(save, world, submerged);
          if (!current) {
            auto* colors = static_cast<Color*>(image.data);
            auto& color = colors[y * kPixels + x];
            color.r = static_cast<uint8_t>(color.r * .64f);
            color.g = static_cast<uint8_t>(color.g * .64f);
            color.b = static_cast<uint8_t>(color.b * .64f);
          }
        }
      }
    }
    if (cache.texture.id) {
      UpdateTexture(cache.texture, image.data);
    } else {
      cache.texture = LoadTextureFromImage(image);
      SetTextureFilter(cache.texture, TEXTURE_FILTER_POINT);
    }
    UnloadImage(image);
  }
  auto map_pos = [&](Vec position) {
    return Vector2{box.x + (position.x - cache.center.x + cache.range) /
                               (2 * cache.range) * box.width,
                   box.y + (position.y - cache.center.y + cache.range) /
                               (2 * cache.range) * box.height};
  };
  DrawTexturePro(cache.texture,
                 {0, 0, static_cast<float>(cache.pixels),
                  static_cast<float>(cache.pixels)},
                 box, {0, 0}, 0, WHITE);
  BeginScissorMode(static_cast<int>(box.x), static_cast<int>(box.y),
                   static_cast<int>(box.width), static_cast<int>(box.height));
  auto player = map_pos(save.player);
  DrawCircleV(player, 4, {248, 221, 141, 255});
  DrawCircleLines(static_cast<int>(player.x), static_cast<int>(player.y), 6,
                  {30, 39, 31, 255});
  if (submerged) {
    DrawCircleLines(static_cast<int>(player.x), static_cast<int>(player.y),
                    650 / (2 * cache.range) * box.width, {123, 208, 216, 180});
  }
  if (large && !submerged) {
    for (size_t i = 0; i < Settlements().size(); i++) {
      if (ExploredAt(save.explored, Settlements()[i].pos)) {
        auto point = map_pos(Settlements()[i].pos);
        Color color =
            ui.map_overlay == 1
                ? (OwnsCity(save, i)               ? Color{166, 214, 124, 255}
                   : save.relations[i].opinion < 0 ? Color{229, 133, 116, 255}
                                                   : Color{236, 207, 143, 255})
                : Color{235, 227, 195, 255};
        std::string label = Settlements()[i].name;
        if (ui.map_overlay == 1) {
          label +=
              " · " + std::string(OwnsCity(save, i)
                                      ? "yours"
                                      : TreatyName(save.relations[i].treaty));
        }
        if (ui.map_overlay == 2) {
          const auto& type = GetContent().towns[Settlements()[i].type];
          label += " · " + type.name;
          for (int good : type.goods) {
            label += " / " + std::string(GoodName(good));
          }
        }
        DrawCircleV(point, 5, color);
        Wrapped(label, point.x + 10, point.y + 12, 270, 16, color, 2);
      }
    }
  }
  if (large && ui.map_overlay == 2) {
    for (const auto& [id, site] : save.known_deposits) {
      if (site.underwater != submerged || save.harvested.contains(id)) {
        continue;
      }
      auto p = map_pos(site.position);
      Icon(save.known_goods[site.good] ? "good_" + std::to_string(site.good)
                                       : "good_1",
           {p.x - 8, p.y - 8, 16, 16});
    }
  }
  for (size_t i = 0; i < save.map_notes.size(); i++) {
    const auto& note = save.map_notes[i];
    if (note.underwater != submerged) {
      continue;
    }
    auto p = map_pos(note.position);
    DrawCircleV(p, 8, {221, 173, 92, 255});
    DrawCircleV(p, 3, {48, 59, 42, 255});
    if (large) {
      Wrapped(note.title, p.x + 12, p.y - 12, 190, 16, {240, 210, 151, 255}, 1);
    }
  }
  EndScissorMode();
  DrawRectangleLinesEx(box, 1, {146, 169, 145, 255});
}
void Renderer::DrawSavedMap(const Save& save, Rectangle box, int layer) {
  const bool submerged = layer < 0 ? save.submerged : static_cast<bool>(layer);
  const auto& cells = submerged ? save.explored_underwater : save.explored;
  Vec low = save.player, high = save.player;
  for (auto cell : cells) {
    Vec position{cell.x * kMapCellSize, cell.y * kMapCellSize};
    low.x = std::min(low.x, position.x);
    low.y = std::min(low.y, position.y);
    high.x = std::max(high.x, position.x + kMapCellSize);
    high.y = std::max(high.y, position.y + kMapCellSize);
  }
  Ui ui;
  ui.map_layer_override = layer;
  ui.map_center = (low + high) * .5f;
  ui.map_range =
      std::max(700.f, std::max(high.x - low.x, high.y - low.y) * .55f);
  ui.time = GetTime();
  Camera2D camera{};
  camera.zoom = 1;
  DrawMap(save, ui, camera, box, true, false);
}

}  // namespace frontier
