#pragma once
#ifndef _MESH_BUFFERS_HPP_
#define _MESH_BUFFERS_HPP_
#include <AllocatedBuffer.hpp>
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

namespace mesh {
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

namespace v1 {

          struct GPUGeoMeshBuffers {
                    GPUGeoMeshBuffers(VkDevice device, VmaAllocator allocator);
                    virtual ~GPUGeoMeshBuffers();

                    AllocatedBuffer indexBuffer;
                    AllocatedBuffer vertexBuffer;

                    std::vector<Vertex> vertex_;
                    std::vector<uint32_t> indicies_;
                    VkDeviceAddress vertexBufferAddress{};

                    void destroy();
                    void invalid();
                    bool isValid() const;
                    void createMesh(std::vector<Vertex>&& vertices,
                              std::vector<uint32_t>&& indices);

                    void createMesh(const Mesh& mesh);
                    void submitMesh(VkCommandBuffer cmd);
                    void flushUpload(VkFence fence);

          private:
                    bool isinit = false;
                    VkDevice device_;
                    VmaAllocator allocator_;

                    bool pendingUpload_ = false;
                    AllocatedBuffer staging;
          };
}

namespace v2 {
struct GPUGeoMeshBuffers2 {
  GPUGeoMeshBuffers2(VkDevice device, VmaAllocator allocator);
  virtual ~GPUGeoMeshBuffers2();

  ::engine::v2::AllocatedBuffer2 vertexBuffer_;
  ::engine::v2::AllocatedBuffer2 indexBuffer_;

  std::vector<Vertex> vertex_;
  std::vector<uint32_t> indicies_;

  void destroy();
  void createMesh(std::vector<Vertex> &&vertices,
                  std::vector<uint32_t> &&indices);

  void createMesh(const Mesh &mesh);
  VkDeviceAddress getVertexBufferDeviceAddress();

  void recordUpload(VkCommandBuffer cmd);
  void setUploadCompleteTimeline(uint64_t value);
  void purgeReleaseStaging(uint64_t observedValue);
  void updateUploadingStatus(uint64_t observedValue);

  ::engine::v2::AllocatedBuffer2& getVertexBuffer();
  ::engine::v2::AllocatedBuffer2& getIndexBuffer();

private:
  bool isinit_ = false;
  VkDevice device_;
  VmaAllocator allocator_;
};

} // namespace v2

} // namespace mesh
} // namespace engine

#endif //_MESH_BUFFERS_HPP_
