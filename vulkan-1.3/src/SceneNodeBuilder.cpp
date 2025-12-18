#include <VulkanEngine.hpp>
#include <builder/SceneNodeBuilder.hpp>
#include <nodes/mesh/MeshNode.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace engine {

SceneNodeBuilder::SceneNodeBuilder(VulkanEngine *eng) : engine_(eng) {}

SceneNodeBuilder &SceneNodeBuilder::set_options(fastgltf::Options option) {
  options_ = option;
  return *this;
}

SceneNodeBuilder &
SceneNodeBuilder::set_filepath(std::filesystem::path filePath) {
  filepath_ = filePath;
  return *this;
}

SceneNodeBuilder &
SceneNodeBuilder::set_config(const node::SceneNodeConf &conf) {
  conf_ = conf;
  return *this;
}

SceneNodeBuilder &SceneNodeBuilder::enable_mipmap(bool status) {
  enableMipmap_ = status;
  return *this;
}

std::optional<std::shared_ptr<node::SceneNode>> SceneNodeBuilder::build() {

  if (filepath_.empty()) {
    spdlog::error("[SceneNodeBuilder Error]: Empty File Path!");
    return {};
  }

  if (!conf_.has_value()) {
    spdlog::error("[SceneNodeBuilder Error]: Invalid Scene Config!");
    return {};
  }

  if (!engine_) {
    spdlog::critical(
        "[SceneNodeBuilder Critical]: Invalid Vulkan Engine Handle!");
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

  conf_.value().setCount = static_cast<uint32_t>(gltf.materials.size());
  scene.reset();
  scene = std::make_shared<node::SceneNode>(engine_);
  scene->init(conf_.value());

  // Load Sampler & LOD
  processSamplers(gltf);

  // Loading Images
  processImages(gltf, enableMipmap_);

  // Loading Material(processMaterials)
  processMaterials(gltf);

  // Loading Meshes(processMeshes)
  processMeshes(gltf);

  // Loading Nodes(processNodes)
  processNodes(gltf);

  // Loading Relation
  processRelation(gltf);

  return scene;
}

VkFilter SceneNodeBuilder::extract_filter(fastgltf::Filter filter) {
  switch (filter) {
    // nearest samplers
  case fastgltf::Filter::Nearest:
  case fastgltf::Filter::NearestMipMapNearest:
  case fastgltf::Filter::NearestMipMapLinear:
    return VK_FILTER_NEAREST;

    // linear samplers
  case fastgltf::Filter::Linear:
  case fastgltf::Filter::LinearMipMapNearest:
  case fastgltf::Filter::LinearMipMapLinear:
  default:
    return VK_FILTER_LINEAR;
  }
}

VkSamplerMipmapMode
SceneNodeBuilder::extract_mipmap_mode(fastgltf::Filter filter) {
  switch (filter) {
  case fastgltf::Filter::NearestMipMapNearest:
  case fastgltf::Filter::LinearMipMapNearest:
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;

  case fastgltf::Filter::NearestMipMapLinear:
  case fastgltf::Filter::LinearMipMapLinear:
  default:
    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  }
}

std::optional<std::shared_ptr<AllocatedTexture>>
SceneNodeBuilder::extract_image(fastgltf::Asset &gltf, fastgltf::Image &image,
                                bool mipMapped) {

  int width, height, nrChannels;
  std::shared_ptr<AllocatedTexture> ret{nullptr};

  auto uri = [&](fastgltf::sources::URI &filePath) {
    assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
    assert(filePath.uri
               .isLocalPath()); // We're only capable of loading local files.

    const std::string path(filePath.uri.path().begin(),
                           filePath.uri.path().end());
    unsigned char *data =
        stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    if (data) {
      VkExtent3D imagesize;
      imagesize.width = width;
      imagesize.height = height;
      imagesize.depth = 1;

      ret.reset();
      ret = std::make_shared<AllocatedTexture>(engine_->device_,
                                               engine_->allocator_);
      ret->createBuffer(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                        mipMapped);
      stbi_image_free(data);
    }
  };

  auto vector = [&](fastgltf::sources::Vector &vector) {
    unsigned char *data = stbi_load_from_memory(
        reinterpret_cast<unsigned char *>(vector.bytes.data()),
        static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
    if (data) {
      VkExtent3D imagesize;
      imagesize.width = width;
      imagesize.height = height;
      imagesize.depth = 1;

      ret.reset();
      ret = std::make_shared<AllocatedTexture>(engine_->device_,
                                               engine_->allocator_);
      ret->createBuffer(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_SAMPLED_BIT, mipMapped);

      stbi_image_free(data);
    }
  };

  auto array = [&](const fastgltf::sources::Array& arr,
            const fastgltf::BufferView& bufferView) {
                      const stbi_uc* dataPtr =
                                reinterpret_cast<const stbi_uc*>(arr.bytes.data())
                                + bufferView.byteOffset;

                      const size_t dataSize = bufferView.byteLength;

                      unsigned char* data = stbi_load_from_memory(
                                dataPtr,
                                static_cast<int>(dataSize),
                                &width,
                                &height,
                                &nrChannels,
                                4);

                      if (!data) {
                                spdlog::error("stbi failed: {}", stbi_failure_reason());
                                return;
                      }

                      VkExtent3D imagesize;
                      imagesize.width = width;
                      imagesize.height = height;
                      imagesize.depth = 1;

                      ret.reset();
                      ret = std::make_shared<AllocatedTexture>(engine_->device_, engine_->allocator_);
                      ret->createBuffer(data, imagesize,
                                VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_USAGE_SAMPLED_BIT , mipMapped);
                      stbi_image_free(data);
            };

  auto bufferview = [&](fastgltf::sources::BufferView &view) {
    auto &bufferView = gltf.bufferViews[view.bufferViewIndex];
    auto &buffer = gltf.buffers[bufferView.bufferIndex];

    std::visit(
        [&, array](auto &&arg) {
                        using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, fastgltf::sources::Array>) {
                    array(arg, bufferView);
                    return;
          }
          throw std::runtime_error("Other Format Other than Array not supported!");
        },
        buffer.data);
  };

  std::visit(
      [uri, vector, bufferview](auto &&args) {
        using ArgsType = std::decay_t<decltype(args)>;
        if constexpr (std::is_same_v<ArgsType, fastgltf::sources::URI>) {
          uri(args);
        } else if constexpr (std::is_same_v<ArgsType,
                                            fastgltf::sources::Vector>) {
          vector(args);
        }
        else if constexpr (std::is_same_v<ArgsType,
                                            fastgltf::sources::BufferView>) {

                  if (args.mimeType != fastgltf::MimeType::PNG &&
                            args.mimeType != fastgltf::MimeType::JPEG) {
                            throw std::runtime_error("Image bufferView is not an image");
                  }
          bufferview(args);

        } else {
          spdlog::error("[SceneNodeBuilder Error]: Unsupported fastgltf::Image "
                        "source type '{}'. Aborting.",
                        typeid(ArgsType).name());
          std::abort();
        }
      },
      image.data);

  if (!ret) {
    return {};
  }
  return ret;
}

void SceneNodeBuilder::processSamplers(fastgltf::Asset &gltf) {
  for (fastgltf::Sampler &sampler : gltf.samplers) {
    VkSamplerCreateInfo sampl{};
    sampl.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampl.maxLod = VK_LOD_CLAMP_NONE;
    sampl.minLod = 0.f;
    sampl.mipLodBias = 0.f;

    // sampl.anisotropyEnable = VK_TRUE;
    // sampl.maxAnisotropy =
    //     engine_->vkb_physicalDevice_.properties.limits.maxSamplerAnisotropy;

    sampl.compareEnable = VK_FALSE;
    sampl.compareOp = VK_COMPARE_OP_ALWAYS;

    sampl.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampl.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampl.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    sampl.magFilter =
        extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Linear));
    sampl.minFilter =
        extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Linear));
    sampl.mipmapMode = extract_mipmap_mode(
        sampler.minFilter.value_or(fastgltf::Filter::Linear));

    VkSampler newSampler;
    vkCreateSampler(engine_->device_, &sampl, nullptr, &newSampler);
    scene->samplers.push_back(newSampler);
  }
}

