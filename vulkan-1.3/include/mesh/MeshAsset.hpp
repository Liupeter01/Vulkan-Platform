#pragma once
#ifndef _MESH_ASSET_HPP_
#define _MESH_ASSET_HPP_
#include <bvh/Bounds3.hpp>
#include <filesystem>
#include <material/GLTFPBR_Material.hpp>
#include <mesh/MeshBuffers.hpp>
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace engine {
namespace mesh {

struct GeoSurface {
  uint32_t startIndex{};
  uint32_t count{};
  Bounds3 bounds{};
  std::shared_ptr<MaterialInstance> material{};
};

struct MeshAsset {
  MeshAsset(VkDevice device, VmaAllocator allocator)
      : meshBuffers{device, allocator} {}

  void createAsset(std::vector<Vertex> &&vertices,
                   std::vector<uint32_t> &&indices) {
    meshBuffers.createMesh(std::move(vertices), std::move(indices));
  }

  void submitMesh(VkCommandBuffer cmd) { meshBuffers.submitMesh(cmd); }
  void flushUpload(VkFence fence) { meshBuffers.flushUpload(fence); }

  VkDeviceAddress getVertexDeviceAddr() {
    return meshBuffers.vertexBufferAddress;
  }

  std::string meshName;
  std::vector<GeoSurface> meshSurfaces;
  GPUGeoMeshBuffers meshBuffers;
};

namespace v1 {}

namespace v2 {
          struct MeshAsset2 {
                    MeshAsset2(VkDevice device, VmaAllocator allocator)
                              : meshBuffers{ device, allocator } {
                    }

                    void createAsset(std::vector<Vertex>&& vertices,
                              std::vector<uint32_t>&& indices) {
                              meshBuffers.createMesh(std::move(vertices), std::move(indices));
                    }

                    void recordUpload(VkCommandBuffer cmd) {
                              meshBuffers.recordUpload(cmd);
                    }

                    void setUploadCompleteTimeline(uint64_t value) {
                              meshBuffers.setUploadCompleteTimeline(value);
                    }

                    void purgeReleaseStaging(uint64_t observedValue) {
                              meshBuffers.purgeReleaseStaging(observedValue);
                    }

                    VkDeviceAddress getVertexDeviceAddr() {
                              return meshBuffers.getVertexBufferDeviceAddress();
                    }

                    std::string meshName;
                    std::vector<GeoSurface> meshSurfaces;
                    v2::GPUGeoMeshBuffers2 meshBuffers;
          };
}
} // namespace mesh
} // namespace engine

#endif //_MESH_ASSET_HPP_
