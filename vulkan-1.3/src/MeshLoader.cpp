#include <Tools.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <iostream>
#include <mesh/MeshLoader.hpp>
#include <tiny_obj_loader.h>

namespace engine {
namespace mesh {

std::optional<std::vector<std::shared_ptr<MeshAsset>>>
MeshAsset::loadGltfMeshes(VkDevice device, VmaAllocator allocator,
                          std::filesystem::path filePath) {

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};

  auto gltfData = fastgltf::GltfDataBuffer::FromPath(filePath);

  constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

  auto load_gltf = parser.loadGltfBinary(gltfData.get(), filePath.parent_path(),
                                         gltfOptions);
  if (!load_gltf) {
    std::cerr << "Failed to load GLB file: "
              << fastgltf::to_underlying(load_gltf.error()) << "\n";
    return std::nullopt;
  }

  gltf = std::move(load_gltf.get());

  std::vector<std::shared_ptr<MeshAsset>> meshes;
  for (const fastgltf::Mesh &mesh : gltf.meshes) {

    MeshAsset meshAssets{device, allocator};
    meshAssets.meshName = mesh.name;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (auto &&primitive : mesh.primitives) {
      GeoSurface newSurface{};
      newSurface.startIndex = static_cast<uint32_t>(indices.size());
      newSurface.count = static_cast<uint32_t>(
          gltf.accessors[primitive.indicesAccessor.value()].count);

      size_t initial_vtx = vertices.size();

      // load indexes
      {
        fastgltf::Accessor &indexaccessor =
            gltf.accessors[primitive.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
            gltf, indexaccessor,
            [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
      }

      // load vertex positions
      {
        fastgltf::Accessor &posAccessor =
            gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, posAccessor, [&](glm::vec3 v, size_t index) {
              Vertex newvtx;
              newvtx.position = v;
              newvtx.normal = {1, 0, 0};
              newvtx.color = glm::vec4{1.f};
              newvtx.uv_x = 0;
              newvtx.uv_y = 0;
              vertices[initial_vtx + index] = newvtx;
            });
      }

      // load vertex normals
      auto normals = primitive.findAttribute("NORMAL");
      if (normals != primitive.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, gltf.accessors[(*normals).accessorIndex],
            [&](glm::vec3 v, size_t index) {
              vertices[initial_vtx + index].normal = v;
            });
      }

      // load UVs
      auto uv = primitive.findAttribute("TEXCOORD_0");
      if (uv != primitive.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            gltf, gltf.accessors[(*uv).accessorIndex],
            [&](glm::vec2 v, size_t index) {
              vertices[initial_vtx + index].uv_x = v.x;
              vertices[initial_vtx + index].uv_y = v.y;
            });
      }

      // load vertex colors
      auto colors = primitive.findAttribute("COLOR_0");
      if (colors != primitive.attributes.end()) {

        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            gltf, gltf.accessors[(*colors).accessorIndex],
            [&](glm::vec4 v, size_t index) {
              vertices[initial_vtx + index].color = v;
            });
      }

      meshAssets.meshSurfaces.push_back(std::move(newSurface));
    }

    if (OverrideColors) {
      for (Vertex &vtx : vertices) {
        vtx.color = glm::vec4(vtx.normal, 1.f);
      }
    }

    auto ptr = std::make_shared<MeshAsset>(std::move(meshAssets));
    ptr->createAsset(std::move(vertices), std::move(indices));
    meshes.emplace_back(ptr);
  }
  return meshes;
}
} // namespace mesh
} // namespace engine
