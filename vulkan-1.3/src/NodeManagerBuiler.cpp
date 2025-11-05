#include <VulkanEngine.hpp>
#include <builder/NodeManagerBuiler.hpp>
#include <spdlog/spdlog.h>

namespace engine {

NodeManagerBuilder::NodeManagerBuilder(VulkanEngine *eng) : engine_(eng) {}

NodeManagerBuilder &NodeManagerBuilder::set_options(fastgltf::Options option) {
  options_ = option;
  return *this;
}

NodeManagerBuilder &
NodeManagerBuilder::set_filepath(std::filesystem::path filePath) {
  filepath_ = filePath;
  return *this;
}

NodeManagerBuilder &NodeManagerBuilder::set_root(const std::string &root) {
  root_name = root;
  return *this;
}

NodeManagerBuilder &NodeManagerBuilder::set_debug_color(bool status) {
  enableDebugColor = status;
  return *this;
}

std::optional<std::shared_ptr<NodeManager>> NodeManagerBuilder::build() {
  if (filepath_.empty()) {
    spdlog::error("[NodeManagerBuilder Error]: Empty File Path!");
    return {};
  }

  if (!engine_) {
    spdlog::critical(
        "[NodeManagerBuilder Critical]: Invalid Vulkan Engine Handle!");
    return {};
  }

  if (root_name.empty()) {
    spdlog::critical("[NodeManagerBuilder Critical]: Invalid Root Name!");
    return {};
  }

  fastgltf::Parser parser{};
  auto gltfData = fastgltf::GltfDataBuffer::FromPath(filepath_);

  fastgltf::Asset gltf;
  const auto type = fastgltf::determineGltfFileType(gltfData.get());
  if (type == fastgltf::GltfType::Invalid) {
    spdlog::error("[SceneNodeBuilder Error]: Invalid GLTF File Type!");
    return {};
  }
  if (type == fastgltf::GltfType::GLB) {
    auto load_gltf =
        parser.loadGltf(gltfData.get(), filepath_.parent_path(), options_);
    if (!load_gltf) {
      spdlog::error("[SceneNodeBuilder Error]: Failed to Load GLB: {}!",
                    fastgltf::to_underlying(load_gltf.error()));
      return std::nullopt;
    }

    gltf = std::move(load_gltf.get());
  } else if (type == fastgltf::GltfType::glTF) {
    auto load_gltf = parser.loadGltfBinary(gltfData.get(),
                                           filepath_.parent_path(), options_);
    if (!load_gltf) {
      spdlog::error("[SceneNodeBuilder Error]: Failed to Load GLTF: {}!",
                    fastgltf::to_underlying(load_gltf.error()));
      return std::nullopt;
    }

    gltf = std::move(load_gltf.get());
  }

  processMeshes(gltf);

  nodeMgr_.reset();
  nodeMgr_ = std::make_shared<NodeManager>();
  nodeMgr_->init(root_name);
  nodeMgr_->attachChildrens(root_name, assets_);
  return nodeMgr_;
}

void NodeManagerBuilder::processMeshes(fastgltf::Asset &gltf) {

  std::vector<std::shared_ptr<MeshAsset>> meshes;
  for (const fastgltf::Mesh &mesh : gltf.meshes) {

    MeshAsset meshAssets{engine_->device_, engine_->allocator_};
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

    if (enableDebugColor) {
      for (Vertex &vtx : vertices) {
        vtx.color = glm::vec4(vtx.normal, 1.f);
      }
    }

    auto ptr = std::make_shared<MeshAsset>(std::move(meshAssets));
    ptr->createAsset(std::move(vertices), std::move(indices));
    meshes.emplace_back(ptr);
  }
  assets_ = std::move(meshes);
}
} // namespace engine
