#include <spdlog/spdlog.h>
#include <VulkanEngine.hpp>
#include <builder/SceneNodeBuilder.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace engine {

          SceneNodeBuilder::SceneNodeBuilder(VulkanEngine* eng)
                    :engine_(eng)
          {}

          SceneNodeBuilder& SceneNodeBuilder::set_options(fastgltf::Options option) {
                    options_ = option;
                    return *this;
          }

          SceneNodeBuilder& SceneNodeBuilder::set_filepath(std::filesystem::path filePath) {
                    filepath_ = filePath;
                    return *this;
          }

          SceneNodeBuilder& SceneNodeBuilder::set_config(const node::SceneNodeConf& conf) {
                    conf_ = conf;
                    return *this;
          }

          std::optional<std::shared_ptr<node::SceneNode>>  SceneNodeBuilder::build() {

                    if (filepath_.empty()) {
                              spdlog::error("[SceneNodeBuilder Error]: Empty File Path!");
                              return {};
                    }

                    if (!conf_.has_value()) {
                              spdlog::error("[SceneNodeBuilder Error]: Invalid Scene Config!");
                              return {};
                    }

                    if (!engine_) {
                              spdlog::critical("[SceneNodeBuilder Critical]: Invalid Vulkan Engine Handle!");
                              return {};
                    }

                    fastgltf::Parser parser{};
                    auto gltfData = fastgltf::GltfDataBuffer::FromPath(filepath_);

                    fastgltf::Asset gltf;
                    const auto type = fastgltf::determineGltfFileType(gltfData.get());
                    if (type == fastgltf::GltfType::Invalid) {
                              spdlog::error("[NodesPackedCreator]: Invalid GLTF File Type!");
                              return {};
                    }
                    if (type == fastgltf::GltfType::GLB) {
                              auto load_gltf = parser.loadGltf(gltfData.get(), filepath_.parent_path(), options_);
                              if (!load_gltf) {
                                        spdlog::error("[NodesPackedCreator]: Failed to Load GLB: {}!",
                                                  fastgltf::to_underlying(load_gltf.error()));
                                        return std::nullopt;
                              }

                              gltf = std::move(load_gltf.get());
                    }
                    else if (type == fastgltf::GltfType::glTF) {
                              auto load_gltf = parser.loadGltfBinary(gltfData.get(), filepath_.parent_path(), options_);
                              if (!load_gltf) {
                                        spdlog::error("[NodesPackedCreator]: Failed to Load GLTF: {}!",
                                                  fastgltf::to_underlying(load_gltf.error()));
                                        return std::nullopt;
                              }

                              gltf = std::move(load_gltf.get());
                    }

                    scene.reset();
                    scene = std::make_shared<node::SceneNode>(engine_);
                    scene->init(conf_.value());

                    //Load Sampler & LOD
                    processSamplers(gltf);

                    processImages(gltf);

                    //Loading Material(processMaterials)
                    processMaterials(gltf);

                    //Loading Meshes(processMeshes)
                    processMeshes( gltf);

                    //Loading Nodes(processNodes)
                    processNodes(gltf);

                    //Loading Relation
                    processRelation(gltf);

                    return scene;
          }

          VkFilter SceneNodeBuilder::extract_filter(fastgltf::Filter filter){
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

          VkSamplerMipmapMode SceneNodeBuilder::extract_mipmap_mode(fastgltf::Filter filter){
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

          void SceneNodeBuilder::processSamplers(fastgltf::Asset& gltf){
                    for (fastgltf::Sampler& sampler : gltf.samplers) {
                              VkSamplerCreateInfo sampl{};
                              sampl.maxLod = VK_LOD_CLAMP_NONE;
                              sampl.minLod = 0;
                              sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
                              sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
                              sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

                              VkSampler newSampler;
                              vkCreateSampler(engine_->device_, &sampl, nullptr, &newSampler);
                              scene->samplers.push_back(newSampler);
                    }
          }

          void SceneNodeBuilder::processImages(fastgltf::Asset& gltf){
                    for (fastgltf::Image& i : gltf.images) {

                              //Current There is no texture loaded
                              // 
                              //Using default checkerboard
                              images_.push_back(engine_->loaderrorImage_);
                              auto [_, status] = scene->images_.try_emplace(i.name.c_str(), engine_->loaderrorImage_);
                              if (!status) {
                                        spdlog::warn("[SceneNodeBuilder Warn]: Try Emplace AllocatedTexture to unordered_map Exist!");
                              }
                    }
          }

          void SceneNodeBuilder::processMaterials(fastgltf::Asset& gltf){
                    scene->materialBuffer.reset();
                    scene->materialBuffer = std::make_unique<AllocatedBuffer>(engine_->allocator_);
                    scene->materialBuffer->create(sizeof(MaterialConstants) * gltf.materials.size(),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU,
                              "SceneNodeBuilder::processMaterials");

                    MaterialConstants* sceneMaterialConstants =
                              reinterpret_cast<MaterialConstants*>(scene->materialBuffer->map());


                    std::size_t index{ 0 };
                    for (fastgltf::Material& mat : gltf.materials) {

                              std::shared_ptr<MaterialInstance> mat_ins = std::make_shared<MaterialInstance>();
                              materials_.push_back(mat_ins);                  //temp

                              auto [_, status] = scene->materials_.try_emplace(mat.name.c_str(), mat_ins);
                              if (!status) {
                                        spdlog::warn("[NodePackedCreator]: Try Emplace MaterialInstance to unordered_map Exist!");
                              }

                              auto* material_writer = &sceneMaterialConstants[index];
                              material_writer->colorFactors.x = mat.pbrData.baseColorFactor[0];
                              material_writer->colorFactors.y = mat.pbrData.baseColorFactor[1];
                              material_writer->colorFactors.z = mat.pbrData.baseColorFactor[2];
                              material_writer->colorFactors.w = mat.pbrData.baseColorFactor[3];
                              material_writer->metal_rough_factors.x = mat.pbrData.metallicFactor;
                              material_writer->metal_rough_factors.y = mat.pbrData.roughnessFactor;

                              MaterialResources resources;
                              resources.colorImage = engine_->white_->getImageView();
                              resources.colorSampler = engine_->defaultSamplerLinear_;
                              resources.metalRoughImage =engine_->white_->getImageView();
                              resources.metalRoughSampler =engine_->defaultSamplerLinear_;
                              resources.materialConstantsData = scene->materialBuffer->buffer;
                              resources.materialConstantsOffset = index * sizeof(MaterialConstants);

                              // grab textures from gltf file
                              if (mat.pbrData.baseColorTexture.has_value()) {
                                        size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                                        size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                                        resources.colorImage = images_[img]->getImageView();
                                        resources.colorSampler = scene->samplers[sampler];
                              }

                              MaterialPass passType = MaterialPass::OPAQUE;
                              if (mat.alphaMode == fastgltf::AlphaMode::Blend)
                                        passType = MaterialPass::TRANSPARENT;

                              *mat_ins = scene->metalRoughMaterial->generate_instance(passType, resources, scene->pool_);
                              index++;
                    }

                    scene->materialBuffer->unmap();
          }

          void SceneNodeBuilder::processMeshes(fastgltf::Asset& gltf){

                    for (fastgltf::Mesh& mesh : gltf.meshes) {

                              std::vector<uint32_t> indices;
                              std::vector<Vertex> vertices;

                              std::shared_ptr<MeshAsset> newMesh =
                                        std::make_shared<MeshAsset>(engine_->device_,
                                                 engine_->allocator_);

                              newMesh->meshName = mesh.name.c_str();
                              meshes_.push_back(newMesh);

                              auto [_, status] =scene->meshes_.try_emplace(newMesh->meshName, newMesh);
                              if (!status) {
                                        spdlog::warn("[NodePackedCreator]: Try Emplace std::shared_ptr<MeshAsset> to unordered_map Exist!");
                              }

                              for (auto&& primitive : mesh.primitives) {
                                        GeoSurface newSurface{};
                                        newSurface.startIndex = static_cast<uint32_t>(indices.size());
                                        newSurface.count = static_cast<uint32_t>(
                                                  gltf.accessors[primitive.indicesAccessor.value()].count);

                                        size_t initial_vtx = vertices.size();

                                        // load indexes
                                        {
                                                  fastgltf::Accessor& indexaccessor =
                                                            gltf.accessors[primitive.indicesAccessor.value()];
                                                  indices.reserve(indices.size() + indexaccessor.count);

                                                  fastgltf::iterateAccessor<std::uint32_t>(
                                                            gltf, indexaccessor,
                                                            [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
                                        }

                                        // load vertex positions
                                        {
                                                  fastgltf::Accessor& posAccessor =
                                                            gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
                                                  vertices.resize(vertices.size() + posAccessor.count);

                                                  fastgltf::iterateAccessorWithIndex<glm::vec3>(
                                                            gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                                                                      Vertex newvtx;
                                                                      newvtx.position = v;
                                                                      newvtx.normal = { 1, 0, 0 };
                                                                      newvtx.color = glm::vec4{ 1.f };
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
                                        }
                                        else {
                                                  newSurface.material = materials_[0];
                                        }

                                        newMesh->meshSurfaces.push_back(newSurface);
                              }
                              newMesh->createAsset(std::move(vertices), std::move(indices));
                    }

          }

          void SceneNodeBuilder::processNodes(fastgltf::Asset& gltf){

                    for (fastgltf::Node& node : gltf.nodes) {
                              std::shared_ptr<node::BaseNode> newNode{};
                              if (!node.meshIndex.has_value()) {
                                        newNode = std::make_shared<node::BaseNode>();
                              }
                              else {
                                        newNode = std::make_shared<node::MeshNode>();
                                        std::dynamic_pointer_cast<node::MeshNode>(newNode)->mesh_ =
                                                  meshes_[node.meshIndex.value()];
                              }

                              newNode->node_name = node.name.c_str();
                              nodes_.push_back(newNode);

                              //auto [_, status] = nodesPacked->nodes_.try_emplace(newNode->node_name, newNode);
                              //if (!status) {
                              //          spdlog::warn("[NodePackedCreator Warn]: Try Emplace std::shared_ptr<BaseNode> to unordered_map Exist!");
                              //}

                              auto visiter = [newNode](auto&& transform) {
                                        using BaseType = std::decay_t<decltype(transform)>;
                                        if constexpr (std::is_same_v< BaseType, fastgltf::TRS>) {
                                                  glm::vec3 t(transform.translation[0], transform.translation[1], transform.translation[2]);
                                                  glm::quat q(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                                                  glm::vec3 s(transform.scale[0], transform.scale[1], transform.scale[2]);
                                                  newNode->localTransform.translation = t;
                                                  newNode->localTransform.scale = s;
                                                  newNode->localTransform.quat = q;
                                        }
                                        else if constexpr (std::is_same_v< BaseType, fastgltf::math::fmat4x4>) {
                                                  memcpy(&newNode->localTransform.localMatrix, transform.data(), sizeof(BaseType));
                                                  newNode->localTransform.dirty = false;
                                        }
                                        else {
                                                  spdlog::warn("[NodesPackedCreator Warn]: No TRS Provided!");
                                                  newNode->localTransform = glm::mat4(1.f);
                                        }
                                        };

                              std::visit(visiter, node.transform);
                    }

          }

          void SceneNodeBuilder::processRelation(fastgltf::Asset& gltf){
                    if (!scene->sceneRoot_) {
                              spdlog::error("[NodesPackedCreator Error]: Invalid Scene Root!");
                              throw std::runtime_error("[NodesPackedCreator]: Invalid Scene Root!");
                    }

                    const std::size_t nodes_size = gltf.nodes.size();
                    for (std::size_t i = 0; i < nodes_size; ++i) {

                              auto& gltf_node = gltf.nodes[i];
                              auto& fatherNode = nodes_[i];

                              for (std::size_t& child : gltf_node.children) {

                                        auto& childNode = nodes_[child];

                                        //manipulate parent node
                                        childNode->parent_ = fatherNode;

                                        //add child node to father
                                        fatherNode->children.push_back(childNode);
                              }
                    }

                    for (auto& node :nodes_) {
                              //No Parent Node Exist
                              if (node->parent_.lock() == nullptr) {
                                        node->refreshTransform(glm::identity<glm::mat4>());
                                        node->parent_ =scene->sceneRoot_;
                                        scene->sceneRoot_->children.push_back(node);
                              }
                    }

                              scene->sceneRoot_->refreshTransform(glm::identity<glm::mat4>());

                    //Create Inner Lookup Table!
                              scene->insert_to_lookup_table(
                                        scene->sceneRoot_,
                                        scene->sceneRoot_->node_path);
          }
}