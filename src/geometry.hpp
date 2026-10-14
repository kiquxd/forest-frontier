#ifndef FOREST_FRONTIER_SRC_GEOMETRY_HPP_
#define FOREST_FRONTIER_SRC_GEOMETRY_HPP_
#include <array>
#include <span>

#include "world.hpp"
namespace frontier {
struct BoxShape {
  Vec low;
  Vec high;
};
struct CapsuleShape {
  Vec a;
  Vec b;
  float radius;
};
float SegmentDistance(Vec point, Vec a, Vec b);
bool CircleBox(Vec center, float radius, BoxShape box);
bool CircleCapsule(Vec center, float radius, CapsuleShape capsule);
bool CirclePolygon(Vec center, float radius, std::span<const Vec> vertices);
float SegmentsDistance(Vec a, Vec b, Vec c, Vec d);
bool SegmentBox(Vec a, Vec b, BoxShape box);
bool SegmentPolygon(Vec a, Vec b, std::span<const Vec> vertices);
std::array<Vec, 4> BuildingOutline(const Building& building);
BoxShape BuildingFootprint(const Building& building);
}  // namespace frontier
#endif  // FOREST_FRONTIER_SRC_GEOMETRY_HPP_
