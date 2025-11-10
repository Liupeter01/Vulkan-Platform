#include <Descriptors.hpp>
#include <Tools.hpp>
#include <VulkanEngine.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <scene/ScenesManager.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace engine {

ScenesManager::ScenesManager(VulkanEngine *eng)
    : engine_(eng), scenePool_(eng->device_) {}

ScenesManager::~ScenesManager() { destroy(); }

void ScenesManager::init() {
  if (isinit)
    return;
  init_pool();
  myScene.sceneDescriptorSetLayout_ = engine_->sceneDescriptorSetLayout_;
  init_default_compute();
  init_particle_sys();
  init_default_material();
  isinit = true;
}

void ScenesManager::destroy() {
  if (isinit) {
    destroy_scene();
    myScene.sceneDescriptorSetLayout_ = VK_NULL_HANDLE;
    destroy_default_compute();
    destroy_particle_sys();
    destroy_default_material();
    destroy_pool();
    isinit = false;
  }
}

void ScenesManager::update_scene() {
  ctx.OpaqueSurfaces.clear();

  for (float x = 0.f; x < 0.6f; x += 0.6f) {

    glm::mat4 scale = glm::scale(glm::mat4{1.f}, glm::vec3{0.3f});
    glm::mat4 translation = glm::translate(glm::mat4{1.f}, glm::vec3{x, 0, 0});

    // Execute Draw Command From Root Node!
    for (auto ib = loadedScenes_.begin(); ib != loadedScenes_.end(); ib++) {
      ib->second->Draw(translation * scale, ctx);
    }
  }

  // Update Camera
  myScene.globalSceneData.view = engine_->camera_->getViewMatrix();
  myScene.globalSceneData.proj = engine_->camera_->getProjectionMatrix();
  myScene.globalSceneData.proj[1][1] *= -1; // Reverse Y
}

// Material
void ScenesManager::init_default_material() {
  metalRoughMaterial.reset();
  metalRoughMaterial =
      std::make_unique<GLTFMetallic_Roughness>(engine_->device_);

  if (!metalRoughMaterial) {
    spdlog::error(
        "[ScenesManager Error]: Create Default Graphic Material Failed!");
    throw std::runtime_error("Alloc Default Graphic Material Failed!");
  }

  metalRoughMaterial->init(myScene.sceneDescriptorSetLayout_);
}

void ScenesManager::destroy_default_material() {
  metalRoughMaterial->destory();
  metalRoughMaterial.reset();
}

// Compute
void ScenesManager::init_default_compute() {
  imageAttachmentCompute.reset();
  imageAttachmentCompute =
      std::make_unique<Compute_ImageAttachment<>>(engine_->device_);

  if (!imageAttachmentCompute) {
    spdlog::error("[ScenesManager Error]: Create Image Attachment Compute "
                  "Material Failed!");
    throw std::runtime_error("Alloc Image Attachment Compute Material Failed!");
  }

  imageAttachmentCompute->init();
}

void ScenesManager::init_particle_sys() {

  particleSysBuffer.reset();
  particleSysBuffer = std::make_unique<ParticleSysDataBuffer>(engine_);
  particleSysBuffer->create(8192 * 2);

  particleSysCompute.reset();
  particleSysCompute =
      std::make_unique<Compute_ParticleSys<>>(engine_->device_);
  if (!particleSysCompute) {
    spdlog::error("[ScenesManager Error]: Create Particle System Compute "
                  "Material Failed!");
    throw std::runtime_error("Alloc Particle System Compute Material Failed!");
  }
}

// Pool
void ScenesManager::init_pool() { scenePool_.init(setCount_, frame_sizes); }

void ScenesManager::destroy_pool() { scenePool_.destroy_pools(); }

void ScenesManager::destroy_particle_sys() {

  particleSysCompute->destory();
  particleSysCompute.reset();
}

