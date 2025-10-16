#pragma once
#ifndef _MESH_BUFFERS_HPP_
#define _MESH_BUFFERS_HPP_
#include <GlobalDef.hpp>
#include <vector>

namespace engine {
struct Vertex {
  glm::vec3 position;
  float uv_x;
  glm::vec3 normal;
  float uv_y;
  glm::vec4 color;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

inline namespace mesh {
struct GPUGeoMeshBuffers {
  GPUGeoMeshBuffers(VkDevice device, VmaAllocator allocator);
  virtual ~GPUGeoMeshBuffers();

  AllocatedBuffer indexBuffer;
  AllocatedBuffer vertexBuffer;

  std::vector<Vertex> vertex_;
  std::vector<uint32_t> indicies_;
  VkDeviceAddress vertexBufferAddress{};

  void createMesh(std::vector<Vertex> &&vertices,
                  std::vector<uint32_t> &&indices);

  void createMesh(const Mesh &mesh);
  void submitMesh(VkCommandBuffer cmd);
  void flushUpload(VkFence fence);

private:
  bool isinit = false;
  VkDevice device_;
  VmaAllocator allocator_;

  bool pendingUpload_ = false;
  AllocatedBuffer staging;
};

struct GPUGeoPushConstants {
  glm::mat4 matrix;
  VkDeviceAddress vertexBuffer;
};

struct GPUSceneData {
  glm::mat4 view{1.f};
  glm::mat4 proj{1.f};
  glm::vec4 ambientColor{0.1f};
  glm::vec4 sunlightDirection{0, 0, 1, 5}; // w for sun power
  glm::vec4 sunlightColor{1, 1, 1, 1};
};

} // namespace mesh
} // namespace engine

#endif //_MESH_BUFFERS_HPP_
