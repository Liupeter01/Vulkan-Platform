#include <scene/Scene.hpp>
#include <Descriptors.hpp>
#include <VulkanEngine.hpp>

namespace engine {
                    Scene::Scene(VulkanEngine * pointer)
                              : engine(pointer)
                    {}

                    Scene::~Scene() {
                              destroy();
                    }

                    void Scene::init(const std::string& root_name){
                              if (isInit) return;
                              init_scene_layout();
                              init_material();
                              init_default_color();
                              init_default_sampler();

                              node_mgr.init();

                              isInit = true;
                    }

                    void Scene::destroy() {
                              if (isInit) {
                                        node_mgr.destroy();
                                        destroy_default_color();
                                        destroy_default_sampler();
                                        destroy_scene_layout();
                                        destroy_material();
                                        isInit = false;
                              }
                    }

                    void Scene::init_scene_layout() {
                              myScene.sceneDescriptorSetLayout_ = create_ubo_layout();
                    }

                    void Scene::init_default_color() {
                              white_.reset();
                              grey_.reset();
                              black_.reset();
                              magenta_.reset();
                              loaderrorImage_.reset();

                              white_ = std::make_unique<AllocatedTexture>(engine->device_, engine->allocator_);
                              grey_ = std::make_unique<AllocatedTexture>(engine->device_, engine->allocator_);
                              black_ = std::make_unique<AllocatedTexture>(engine->device_, engine->allocator_);
                              magenta_ = std::make_unique<AllocatedTexture>(engine->device_, engine->allocator_);
                              loaderrorImage_ = std::make_unique<AllocatedTexture>(engine->device_, engine->allocator_);

                              uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
                              uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
                              uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
                              uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));

                              std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture

                              for (int x = 0; x < 16; x++) {
                                        for (int y = 0; y < 16; y++) {
                                                  pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
                                        }
                              }

                              white_->createBuffer(reinterpret_cast<void*>(&white), VkExtent3D{ 1, 1, 1 },
                                        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                              grey_->createBuffer(reinterpret_cast<void*>(&grey), VkExtent3D{ 1, 1, 1 },
                                        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                              black_->createBuffer(reinterpret_cast<void*>(&black), VkExtent3D{ 1, 1, 1 },
                                        VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

                              magenta_->createBuffer(reinterpret_cast<void*>(&magenta),
                                        VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
                                        VK_IMAGE_USAGE_SAMPLED_BIT);

                              loaderrorImage_->createBuffer(reinterpret_cast<void*>(pixels.data()),
                                        VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
                                        VK_IMAGE_USAGE_SAMPLED_BIT);

                    }

                    void Scene::init_default_sampler() {
                              VkSamplerCreateInfo samplerInfo{};
                              samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                              samplerInfo.magFilter = VK_FILTER_NEAREST;
                              samplerInfo.minFilter = VK_FILTER_NEAREST;
                              vkCreateSampler(engine->device_, &samplerInfo, nullptr, &defaultSamplerNearest_);

                              samplerInfo.magFilter = VK_FILTER_LINEAR;
                              samplerInfo.minFilter = VK_FILTER_LINEAR;
                              vkCreateSampler(engine->device_, &samplerInfo, nullptr, &defaultSamplerLinear_);
                    }

                    void Scene::destroy_default_sampler(){
                              vkDestroySampler(engine->device_, defaultSamplerNearest_, nullptr);
                              vkDestroySampler(engine->device_, defaultSamplerLinear_, nullptr);
                    }

                    void Scene::destroy_default_color() {

                              white_->destroy();
                              grey_->destroy();
                              black_->destroy();
                              magenta_->destroy();
                              loaderrorImage_->destroy();

                              white_.reset();
                              grey_.reset();
                              black_.reset();
                              magenta_.reset();
                              loaderrorImage_.reset();
                    }

                    void Scene::destroy_scene_layout() {
                              vkDestroyDescriptorSetLayout(engine->device_, myScene.sceneDescriptorSetLayout_, nullptr);
                    }

                    void  Scene::init_material() {
                              metalRoughMaterial.reset();
                              metalRoughMaterial = std::make_unique<GLTFMetallic_Roughness>(engine->device_);
                              metalRoughMaterial->init(myScene.sceneDescriptorSetLayout_);
                    }

                    void  Scene::destroy_material() {
                              metalRoughMaterial->destory();
                              metalRoughMaterial.reset();
                    }

                    VkDescriptorSetLayout Scene::create_ubo_layout() {
                              return DescriptorLayoutBuilder{ engine->device_ }
                                        .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                                        .build(VK_SHADER_STAGE_VERTEX_BIT |
                                                  VK_SHADER_STAGE_FRAGMENT_BIT); // add bindings
                    }

                    void Scene::update_scene() {

                              ctx.OpaqueSurfaces.clear();

                              //Execute Draw Command From Root Node!
                              node_mgr.draw({ 1.f }, ctx);

                              //Update Camera
                              myScene.globalSceneData.view = engine->camera_->getViewMatrix();
                              myScene.globalSceneData.proj = engine->camera_->getProjectionMatrix();
                              myScene.globalSceneData.proj[1][1] *= -1;                    //Reverse Y
                    }

                    std::tuple <VkDescriptorSet, std::shared_ptr<AllocatedBuffer>> Scene::createSceneSet(FrameData& frame) {

                              std::shared_ptr<AllocatedBuffer> sceneDataBuffer = std::make_shared<AllocatedBuffer>(engine->allocator_);

                              sceneDataBuffer->create(
                                        sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU, "Scene::execute::sceneDataBuffer");

                              GPUSceneData* data = reinterpret_cast<GPUSceneData*>(sceneDataBuffer->map());
                              memcpy(data, &myScene.globalSceneData, sizeof(GPUSceneData));
                              sceneDataBuffer->unmap();

                              VkDescriptorSet sceneSet = frame._frameDescriptor.allocate(myScene.sceneDescriptorSetLayout_);
                              DescriptorWriter scenewriter{ engine->device_ };
                              scenewriter.write_buffer(0, sceneDataBuffer->buffer, sizeof(GPUSceneData), 0,
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                              scenewriter.update_set(sceneSet);

                              return { sceneSet , sceneDataBuffer };
                    }

                    std::tuple< MaterialInstance, std::shared_ptr<AllocatedBuffer>> Scene::createDefaultMaterialInstance(FrameData& frame) {

                              std::shared_ptr<AllocatedBuffer> materialBuffer =
                                        std::make_shared<AllocatedBuffer>(engine->allocator_);
                              materialBuffer->create(
                                        sizeof(MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU, "Scene::createDefaultMaterialInstance::MaterialConstants");

                              MaterialConstants* constant =
                                        reinterpret_cast<MaterialConstants*>(materialBuffer->map());
                              constant->colorFactors = glm::vec4{ 1, 1, 1, 1 };
                              constant->metal_rough_factors = glm::vec4{ 1, 0.5, 0, 0 };
                              materialBuffer->unmap();

                              MaterialResources materialResources;
                              materialResources.colorImage = white_->getImageView();
                              materialResources.colorSampler = defaultSamplerLinear_;
                              materialResources.metalRoughImage = white_->getImageView();
                              materialResources.metalRoughSampler = defaultSamplerLinear_;
                              materialResources.materialConstantsData = materialBuffer->buffer;

                              return  { metalRoughMaterial->generate_instance(
                                         MaterialPass::OPAQUE, materialResources, frame._frameDescriptor), materialBuffer };
                    }

                    void Scene::execute(VkCommandBuffer cmd, FrameData& frame) {

                              std::string last_mesh;

                              /*set = 0, binding = 0*/
                              auto [sceneSet, sceneDataBuffer] = createSceneSet(frame);

                              /*set = 1, binding = 0, 1, 2*/
                              auto [defaultMateral , materialBuffer ] = createDefaultMaterialInstance(frame);

                              frame.destroy_by_deferred([sceneDataBuffer, materialBuffer]() {
                                        sceneDataBuffer->destroy();
                                        materialBuffer->destroy();
                                        });

                              for (auto& surface : ctx.OpaqueSurfaces) {

                                        //Its different from last mesh! So we should resubmit the vertices!
                                        if (last_mesh != surface.mesh_name) {
                                                  last_mesh = surface.mesh_name;

                                                  if (auto mesh = node_mgr.findMesh(surface.mesh_name); mesh) {
                                                            auto submitter = [&mesh](VkCommandBuffer command) {(*mesh)->submitMesh(command); };
                                                            engine->imm_command_submit(submitter);
                                                            (*mesh)->flushUpload(engine->immFence_);
                                                  }
                                        }

                                        //No Material Exist!
                                        if (!surface.material) {
                                                  //setup default material
                                                  surface.material = &defaultMateral;
                                        }

                                        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                                  surface.material->pipeline->getPipeline());

                                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                  surface.material->pipeline->getPipelineLayout(), 0, 1, &sceneSet, 0, nullptr);
                                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                                  surface.material->pipeline->getPipelineLayout(), 1, 1, &surface.material->materialSet, 0, nullptr);

                                        GPUGeoPushConstants constants{};
                                        constants.matrix = surface.transform;
                                        constants.vertexBuffer = surface.vertexBufferAddress;

                                        vkCmdPushConstants(cmd, surface.material->pipeline->getPipelineLayout(),
                                                  VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUGeoPushConstants),
                                                  &constants);

                                        vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);
                              }
                    }

                    bool Scene::attachChildren(const std::string& parentName, std::shared_ptr<MeshAsset> asset) {
                              return node_mgr.attachChildren(parentName, asset);
                    }
                    bool Scene::attachChildren(const std::string& parentName, std::shared_ptr<node::BaseNode> child) {
                              return node_mgr.attachChildren(parentName, child);
                    }
                    bool Scene::attachChildrens(const std::string& parentName,
                              const std::vector<std::shared_ptr<node::BaseNode>>& childrens) {
                              return node_mgr.attachChildrens(parentName, childrens);
                    }
                    bool Scene::attachChildrens(const std::string& parentName,
                              const std::vector<std::shared_ptr<MeshAsset>>& childrens) {
                              return node_mgr.attachChildrens(parentName, childrens);
                    }
          
}