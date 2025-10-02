#pragma once
#ifndef _MODEL_HPP_
#define _MODEL_HPP_
#include <Buffer.hpp>
#include <Tools.hpp>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace engine {
struct Vertex {
  glm::vec3 position{};
  glm::vec3 color{};
  glm::vec3 normal{};
  glm::vec2 uv{};

  [[nodiscard]] static std::vector<VkVertexInputBindingDescription>
  getBindingDescriptions();
  [[nodiscard]] static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions();

  bool operator==(const Vertex &o) const {
    return position == o.position && color == o.color && normal == o.normal &&
           uv == o.uv;
  }
};

struct Builder {
  std::vector<Vertex> vertices{};
  std::vector<uint32_t> indices{};

  void loadObjModel(const std::string &path);
};

struct Model {
  Model(Model &&) = delete;
  Model &operator=(Model &&) = delete;
  Model(MyEngineDevice &device, const Builder &builder);
  Model(MyEngineDevice &device, const std::string &path);
  virtual ~Model() {}

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

protected:
  void createVertexBuffers(const std::vector<Vertex> &vertices);
  void createIndicesBuffers(const std::vector<uint32_t> &indices);

private:
  bool hasIndicesArgument_ = false;
  MyEngineDevice &device_;

  uint32_t vertexCounter_;
  std::unique_ptr<Buffer> vertexBuffer_;

  uint32_t indicesCounter_;
  std::unique_ptr<Buffer> indexBuffer_;
};
} // namespace engine

namespace std {

template <> struct std::hash<engine::Vertex> {
  size_t operator()(const engine::Vertex &vertex) const {
    // Combine the hashes of position, color, normal, and texCoord

    size_t seed = 0;
    std::hash<glm::vec3> vec3Hasher{};
    std::hash<glm::vec2> vec2Hasher{};

    engine::tools::hash_combine_impl(seed, vec3Hasher(vertex.position));
    engine::tools::hash_combine_impl(seed, vec3Hasher(vertex.normal));
    engine::tools::hash_combine_impl(seed, vec3Hasher(vertex.color));
    engine::tools::hash_combine_impl(seed, vec2Hasher(vertex.uv));

    // Combine all hashes using bit manipulation
    return seed;
  }
};
}; // namespace std

#endif //_MODEL_HPP_
