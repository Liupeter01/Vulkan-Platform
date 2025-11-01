#include <bvh/Bounds3.hpp>

namespace engine {
Bounds3::Bounds3()
    : min(std::numeric_limits<float>::infinity()),
      max(std::numeric_limits<float>::infinity()) {}

Bounds3::Bounds3(const glm::vec3 &p) : Bounds3(p, p) {}

Bounds3::Bounds3(const glm::vec3 &min, const glm::vec3 &max)
    : min(glm::vec3(std::fmin(min.x, max.x), std::fmin(min.y, max.y),
                    std::fmin(min.z, max.z))),
      max(glm::vec3(std::fmax(min.x, max.x), std::fmax(min.y, max.y),
                    std::fmax(min.z, max.z))) {}

glm::vec3 Bounds3::diagonal() const { return max - min; }

glm::vec3 Bounds3::centroid() const { return 0.5f * (min + max); }

bool Bounds3::overlaps(const Bounds3 &box) const {
  return overlaps(*this, box);
}

Bounds3 Bounds3::intersect(const Bounds3 &b) const {
  return Bounds3::intersect(*this, b);
}

Bounds3::ExtentType Bounds3::maxExtent() const {
  auto d = diagonal();
  if (d.x > d.y && d.x > d.z)
    return ExtentType::X; // x
  else if (d.y > d.z)
    return ExtentType::Y; // y

  return ExtentType::Z; // z
}

double Bounds3::surfaceArea() const {
  auto d = diagonal();
  return 2.f * (d.x * d.y + d.x * d.z + d.y * d.z);
}

bool Bounds3::inside(const glm::vec3 &point) const {
  return Bounds3::inside(*this, point);
}

Bounds3 Bounds3::BoundsUnion(const Bounds3 &box) const {
  return Bounds3::BoundsUnion(*this, box);
}

Bounds3 Bounds3::BoundsUnion(const glm::vec3 &point) const {
  return Bounds3::BoundsUnion(*this, point);
}

Bounds3 Bounds3::intersect(const Bounds3 &a, const Bounds3 &b) {

  const glm::vec3 min{std::fmax(a.min.x, b.min.x), std::fmax(a.min.y, b.min.y),
                      std::fmax(a.min.z, b.min.z)};

  const glm::vec3 max{std::fmax(a.max.x, b.max.x), std::fmax(a.max.y, b.max.y),
                      std::fmax(a.max.z, b.max.z)};

  return {min, max};
}

bool Bounds3::overlaps(const Bounds3 &box1, const Bounds3 &box2) {

  return box1.max.x >= box2.min.x && box1.min.x <= box2.max.x &&
         box1.max.y >= box2.min.y && box1.min.y <= box2.max.y &&
         box1.max.z >= box2.min.z && box1.min.z <= box2.max.z;
}

bool Bounds3::inside(const Bounds3 &box, const glm::vec3 &point) {
  return point.x >= box.min.x && point.x <= box.max.x && point.y >= box.min.y &&
         point.y <= box.max.y && point.z >= box.min.z && point.z <= box.max.z;
}

Bounds3 Bounds3::BoundsUnion(const Bounds3 &box1, const Bounds3 &box2) {
  return {glm::min(box1.min, box2.min), glm::max(box1.max, box2.max)};
}

Bounds3 Bounds3::BoundsUnion(const Bounds3 &box, const glm::vec3 &point) {
  return Bounds3::BoundsUnion(box, {point});
}
} // namespace engine