void SceneNodeBuilder::processImages(fastgltf::Asset &gltf, bool mipMapped) {
  for (fastgltf::Image &i : gltf.images) {

    auto status = extract_image(gltf, i, mipMapped);

    // Current There is no texture loaded, using default checkerboard
    auto image = status.has_value() ? status.value() : engine_->loaderrorImage_;

    images_.push_back(image);
    auto [_, isInserted] = scene->images_.try_emplace(i.name.c_str(), image);
    if (!isInserted) {
      spdlog::warn("[SceneNodeBuilder Warn]: Try Emplace AllocatedTexture to "
                   "unordered_map Exist!");
    }
  }
}

void SceneNodeBuilder::processMaterials(fastgltf::Asset &gltf) {
  scene->materialBuffer.reset();
  scene->materialBuffer =
      std::make_unique<AllocatedBuffer>(engine_->allocator_);
  scene->materialBuffer->create(
      sizeof(MaterialConstants) * gltf.materials.size(),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
      "SceneNodeBuilder::processMaterials");

  MaterialConstants *sceneMaterialConstants =
      reinterpret_cast<MaterialConstants *>(scene->materialBuffer->map());

  std::size_t index{0};
  for (fastgltf::Material &mat : gltf.materials) {

    std::shared_ptr<MaterialInstance> mat_ins =
        std::make_shared<MaterialInstance>();
    materials_.push_back(mat_ins); // temp

    auto [_, status] = scene->materials_.try_emplace(mat.name.c_str(), mat_ins);
    if (!status) {
      spdlog::warn("[NodePackedCreator]: Try Emplace MaterialInstance to "
                   "unordered_map Exist!");
    }

    auto *material_writer = &sceneMaterialConstants[index];
    material_writer->colorFactors.x = mat.pbrData.baseColorFactor[0];
    material_writer->colorFactors.y = mat.pbrData.baseColorFactor[1];
    material_writer->colorFactors.z = mat.pbrData.baseColorFactor[2];
    material_writer->colorFactors.w = mat.pbrData.baseColorFactor[3];
    material_writer->metal_rough_factors.x = mat.pbrData.metallicFactor;
    material_writer->metal_rough_factors.y = mat.pbrData.roughnessFactor;

    MaterialResources resources;
    resources.colorImage = engine_->loaderrorImage_->getImageView();
    resources.colorSampler = engine_->defaultSamplerLinear_;
    resources.metalRoughImage =  engine_->loaderrorImage_->getImageView();
    resources.metalRoughSampler = engine_->defaultSamplerLinear_;
    resources.materialConstantsData = scene->materialBuffer->buffer;
    resources.materialConstantsOffset = index * sizeof(MaterialConstants);

    // grab textures from gltf file
    size_t img_ind, sampler_ind;
    if (mat.pbrData.baseColorTexture.has_value()) {
     img_ind =
          gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex]
              .imageIndex.value();
      sampler_ind =
          gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex]
              .samplerIndex.value();

      resources.colorImage = images_[img_ind]->getImageView();
      resources.colorSampler = scene->samplers[sampler_ind];
    }

    MaterialPass passType = MaterialPass::OPAQUE;
    if (mat.alphaMode == fastgltf::AlphaMode::Blend)
      passType = MaterialPass::TRANSPARENT;

    *mat_ins = scene->metalRoughMaterial->generate_instance(passType, resources,
                                                            scene->pool_);

    if (mat.pbrData.baseColorTexture.has_value()) {
              mat_ins->texture = images_[img_ind];
              mat_ins->samplers = scene->samplers[sampler_ind];
    }
    index++;
  }

  scene->materialBuffer->unmap();
}

