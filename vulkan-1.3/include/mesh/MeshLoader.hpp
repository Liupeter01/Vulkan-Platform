#pragma once
#ifndef _MESH_LOADER_HPP_
#define _MESH_LOADER_HPP_
#include <filesystem>
#include <mesh/MeshBuffers.hpp>
#include <optional>
#include <material/GLTFPBR_Material.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace engine {
namespace mesh {

struct GeoSurface {
  uint32_t startIndex{};
  uint32_t count{};
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

  /* DEBUG ONLY! display the vertex normals*/
  static constexpr bool OverrideColors = true;

  [[nodiscard]]
  static std::optional<std::vector<std::shared_ptr<MeshAsset>>>
  loadGltfMeshes(VkDevice device, VmaAllocator allocator,
                 std::filesystem::path filePath);
};
} // namespace mesh
} // namespace engine

#endif //_MESH_LOADER_HPP_
