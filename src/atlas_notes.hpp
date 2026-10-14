#ifndef FOREST_FRONTIER_SRC_ATLAS_NOTES_HPP_
#define FOREST_FRONTIER_SRC_ATLAS_NOTES_HPP_
#include <string>

#include "world.hpp"
namespace frontier {
float SightRange(const Save& save);
bool VisibleAt(const Save& save, Vec position, bool underwater);
int StoreMapNote(Save& save, int index, Vec position, bool underwater,
                 const std::string& title, const std::string& text);
bool DeleteMapNote(Save& save, int index);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_ATLAS_NOTES_HPP_