void SceneNodeBuilder::processMeshes(fastgltf::Asset &gltf) {

  for (fastgltf::Mesh &mesh : gltf.meshes) {

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    std::shared_ptr<MeshAsset> newMesh =
        std::make_shared<MeshAsset>(engine_->device_, engine_->allocator_);

    newMesh->meshName = mesh.name.c_str();
    meshes_.push_back(newMesh);

    auto [_, status] = scene->meshes_.try_emplace(newMesh->meshName, newMesh);
    if (!status) {
      spdlog::warn("[NodePackedCreator]: Try Emplace "
                   "std::shared_ptr<MeshAsset> to unordered_map Exist!");
    }

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
            gltf, indexaccessor, [&](std::uint32_t idx) {
              indices.push_back(idx + static_cast<uint32_t>(initial_vtx));
            });
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

      if (primitive.materialIndex.has_value()) {
        newSurface.material = materials_[primitive.materialIndex.value()];
      } else {
        newSurface.material = materials_[0];
      }

      glm::vec3 min = vertices[initial_vtx].position;
      glm::vec3 max = min;

      for (std::size_t i = initial_vtx; i < vertices.size(); i++) {
        min = glm::min(min, vertices[i].position);
        max = glm::max(max, vertices[i].position);
      }
      newSurface.bounds = Bounds3{min, max};

      newMesh->meshSurfaces.push_back(newSurface);
    }
    newMesh->createAsset(std::move(vertices), std::move(indices));
  }
}