void ScenesManager::destroy_default_compute() {
  imageAttachmentCompute->destory();
  imageAttachmentCompute.reset();
}

void ScenesManager::destroy_scene() {
  vkDeviceWaitIdle(engine_->device_);
  loadedScenes_.clear();
}

void ScenesManager::submitMesh(VkCommandBuffer cmd) {
  if (auto mesh = loadedScenes_.find("default")->second->findMesh("Suzanne");
      mesh) {
    (*mesh)->submitMesh(cmd);
  }
}

void ScenesManager::flushUpload(VkFence fence) {

  if (auto mesh = loadedScenes_.find("default")->second->findMesh("Suzanne");
      mesh) {
    (*mesh)->flushUpload(fence);
  }
}

std::tuple<VkDescriptorSet, std::shared_ptr<AllocatedBuffer>>
ScenesManager::createSceneSet(FrameData &frame) {

  std::shared_ptr<AllocatedBuffer> sceneDataBuffer =
      std::make_shared<AllocatedBuffer>(engine_->allocator_);

  sceneDataBuffer->create(
      sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, "Scene::execute::sceneDataBuffer");

  GPUSceneData *data = reinterpret_cast<GPUSceneData *>(sceneDataBuffer->map());
  memcpy(data, &myScene.globalSceneData, sizeof(GPUSceneData));
  sceneDataBuffer->unmap();

  VkDescriptorSet sceneSet =
      frame._frameDescriptor.allocate(myScene.sceneDescriptorSetLayout_);
  DescriptorWriter scenewriter{engine_->device_};
  scenewriter.write_buffer(0, sceneDataBuffer->buffer, sizeof(GPUSceneData), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  scenewriter.update_set(sceneSet);

  return {sceneSet, sceneDataBuffer};
}

std::tuple<MaterialInstance, std::shared_ptr<AllocatedBuffer>>
ScenesManager::createDefaultMaterialInstance(FrameData &frame) {

  std::shared_ptr<AllocatedBuffer> materialBuffer =
      std::make_shared<AllocatedBuffer>(engine_->allocator_);
  materialBuffer->create(
      sizeof(MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU,
      "Scene::createDefaultMaterialInstance::MaterialConstants");

  MaterialConstants *constant =
      reinterpret_cast<MaterialConstants *>(materialBuffer->map());
  constant->colorFactors = glm::vec4{1, 1, 1, 1};
  constant->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};
  materialBuffer->unmap();

  MaterialResources materialResources;
  materialResources.colorImage = engine_->white_->getImageView();
  materialResources.colorSampler = engine_->defaultSamplerLinear_;
  materialResources.metalRoughImage = engine_->white_->getImageView();
  materialResources.metalRoughSampler = engine_->defaultSamplerLinear_;
  materialResources.materialConstantsData = materialBuffer->buffer;

  return {metalRoughMaterial->generate_instance(
              MaterialPass::OPAQUE, materialResources, frame._frameDescriptor),
          materialBuffer};
}

void ScenesManager::render(VkCommandBuffer cmd, FrameData &frame) {

  MeshAsset *last_mesh{};
  MaterialInstance *last_material{};
  MaterialPipeline *last_pipeline{};

  /*set = 0, binding = 0*/
  auto [sceneSet, sceneDataBuffer] = createSceneSet(frame);

  /*set = 1, binding = 0, 1, 2*/
  auto [defaultMateral, materialBuffer] = createDefaultMaterialInstance(frame);

  frame.destroy_by_deferred([sceneDataBuffer, materialBuffer]() {
    sceneDataBuffer->destroy();
    materialBuffer->destroy();
  });

  auto drawExtent = frame.getExtent2D();

  auto colorAttachmentInfo =
      tools::color_attachment_info(frame.drawImage_->imageView);
  auto depthAttachmentInfo =
      tools::depth_attachment_info(frame.depthImage_->imageView);
  auto renderInfo = tools::rendering_info(drawExtent, &colorAttachmentInfo,
                                          &depthAttachmentInfo);

  vkCmdBeginRendering(cmd, &renderInfo);

  // set dynamic viewport and scissor
  VkViewport viewport = {};
  viewport.width = static_cast<float>(drawExtent.width);
  viewport.height = static_cast<float>(drawExtent.height);
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.extent.width = drawExtent.width;
  scissor.extent.height = drawExtent.height;

  vkCmdSetScissor(cmd, 0, 1, &scissor);

  auto PV = myScene.globalSceneData.proj * myScene.globalSceneData.view;

  for (auto &surface : ctx.OpaqueSurfaces) {

    if (!surface.isVisible(PV)) {
      continue;
    }

    // No Material Set, Then use default
    if (!surface.material) {
      // setup default material
      surface.material = &defaultMateral;
    }

    auto pipeline = surface.material->pipeline->getPipeline();
    auto layout = surface.material->pipeline->getPipelineLayout();

    if (last_pipeline != surface.material->pipeline) {
      last_pipeline = surface.material->pipeline;
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0,
                              static_cast<uint32_t>(1), &sceneSet, 0, nullptr);
    }

    if (last_material != surface.material) {
      last_material = surface.material;
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1,
                              static_cast<uint32_t>(1),
                              &surface.material->materialSet, 0, nullptr);
    }

    // Its different from last mesh! So we should resubmit the vertices!
    if (last_mesh != surface.parent) {
      last_mesh = surface.parent;

      if (auto mesh =
              loadedScenes_.find("default")->second->findMesh("Suzanne");
          mesh) {
        vkCmdBindIndexBuffer(
            cmd, (*mesh)->meshBuffers.indexBuffer.buffer, 0,
            tools::getIndexType<decltype((*mesh)->meshBuffers.indicies_[0])>());
      }
    }

    GPUGeoPushConstants constants{};
    constants.matrix = surface.transform;
    constants.vertexBuffer = surface.vertexBufferAddress;

    vkCmdPushConstants(cmd, surface.material->pipeline->getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(GPUGeoPushConstants), &constants);

    vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);

    // Update Satistic Info
    engine_->stats.drawcall_count += 1;
    engine_->stats.triangle_count += surface.indexCount / 3;
  }

  vkCmdEndRendering(cmd);
}

