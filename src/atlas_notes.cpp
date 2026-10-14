#include "atlas_notes.hpp"

#include <cmath>
#include <stdexcept>

#include "gameplay.hpp"
#include "settlement_system.hpp"
#include "vessels.hpp"
namespace frontier {
float SightRange(const Save& save) {
  if (save.submerged) {
    return 650;
  }
  auto terrain = TerrainAt(save.player, save.seed);
  if (BridgeAt(save, save.player)) {
    return 850;
  }
  if (terrain == Terrain::kDeep || terrain == Terrain::kShallow) {
    return ActiveVehicle(save) > 0 ? Vessel(ActiveVehicle(save)).view : 650;
  }
  return terrain == Terrain::kForest ? 650 : 850;
}
bool VisibleAt(const Save& save, Vec position, bool underwater) {
  const auto& cells =
      underwater ? save.visible_underwater : save.visible_surface;
  return cells.contains(MapCell(position));
}
int StoreMapNote(Save& save, int index, Vec position, bool underwater,
                 const std::string& title, const std::string& text) {
  const auto& cells = underwater ? save.explored_underwater : save.explored;
  if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
      Length(position) > 2e6f || !ExploredAt(cells, position) ||
      title.empty() || title.size() > 64 || text.size() > 2048) {
    throw std::runtime_error(
        "Choose an explored location and a title (64 characters max).");
  }
  if (index < -1 || index >= static_cast<int>(save.map_notes.size())) {
    throw std::runtime_error("Invalid journal entry");
  }
  if (index < 0) {
    if (save.map_notes.size() >= 128) {
      throw std::runtime_error("Journal limit: 128 notes");
    }
    save.map_notes.push_back({position, underwater, title, text});
    return static_cast<int>(save.map_notes.size()) - 1;
  }
  save.map_notes[index] = {position, underwater, title, text};
  return index;
}
bool DeleteMapNote(Save& save, int index) {
  if (index < 0 || index >= static_cast<int>(save.map_notes.size())) {
    return false;
  }
  save.map_notes.erase(save.map_notes.begin() + index);
  return true;
}
}  // namespace frontier
