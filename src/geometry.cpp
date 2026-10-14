#include "geometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
namespace frontier {
float SegmentDistance(Vec p, Vec a, Vec b) {
  Vec d = b - a, offset = p - a;
  float square = d.x * d.x + d.y * d.y;
  float t = square > 0 ? std::clamp((offset.x * d.x + offset.y * d.y) / square,
                                    0.f, 1.f)
                       : 0;
  return Length(p - (a + d * t));
}
bool CircleBox(Vec p, float r, BoxShape box) {
  Vec closest{std::clamp(p.x, box.low.x, box.high.x),
              std::clamp(p.y, box.low.y, box.high.y)};
  return Length(p - closest) < r - .0001f;
}
bool CircleCapsule(Vec p, float r, CapsuleShape s) {
  return SegmentDistance(p, s.a, s.b) < r + s.radius - .0001f;
}
bool CirclePolygon(Vec p, float r, std::span<const Vec> points) {
  if (points.size() < 3) {
    return false;
  }
  bool inside = false;
  for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
    auto a = points[j], b = points[i];
    if (SegmentDistance(p, a, b) < r - .0001f) {
      return true;
    }
    if ((a.y > p.y) != (b.y > p.y) &&
        p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x) {
      inside = !inside;
    }
  }
  return inside;
}
float SegmentsDistance(Vec a, Vec b, Vec c, Vec d) {
  auto cross = [](Vec u, Vec v) { return u.x * v.y - u.y * v.x; };
  Vec r = b - a, s = d - c;
  float denom = cross(r, s);
  if (std::abs(denom) > .00001f) {
    float t = cross(c - a, s) / denom, u = cross(c - a, r) / denom;
    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
      return 0;
    }
  }
  return std::min({SegmentDistance(a, c, d), SegmentDistance(b, c, d),
                   SegmentDistance(c, a, b), SegmentDistance(d, a, b)});
}
bool SegmentBox(Vec a, Vec b, BoxShape box) {
  float low = 0, high = 1;
  Vec d = b - a;
  for (int axis = 0; axis < 2; axis++) {
    float p = axis ? a.y : a.x, v = axis ? d.y : d.x,
          mn = axis ? box.low.y : box.low.x,
          mx = axis ? box.high.y : box.high.x;
    if (std::abs(v) < .00001f) {
      if (p < mn || p > mx) {
        return false;
      }
    } else {
      float x = (mn - p) / v, y = (mx - p) / v;
      if (x > y) {
        std::swap(x, y);
      }
      low = std::max(low, x);
      high = std::min(high, y);
      if (low > high) {
        return false;
      }
    }
  }
  return high > .01f && low < .99f;
}
bool SegmentPolygon(Vec a, Vec b, std::span<const Vec> vertices) {
  if (CirclePolygon(a, 0, vertices) || CirclePolygon(b, 0, vertices)) {
    return true;
  }
  for (size_t i = 0; i < vertices.size(); i++) {
    if (SegmentsDistance(a, b, vertices[i],
                         vertices[(i + 1) % vertices.size()]) < .0001f) {
      return true;
    }
  }
  return false;
}
std::array<Vec, 4> BuildingOutline(const Building& b) {
  // Invert the atlas plinth's actual screen-space corners, relative to its
  // (64,144) sprite anchor. Drawing and contacts use exactly the same
  // footprint.
  return {b.pos + Unproject({-46, -21}), b.pos + Unproject({0, -44}),
          b.pos + Unproject({46, -21}), b.pos + Unproject({0, 2})};
}
BoxShape BuildingFootprint(const Building& b) {
  auto corners = BuildingOutline(b);
  BoxShape box{corners[0], corners[0]};
  for (auto p : corners) {
    box.low.x = std::min(box.low.x, p.x);
    box.low.y = std::min(box.low.y, p.y);
    box.high.x = std::max(box.high.x, p.x);
    box.high.y = std::max(box.high.y, p.y);
  }
  return box;
}
}  // namespace frontier