void ScenesManager::compute(VkCommandBuffer cmd, FrameData &frame) {

  auto color_blending_background = [&]() {
    auto drawExtent = frame.getExtent2D();
    ImageAttachmentResources res{frame.drawImage_->imageView};

    auto ins =
        imageAttachmentCompute->generate_instance(res, frame._frameDescriptor);
    imageAttachmentCompute->set_dispatch_size((drawExtent.width + 15) >> 4,
                                              (drawExtent.height + 15) >> 4, 1);

    imageAttachmentCompute->dispatch(cmd, ins);
  };

  color_blending_background();
}

bool ScenesManager::addScene(std::shared_ptr<NodeManager> scene) {
  if (scene->name.empty()) {
    spdlog::warn(
        "[ScenesManager Warn]: No Scene Name Found! Create As default!");
  }
  auto [_, status] = loadedScenes_.try_emplace(scene->name, scene);
  if (!status) {
    spdlog::warn("[ScenesManager Warn]: Insert Scene Failed Due to Node "
                 "Already Existed!");
  }
  return status;
}

ComputeShaderPushConstants &ScenesManager::getComputeData() {
  return imageAttachmentCompute->getPushConstantData();
}

void ScenesManager::submit() {
  engine_->imm_command_submit([this](VkCommandBuffer cmd) { submitMesh(cmd); });

  flushUpload(engine_->immFence_);
}

void ScenesManager::Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {
  for (auto &[scene_name, scene] : loadedScenes_) {
    scene->Draw(parentMatrix, ctx);
  }
}
} // namespace engine