void SceneNodeBuilder::processNodes(fastgltf::Asset &gltf) {

  for (fastgltf::Node &node : gltf.nodes) {
    std::shared_ptr<node::BaseNode> newNode{};
    if (!node.meshIndex.has_value()) {
      newNode = std::make_shared<node::BaseNode>();
    } else {
      newNode = std::make_shared<node::MeshNode>();
      std::dynamic_pointer_cast<node::MeshNode>(newNode)->mesh_ =
          meshes_[node.meshIndex.value()];
    }

    newNode->node_name = node.name.c_str();
    nodes_.push_back(newNode);

    // auto [_, status] = nodesPacked->nodes_.try_emplace(newNode->node_name,
    // newNode); if (!status) {
    //           spdlog::warn("[NodePackedCreator Warn]: Try Emplace
    //           std::shared_ptr<BaseNode> to unordered_map Exist!");
    // }

    auto visiter = [newNode](auto &&transform) {
      using BaseType = std::decay_t<decltype(transform)>;
      if constexpr (std::is_same_v<BaseType, fastgltf::TRS>) {
        glm::vec3 t(transform.translation[0], transform.translation[1],
                    transform.translation[2]);
        glm::quat q(transform.rotation[3], transform.rotation[0],
                    transform.rotation[1], transform.rotation[2]);
        glm::vec3 s(transform.scale[0], transform.scale[1], transform.scale[2]);
        newNode->localTransform.translation = t;
        newNode->localTransform.scale = s;
        newNode->localTransform.quat = q;
      } else if constexpr (std::is_same_v<BaseType, fastgltf::math::fmat4x4>) {
        memcpy(&newNode->localTransform.localMatrix, transform.data(),
               sizeof(BaseType));
        newNode->localTransform.dirty = false;
      } else {
        spdlog::error("[NodesPackedCreator Error]: No TRS Provided!");
        throw std::runtime_error("NO TRS Provided!");
      }
    };

    std::visit(visiter, node.transform);
  }
}

void SceneNodeBuilder::processRelation(fastgltf::Asset &gltf) {
  if (!scene->sceneRoot_) {
    spdlog::error("[NodesPackedCreator Error]: Invalid Scene Root!");
    throw std::runtime_error("[NodesPackedCreator]: Invalid Scene Root!");
  }

  const std::size_t nodes_size = gltf.nodes.size();
  for (std::size_t i = 0; i < nodes_size; ++i) {

    auto &gltf_node = gltf.nodes[i];
    auto &fatherNode = nodes_[i];

    for (std::size_t &child : gltf_node.children) {

      auto &childNode = nodes_[child];

      // manipulate parent node
      childNode->parent_ = fatherNode;

      // add child node to father
      fatherNode->children.push_back(childNode);
    }
  }

  for (auto &node : nodes_) {
    // No Parent Node Exist
    if (node->parent_.lock() == nullptr) {
      node->refreshTransform(glm::identity<glm::mat4>());
      node->parent_ = scene->sceneRoot_;
      scene->sceneRoot_->children.push_back(node);
    }
  }

  scene->sceneRoot_->refreshTransform(glm::identity<glm::mat4>());

  // Create Inner Lookup Table!
  scene->insert_to_lookup_table(scene->sceneRoot_,
                                scene->sceneRoot_->node_path);
}
} // namespace engine
