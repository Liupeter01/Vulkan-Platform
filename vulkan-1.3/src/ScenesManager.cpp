#include <AllocatedTexture.hpp>
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

  create_scene_set();

  init_default_compute();
  init_particle_sys();
  isinit = true;
}

void ScenesManager::destroy() {
  if (isinit) {
    destroy_scene();
    myScene.sceneDescriptorSetLayout_ = VK_NULL_HANDLE;
    destroy_default_compute();
    destroy_particle_sys();
    destroy_default_material();

    destroy_scene_set();

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

  update_scene_set();
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

  defaultMaterial.materialBuffer.reset();
  defaultMaterial.materialBuffer =
      std::make_shared<::engine::v1::AllocatedBuffer>(engine_->allocator_);

  if (!defaultMaterial.materialBuffer) {
    spdlog::error("[ScenesManager Error]: Create Default Graphic Material "
                  "Buffer Failed!");
    throw std::runtime_error("Alloc Default Graphic Material Buffer Failed!");
  }

  defaultMaterial.materialBuffer->create(
      sizeof(MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU,
      "Scene::createDefaultMaterialInstance::MaterialConstants");

  MaterialConstants *constant = reinterpret_cast<MaterialConstants *>(
      defaultMaterial.materialBuffer->map());
  constant->colorFactors = glm::vec4{1, 1, 1, 1};
  constant->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};
  defaultMaterial.materialBuffer->unmap();

  MaterialResources materialResources;
  materialResources.colorImage = engine_->white_->imageView();
  materialResources.colorSampler = engine_->defaultSamplerLinear_;
  materialResources.metalRoughImage = engine_->white_->imageView();
  materialResources.metalRoughSampler = engine_->defaultSamplerLinear_;
  materialResources.materialConstantsData =
      defaultMaterial.materialBuffer->buffer;

  defaultMaterial.defaultMateral = metalRoughMaterial->generate_instance(
      MaterialPass::OPAQUE, materialResources, scenePool_);
}

void ScenesManager::destroy_default_material() {

  defaultMaterial.defaultMateral = MaterialInstance{};
  defaultMaterial.materialBuffer->destroy();
  defaultMaterial.materialBuffer.reset();

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
  particleSysBuffer =
      std::make_unique<ParticleSysDataBuffer<particle::GPUParticle>>(
          engine_->device_, engine_->allocator_);
  particleSysBuffer->configure(8192 * 2, ParticleSysDataBuffer<particle::GPUParticle>::ParticleDimension::Particle2D);
  particleSysBuffer->perpareTransferData();

  particleSysCompute.reset();
  particleSysCompute =
      std::make_shared<particle2d::PointSpriteParticleSystem2D<>>(
          engine_->device_);
  if (!particleSysCompute) {
    spdlog::error("[ScenesManager Error]: Create Particle System Compute "
                  "Material Failed!");
    throw std::runtime_error("Alloc Particle System Compute Material Failed!");
  }
  particleSysCompute->setGlobalLayout(myScene.sceneDescriptorSetLayout_,
                                      myScene.sceneDescriptorSet);
  particleSysCompute->init();
}

void ScenesManager::on_gui() {
  if (imageAttachmentCompute)
    imageAttachmentCompute->on_gui();

  if (particleSysCompute)
    particleSysCompute->on_gui();
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

void ScenesManager::update_scene_set() {

  if (!myScene.sceneDataBuffer) {
    throw std::runtime_error("Invalid Scene Data Buffer!");
  }

  mesh::GPUSceneData *data =
      reinterpret_cast<mesh::GPUSceneData *>(myScene.sceneDataBuffer->map());
  memcpy(data, &myScene.globalSceneData, sizeof(mesh::GPUSceneData));
  myScene.sceneDataBuffer->unmap();
}

VkDescriptorSet ScenesManager::get_scene_set() {
  DescriptorWriter scenewriter{engine_->device_};
  scenewriter.write_buffer(0, myScene.sceneDataBuffer->buffer,
                           sizeof(mesh::GPUSceneData), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  return myScene.sceneDescriptorSet;
}

void ScenesManager::create_scene_set() {
  if (myScene.sceneDataBuffer) {
    myScene.sceneDataBuffer->destroy();
  }
  myScene.sceneDataBuffer.reset();
  myScene.sceneDataBuffer =
      std::make_shared<::engine::v1::AllocatedBuffer>(engine_->allocator_);

  myScene.sceneDataBuffer->create(
      sizeof(mesh::GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, "Scene::execute::sceneDataBuffer");

  mesh::GPUSceneData *data =
      reinterpret_cast<mesh::GPUSceneData *>(myScene.sceneDataBuffer->map());
  memcpy(data, &myScene.globalSceneData, sizeof(mesh::GPUSceneData));
  myScene.sceneDataBuffer->unmap();

  VkDescriptorSet sceneSet =
      scenePool_.allocate(myScene.sceneDescriptorSetLayout_);
  DescriptorWriter scenewriter{engine_->device_};
  scenewriter.write_buffer(0, myScene.sceneDataBuffer->buffer,
                           sizeof(mesh::GPUSceneData), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  scenewriter.update_set(sceneSet);

  myScene.sceneDescriptorSet = sceneSet;
}

void ScenesManager::destroy_scene_set() {
  if (myScene.sceneDataBuffer) {
    myScene.sceneDataBuffer->destroy();
  }
  myScene.sceneDataBuffer.reset();
}

void ScenesManager::transfer(VkCommandBuffer cmd,
                             std::unique_ptr<CommonFrameContext> &frame) {

  std::call_once(
      transfer_once_,
      [this, cmd](uint64_t value) {
        engine_->submit_default_color(value, cmd);
        particleSysBuffer->setUploadCompleteTimeline(value);
        particleSysBuffer->singleTimeUpload(cmd);
      },
      frame->parent_->transferSignalValue_);

  auto PV = myScene.globalSceneData.proj * myScene.globalSceneData.view;

  for (auto &surface : ctx.OpaqueSurfaces) {

    std::shared_ptr<mesh::v2::MeshAsset2> obj = surface.parent.lock();

    if (!obj) {
      continue;
    }

    if (!surface.isVisible(PV)) {
      // Maybe LRU in the future
      continue;
    }

    const auto vertexState = obj->getVertexBuffer().state();
    const auto indexState = obj->getIndexBuffer().state();

    if (((vertexState == v2::ResourceState::CpuOnly) &&
         (indexState == v2::ResourceState::CpuOnly)) ||
        ((vertexState == v2::ResourceState::UnInstalled) &&
         (indexState == v2::ResourceState::UnInstalled))) {

      obj->setUploadCompleteTimeline(frame->parent_->transferSignalValue_);
      obj->recordUpload(cmd);
    }

    if (!surface.material->texture) {
      continue;
    }

    const auto textureState = surface.material->texture->state();
    if ((textureState == v2::ResourceState::CpuOnly) ||
        (textureState == v2::ResourceState::UnInstalled)) {

      surface.material->texture->setUploadCompleteTimeline(
          frame->parent_->transferSignalValue_);
      surface.material->texture->recordUpload(cmd);
    }
  }
}

void ScenesManager::render(VkCommandBuffer cmd,
                           std::unique_ptr<CommonFrameContext> &frame) {

  ::engine::mesh::v2::MeshAsset2 *last_mesh{};
  MaterialInstance *last_material{};
  MaterialPipeline *last_pipeline{};

  /*set = 0, binding = 0*/
  auto sceneSet = get_scene_set();

  auto drawExtent = frame->parent_->getExtent2D();

  auto colorAttachmentInfo =
      tools::color_attachment_info(frame->parent_->drawImage_->imageView);
  auto depthAttachmentInfo =
      tools::depth_attachment_info(frame->parent_->depthImage_->imageView);
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

  /*deal with other systems*/
  if (particleSysCompute->has_graphic()) {
    const std::size_t dispatchGroupsX =
        particleSysBuffer->getParticleCount() >> 8;
    if (!dispatchGroupsX) {
      spdlog::info("[ParticleMovement] Dispatching {} groups for {} particles",
                   dispatchGroupsX, particleSysBuffer->getParticleCount());
    }

    particle::ParticleResources res;
    res.bufferSize = particleSysBuffer->getBufferSize();
    res.particlesIn = particleSysBuffer->get_in_buffer();
    res.particlesOut = particleSysBuffer->get_out_buffer();

    auto ins =
        particleSysCompute->generate_instance(res, frame->_frameDescriptor);
    particleSysCompute->set_dispatch_size(dispatchGroupsX, 1, 1);
    particleSysCompute->render(cmd, ins);
  }

  std::call_once(
      graphic_once_,
      [this, cmd, &frame](uint64_t value) {
        init_default_material();

        frame->destroy_by_deferred([value, eng = engine_]() {
          if (eng) {
            eng->purge_remove_color_staging(value);
          }
        });
      },
      frame->parent_->graphicsWaitValue_);

  auto PV = myScene.globalSceneData.proj * myScene.globalSceneData.view;
  for (auto &surface : ctx.OpaqueSurfaces) {

    std::shared_ptr<mesh::v2::MeshAsset2> obj = surface.parent.lock();

    if (!obj) {
      continue;
    }

    if (!surface.isVisible(PV)) {
      continue;
    }

    const auto vertexState = obj->getVertexBuffer().state();
    const auto indexState = obj->getIndexBuffer().state();

    if ((vertexState == v2::ResourceState::UploadScheduled) ||
        (indexState == v2::ResourceState::UploadScheduled)) {

      obj->updateUploadingStatus(frame->parent_->graphicsWaitValue_);

      frame->destroy_by_deferred(
          [obj, value = frame->parent_->graphicsWaitValue_]() {
            obj->purgeReleaseStaging(value);
          });
    }

    if (surface.material->texture) {
      const auto textureState = surface.material->texture->state();
      if (textureState == v2::ResourceState::UploadScheduled) {

        DescriptorWriter scenewriter{engine_->device_};
        scenewriter.write_image(1, surface.material->texture->imageView(),
                                surface.material->samplers,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        surface.material->texture->updateUploadingStatus(
            frame->parent_->graphicsWaitValue_);

        frame->destroy_by_deferred(
            [obj = surface.material->texture,
             value = frame->parent_->graphicsWaitValue_]() {
              obj->purgeReleaseStaging(value);
            });
      }
    }

    // No Material Set, Then use default
    if (!surface.material) {
      // setup default material   (set = 1, binding = 0, 1, 2)*/
      surface.material = &defaultMaterial.defaultMateral;
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
    if (last_mesh != obj.get()) {
      last_mesh = obj.get();

      vkCmdBindIndexBuffer(
          cmd, obj->meshBuffers.indexBuffer_.buffer(), 0,
          tools::getIndexType<decltype(obj->meshBuffers.indicies_[0])>());
    }

    mesh::GPUGeoPushConstants constants{};
    constants.matrix = surface.transform;
    constants.vertexBuffer = obj->getVertexDeviceAddr();

    vkCmdPushConstants(cmd, surface.material->pipeline->getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(mesh::GPUGeoPushConstants), &constants);

    vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);

    // Update Satistic Info
    engine_->stats.drawcall_count += 1;
    engine_->stats.triangle_count += surface.indexCount / 3;
  }

  vkCmdEndRendering(cmd);
}

void ScenesManager::compute(VkCommandBuffer cmd,
                            std::unique_ptr<CommonFrameContext> &frame) {

          std::call_once(
                   compute_once_,
                    [this, cmd, &frame](uint64_t value) {
                              particleSysBuffer->updateUploadingStatus(value);

                              frame->destroy_by_deferred(
                                        [this,
                                        value = frame->parent_->graphicsWaitValue_]() {
                                                  particleSysBuffer->purgeReleaseStaging(value);
                                        });
                    },
                    frame->parent_->computeWaitValue_);

  // auto color_blending_background = [&]() {
  //   auto drawExtent = frame.getExtent2D();
  //   ImageAttachmentResources res{frame.drawImage_->imageView};

  //  auto ins =
  //      imageAttachmentCompute->generate_instance(res,
  //      frame._frameDescriptor);
  //  imageAttachmentCompute->set_dispatch_size((drawExtent.width + 15) >> 4,
  //                                            (drawExtent.height + 15) >> 4,
  //                                            1);

  //  imageAttachmentCompute->dispatch(cmd, ins);
  //};

  auto particle_movement = [&]() {
    if (particleSysCompute->has_compute()) {
      const std::size_t dispatchGroupsX =
          particleSysBuffer->getParticleCount() >> 8;
      if (!dispatchGroupsX) {
        spdlog::info(
            "[ParticleMovement] Dispatching {} groups for {} particles",
            dispatchGroupsX, particleSysBuffer->getParticleCount());
      }

      particle::ParticleResources res;
      res.bufferSize = particleSysBuffer->getBufferSize();
      res.particlesIn = particleSysBuffer->get_in_buffer();
      res.particlesOut = particleSysBuffer->get_out_buffer();
      // res.colorImage = frame.drawImage_->imageView;

      auto ins =
          particleSysCompute->generate_instance(res, frame->_frameDescriptor);
      particleSysCompute->set_dispatch_size(dispatchGroupsX, 1, 1);
      particleSysCompute->dispatch(cmd, ins);

      particleSysBuffer->swap();
    }
  };

  // color_blending_background();
  particle_movement();
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

void ScenesManager::Draw(const glm::mat4 &parentMatrix, DrawContext &ctx) {
  for (auto &[scene_name, scene] : loadedScenes_) {
    scene->Draw(parentMatrix, ctx);
  }
}
} // namespace engine
