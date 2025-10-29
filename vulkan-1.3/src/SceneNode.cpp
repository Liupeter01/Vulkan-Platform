#include <spdlog/spdlog.h>
#include <VulkanEngine.hpp>
#include <nodes/scene/SceneNode.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace engine {
          namespace node {

                    SceneNode::SceneNode(VulkanEngine* eng)
                              :engine_(eng), pool_(eng->device_), NodeManager()
                    {}

                    SceneNode::~SceneNode() {
                              destroy();
                    }

                    void SceneNode::init(const SceneNodeConf& conf) {
                              if (isinit) return;
                              NodeManager::init(conf.sceneRootName);
                              init_allocator(conf.setCount, conf.poolSizeRatio);
                              init_material(conf.globalSceneLayout);
                              isinit = true;
                    }

                    void SceneNode::destroy() {
                              if (isinit) {
                                        destroy_material();
                                        destroy_allocator();
                                        destroy_sampler();
                                        NodeManager::destroy();
                                        isinit = false;
                              }
                    }

                    void SceneNode::init_allocator(const uint32_t setCount,
                              const std::vector<PoolSizeRatio>& poolSizeRatio) {
                              pool_.init(setCount, poolSizeRatio);
                    }

                    void SceneNode::destroy_allocator() {
                              pool_.destroy_pools();
                    }

                    void SceneNode::destroy_sampler() {
                              for (auto sampler : samplers)
                                        vkDestroySampler(engine_->device_, sampler, nullptr);
                              samplers.clear();
                    }

                    void SceneNode::init_material(VkDescriptorSetLayout globalSceneLayout) {
                              metalRoughMaterial.reset();
                              metalRoughMaterial = std::make_unique<GLTFMetallic_Roughness>(engine_->device_);
                              metalRoughMaterial->init(globalSceneLayout);
                    }

                    void SceneNode::destroy_material() {
                              metalRoughMaterial->destory();
                              metalRoughMaterial.reset();
                    }

                    bool SceneNode::insert_to_lookup_table(std::shared_ptr<node::BaseNode>& node,
                              std::string_view parentPath) {
                              return NodeManager::insert_to_lookup_table(node, parentPath);
                    }

                    void SceneNode::reset_allocator_pools() {
                              pool_.reset_pools();
                    }

                    void SceneNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx){
                              NodeManager::Draw(topMatrix, ctx);
                     }

          } // namespace node
} // namespace engine