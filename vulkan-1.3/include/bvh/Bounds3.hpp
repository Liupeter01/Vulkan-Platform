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
  enum ExtentType { X, Y, Z };
  Bounds3();
  Bounds3(const glm::vec3 &p);
  Bounds3(const glm::vec3 &min, const glm::vec3 &max);

  glm::vec3 diagonal() const;                                      
  glm::vec3 centroid() const;
  ExtentType maxExtent()const;
  double surfaceArea()const;
  [[nodiscard]] Bounds3 intersect(const Bounds3 &b)const;
  bool overlaps(const Bounds3& box)const;
  bool inside(const glm::vec3 &point)const;
  [[nodiscard]] Bounds3 BoundsUnion(const Bounds3& box) const;
  [[nodiscard]] Bounds3 BoundsUnion(const glm::vec3& point) const;

  [[nodiscard]] static Bounds3 intersect(const Bounds3& a, const Bounds3& b);
  static bool overlaps(const Bounds3& box1, const Bounds3& box2);
  static bool inside(const Bounds3& box, const glm::vec3& point);
  [[nodiscard]] static Bounds3 BoundsUnion(const Bounds3& box1, const Bounds3& box2) ;
  [[nodiscard]] static Bounds3 BoundsUnion(const Bounds3& box, const glm::vec3& point) ;

  // two points to specify the bounding box
  glm::vec3 min{};
  glm::vec3 max{};
};

} // namespace engine

#endif //_BOUNDS3_HPP_
