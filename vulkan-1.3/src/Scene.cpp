#include <Descriptors.hpp>
#include <Tools.hpp>
#include <VulkanEngine.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <scene/Scene.hpp>
#include <spdlog/spdlog.h>

namespace engine {
Scene::Scene(VulkanEngine *pointer) : engine(pointer) {}

Scene::~Scene() { destroy(); }

void Scene::init(const std::string &root_name) {
  if (isInit)
    return;
  myScene.sceneDescriptorSetLayout_ = engine->sceneDescriptorSetLayout_;
  init_material();
  init_compute();
  node_mgr.init();
  isInit = true;
}

void Scene::destroy() {
  if (isInit) {
    node_mgr.destroy();
    myScene.sceneDescriptorSetLayout_ = VK_NULL_HANDLE;
    destroy_compute();
    destroy_material();
    isInit = false;
  }
}

void Scene::submit() {
  engine->imm_command_submit([this](VkCommandBuffer cmd) {
    submitColorImage(cmd);
    submitMesh(cmd);
  });

  flushUpload(engine->immFence_);
}

void Scene::init_material() {
  metalRoughMaterial.reset();
  metalRoughMaterial =
      std::make_unique<GLTFMetallic_Roughness>(engine->device_);
  metalRoughMaterial->init(myScene.sceneDescriptorSetLayout_);
}

void Scene::destroy_material() {
  metalRoughMaterial->destory();
  metalRoughMaterial.reset();
}

void Scene::init_compute() {
  imageAttachmentCompute.reset();
  imageAttachmentCompute =
      std::make_unique<Compute_ImageAttachment>(engine->device_);
  imageAttachmentCompute->init();
}

void Scene::destroy_compute() {
  imageAttachmentCompute->destory();
  imageAttachmentCompute.reset();
}

void Scene::submitMesh(VkCommandBuffer cmd) {

  if (auto mesh = node_mgr.findMesh("Suzanne"); mesh) {
    (*mesh)->submitMesh(cmd);
  }
}

void Scene::submitColorImage(VkCommandBuffer cmd) {
  engine->black_->uploadBufferToImage(cmd);
  engine->white_->uploadBufferToImage(cmd);
  engine->grey_->uploadBufferToImage(cmd);
  engine->magenta_->uploadBufferToImage(cmd);
  engine->loaderrorImage_->uploadBufferToImage(cmd);
}

void Scene::flushUpload(VkFence fence) {

  engine->black_->flushUpload(fence);
  engine->white_->flushUpload(fence);
  engine->grey_->flushUpload(fence);
  engine->magenta_->flushUpload(fence);
  engine->loaderrorImage_->flushUpload(fence);

  if (auto mesh = node_mgr.findMesh("Suzanne"); mesh) {
    (*mesh)->flushUpload(fence);
  }
}

void Scene::update_scene() {

  ctx.OpaqueSurfaces.clear();

  for (float x = 0.f; x < 0.6f; x += 0.6f) {

    glm::mat4 scale = glm::scale(glm::mat4{1.f}, glm::vec3{0.3f});
    glm::mat4 translation = glm::translate(glm::mat4{1.f}, glm::vec3{x, 0, 0});

    // Execute Draw Command From Root Node!
    node_mgr.Draw(translation * scale, ctx);
  }

  // Update Camera
  myScene.globalSceneData.view = engine->camera_->getViewMatrix();
  myScene.globalSceneData.proj = engine->camera_->getProjectionMatrix();
  myScene.globalSceneData.proj[1][1] *= -1; // Reverse Y
}

std::tuple<VkDescriptorSet, std::shared_ptr<AllocatedBuffer>>
Scene::createSceneSet(FrameData &frame) {

  std::shared_ptr<AllocatedBuffer> sceneDataBuffer =
      std::make_shared<AllocatedBuffer>(engine->allocator_);

  sceneDataBuffer->create(
      sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, "Scene::execute::sceneDataBuffer");

  GPUSceneData *data = reinterpret_cast<GPUSceneData *>(sceneDataBuffer->map());
  memcpy(data, &myScene.globalSceneData, sizeof(GPUSceneData));
  sceneDataBuffer->unmap();

  VkDescriptorSet sceneSet =
      frame._frameDescriptor.allocate(myScene.sceneDescriptorSetLayout_);
  DescriptorWriter scenewriter{engine->device_};
  scenewriter.write_buffer(0, sceneDataBuffer->buffer, sizeof(GPUSceneData), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  scenewriter.update_set(sceneSet);

  return {sceneSet, sceneDataBuffer};
}

std::tuple<MaterialInstance, std::shared_ptr<AllocatedBuffer>>
Scene::createDefaultMaterialInstance(FrameData &frame) {

  std::shared_ptr<AllocatedBuffer> materialBuffer =
      std::make_shared<AllocatedBuffer>(engine->allocator_);
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
  materialResources.colorImage = engine->white_->getImageView();
  materialResources.colorSampler = engine->defaultSamplerLinear_;
  materialResources.metalRoughImage = engine->white_->getImageView();
  materialResources.metalRoughSampler = engine->defaultSamplerLinear_;
  materialResources.materialConstantsData = materialBuffer->buffer;

  return {metalRoughMaterial->generate_instance(
              MaterialPass::OPAQUE, materialResources, frame._frameDescriptor),
          materialBuffer};
}

void Scene::render(VkCommandBuffer cmd, FrameData &frame) {

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

  for (auto &surface : ctx.OpaqueSurfaces) {

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

      if (auto mesh = node_mgr.findMesh(surface.mesh_name); mesh) {
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
    engine->stats.drawcall_count += 1;
    engine->stats.triangle_count += surface.indexCount / 3;
  }

  vkCmdEndRendering(cmd);
}

void Scene::compute(VkCommandBuffer cmd, FrameData &frame) {

  auto drawExtent = frame.getExtent2D();
  ComputeResources res{frame.drawImage_->imageView};

  auto ins =
      imageAttachmentCompute->generate_instance(res, frame._frameDescriptor);

  vkCmdBindPipeline(cmd, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE,
                    ins.pipeline->getPipeline());

  vkCmdPushConstants(
      cmd, ins.pipeline->getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
      sizeof(ComputeShaderPushConstants), &myScene.computeShaderData);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                          ins.pipeline->getPipelineLayout(), 0, 1,
                          &ins.computeSet, 0, nullptr);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so
  // we need to divide by it
  vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(drawExtent.width / 16.0f)),
                static_cast<uint32_t>(std::ceil(drawExtent.height / 16.0f)), 1);
}

bool Scene::attachChildren(const std::string &parentName,
                           std::shared_ptr<MeshAsset> asset) {
  return node_mgr.attachChildren(parentName, asset);
}
bool Scene::attachChildren(const std::string &parentName,
                           std::shared_ptr<node::BaseNode> child) {
  return node_mgr.attachChildren(parentName, child);
}
bool Scene::attachChildrens(
    const std::string &parentName,
    const std::vector<std::shared_ptr<node::BaseNode>> &childrens) {
  return node_mgr.attachChildrens(parentName, childrens);
}
bool Scene::attachChildrens(
    const std::string &parentName,
    const std::vector<std::shared_ptr<MeshAsset>> &childrens) {
  return node_mgr.attachChildrens(parentName, childrens);
}

ComputeShaderPushConstants &Scene::getComputeData() {
  return myScene.computeShaderData;
}

} // namespace engine
