#pragma once
#ifndef _BOUNDS3_HPP_
#define _BOUNDS3_HPP_
#include <cmath>
#include <limits>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct Bounds3 {
  enum ExtentType { 
            X, Y, Z
  };

  enum class CornerType : uint8_t {
            LEFT_BOTTOM_FRONT = 0,  // (min.x, min.y, min.z)
            RIGHT_BOTTOM_FRONT = 1, // (max.x, min.y, min.z)
            LEFT_TOP_FRONT = 2,     // (min.x, max.y, min.z)
            RIGHT_TOP_FRONT = 3,    // (max.x, max.y, min.z)
            LEFT_BOTTOM_BACK = 4,   // (min.x, min.y, max.z)
            RIGHT_BOTTOM_BACK = 5,  // (max.x, min.y, max.z)
            LEFT_TOP_BACK = 6,      // (min.x, max.y, max.z)
            RIGHT_TOP_BACK = 7      // (max.x, max.y, max.z)
  };

  Bounds3();
  Bounds3(const glm::vec3 &p);
  Bounds3(const glm::vec3 &min, const glm::vec3 &max);

  glm::vec3 diagonal() const;
  glm::vec3 centroid() const;
  ExtentType maxExtent() const;

  glm::vec3 getCorner(const CornerType type);

  float sphereRadius(bool acc = false) const;
  float __sphere_radius_acc() const;
  float __sphere_radius_fast() const;

  double surfaceArea() const;
  [[nodiscard]] Bounds3 intersect(const Bounds3 &b) const;
  bool overlaps(const Bounds3 &box) const;
  bool inside(const glm::vec3 &point) const;
  [[nodiscard]] Bounds3 BoundsUnion(const Bounds3 &box) const;
  [[nodiscard]] Bounds3 BoundsUnion(const glm::vec3 &point) const;

  [[nodiscard]] static Bounds3 intersect(const Bounds3 &a, const Bounds3 &b);
  static bool overlaps(const Bounds3 &box1, const Bounds3 &box2);
  static bool inside(const Bounds3 &box, const glm::vec3 &point);
  [[nodiscard]] static Bounds3 BoundsUnion(const Bounds3 &box1,
                                           const Bounds3 &box2);
  [[nodiscard]] static Bounds3 BoundsUnion(const Bounds3 &box,
                                           const glm::vec3 &point);

  [[nodiscard]] static glm::vec3 getCorner(const Bounds3& box, const CornerType type);
  // two points to specify the bounding box
  glm::vec3 min{};
  glm::vec3 max{};
};

} // namespace engine

#endif //_BOUNDS3_HPP_
