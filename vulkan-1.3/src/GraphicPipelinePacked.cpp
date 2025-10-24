#include <Tools.hpp>
#include <algorithm>
#include <iostream>
#include <pipeline/GraphicPipelinePacked.hpp>

namespace engine {
namespace graphic {

GraphicPipelinePacked::GraphicPipelinePacked(VkDevice device,
                                             VmaAllocator allocator)
    : device_(device), allocator_(allocator), metalRoughMaterial(device) {}

GraphicPipelinePacked::~GraphicPipelinePacked() { destroy(); }

void GraphicPipelinePacked::init(VkDescriptorSetLayout sceneDescriptorSetLayout) {
  if (isInit_)
    return;

  init_layout();

  metalRoughMaterial.init(sceneDescriptorSetLayout_);

  init_default_colors();
  init_sampler();

  sceneData_.proj[1][1] *= -1;
  isInit_ = true;
}

void GraphicPipelinePacked::destroy() {

  if (isInit_) {

    destroy_sampler();
    destroy_default_colors();
    destroy_layout();

    metalRoughMaterial.destory();

    isInit_ = false;
  }
}

void GraphicPipelinePacked::draw(VkExtent2D drawExtent,
                                 AllocatedImage &offscreen_draw,
                                 AllocatedImage &offscreen_depth,
                                 FrameData &currentFrame) {

  // now that we are sure that the commands finished executing, we can safely
  VkCommandBuffer cmd = currentFrame._mainCommandBuffer;

  /*set = 0, binding = 0*/
  std::shared_ptr<AllocatedBuffer> sceneDataBuffer =
      std::make_shared<AllocatedBuffer>(allocator_);

  sceneDataBuffer->create(
      sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, "GraphicPipeline::Draw::SceneDataBuffer");

  GPUSceneData *data = reinterpret_cast<GPUSceneData *>(sceneDataBuffer->map());
  *data = sceneData_;
  sceneDataBuffer->unmap();

  /*set = 1, binding = 0*/
  std::shared_ptr<AllocatedBuffer> materialBuffer =
      std::make_shared<AllocatedBuffer>(allocator_);
  materialBuffer->create(
      sizeof(MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, "GraphicPipeline::Draw::MaterialConstants");

  MaterialConstants *constant =
      reinterpret_cast<MaterialConstants *>(materialBuffer->map());
  constant->colorFactors = glm::vec4{1, 1, 1, 1};
  constant->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};
  materialBuffer->unmap();

  MaterialResources materialResources;
  materialResources.colorImage = white_->getImageView();
  materialResources.colorSampler = defaultSamplerLinear_;
  materialResources.metalRoughImage = white_->getImageView();
  materialResources.metalRoughSampler = defaultSamplerLinear_;
  materialResources.materialConstantsData = materialBuffer->buffer;

  MaterialInstance ins = metalRoughMaterial.generate_instance(
      MaterialPass::OPAQUE, materialResources, currentFrame._frameDescriptor);

  currentFrame.destroy_by_deferred([sceneDataBuffer, materialBuffer]() {
    sceneDataBuffer->destroy();
    materialBuffer->destroy();
  });

  VkDescriptorSet sceneSet = currentFrame.allocate(sceneDescriptorSetLayout_);
  DescriptorWriter scenewriter{device_};
  scenewriter.write_buffer(0, sceneDataBuffer->buffer, sizeof(GPUSceneData), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  scenewriter.update_set(sceneSet);

  std::vector<VkDescriptorSet> descriptorSets{sceneSet, ins.materialSet};

  auto colorAttachmentInfo =
      tools::color_attachment_info(offscreen_draw.imageView);
  auto depthAttachmentInfo =
      tools::depth_attachment_info(offscreen_depth.imageView);
  auto renderInfo = tools::rendering_info(drawExtent, &colorAttachmentInfo,
                                          &depthAttachmentInfo);

  vkCmdBeginRendering(cmd, &renderInfo);

  // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ins.pipeline->getPipeline());

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

  GPUGeoPushConstants constants{};
  constants.matrix = {1.f};
  constants.vertexBuffer = meshes_["Suzanne"]->getVertexDeviceAddr();

  vkCmdPushConstants(cmd, ins.pipeline->getPipelineLayout(),
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUGeoPushConstants),
                     &constants);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          ins.pipeline->getPipelineLayout(), 0,
                          static_cast<uint32_t>(descriptorSets.size()),
                          descriptorSets.data(), 0, nullptr);

  if (meshes_["Suzanne"]->meshBuffers.indicies_.empty()) {
    assert(!meshes_["Suzanne"]->meshBuffers.indicies_.empty() &&
           "Index buffer is empty!");
  }

  vkCmdBindIndexBuffer(
      cmd, meshes_["Suzanne"]->meshBuffers.indexBuffer.buffer, 0,
      tools::getIndexType<
          decltype(meshes_["Suzanne"]->meshBuffers.indicies_[0])>());

  for (const GeoSurface &surface : meshes_["Suzanne"]->meshSurfaces) {
    vkCmdDrawIndexed(cmd,
                     surface.count,      // index
                     1,                  // instance
                     surface.startIndex, // index
                     0,                  // vertex
                     0                   // instance
    );
  }

  vkCmdEndRendering(cmd);
}

void GraphicPipelinePacked::submitMesh(VkCommandBuffer cmd) {

  meshes_["Suzanne"]->submitMesh(cmd);
}

void GraphicPipelinePacked::submitColorImage(VkCommandBuffer cmd) {

  black_->uploadBufferToImage(cmd);
  white_->uploadBufferToImage(cmd);
  grey_->uploadBufferToImage(cmd);
  magenta_->uploadBufferToImage(cmd);
  loaderrorImage_->uploadBufferToImage(cmd);
}

void GraphicPipelinePacked::flushUpload(VkFence fence) {

  meshes_["Suzanne"]->flushUpload(fence);

  black_->flushUpload(fence);
  white_->flushUpload(fence);
  grey_->flushUpload(fence);
  magenta_->flushUpload(fence);
  loaderrorImage_->flushUpload(fence);
}

std::function<void(VkCommandBuffer)> GraphicPipelinePacked::getMeshFunctor() {
  return [this](VkCommandBuffer cmd) { submitMesh(cmd); };
}
std::function<void(VkCommandBuffer)> GraphicPipelinePacked::getColorFunctor() {
  return [this](VkCommandBuffer cmd) { submitColorImage(cmd); };
}

void GraphicPipelinePacked::init_layout() {
  sceneDescriptorSetLayout_ = create_ubo_layout();
}

void GraphicPipelinePacked::destroy_layout() {
  vkDestroyDescriptorSetLayout(device_, sceneDescriptorSetLayout_, nullptr);
}

VkDescriptorSetLayout GraphicPipelinePacked::create_ubo_layout() {
  return DescriptorLayoutBuilder{device_}
      .add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
      .build(VK_SHADER_STAGE_VERTEX_BIT |
             VK_SHADER_STAGE_FRAGMENT_BIT); // add bindings
}

void GraphicPipelinePacked::load_asset(std::shared_ptr<MeshAsset> asset) {

  if (!asset)
    return;

  auto [_, status] = meshes_.try_emplace(asset->meshName, asset);
  if (!status) {
    std::cerr << "Create Asset: " << asset->meshName << " Failed!\n";
  }
}

void GraphicPipelinePacked::load_asset(
          const std::vector<std::shared_ptr<MeshAsset>>& assets) {

  std::for_each(
      assets.begin(), assets.end(),
      [this](std::shared_ptr<MeshAsset> asset) { load_asset(asset); });
}

void GraphicPipelinePacked::destroy_default_colors() {
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

void GraphicPipelinePacked::init_sampler() {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  vkCreateSampler(device_, &samplerInfo, nullptr, &defaultSamplerNearest_);

  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  vkCreateSampler(device_, &samplerInfo, nullptr, &defaultSamplerLinear_);
}

void GraphicPipelinePacked::destroy_sampler() {
  vkDestroySampler(device_, defaultSamplerNearest_, nullptr);
  vkDestroySampler(device_, defaultSamplerLinear_, nullptr);
}

void GraphicPipelinePacked::init_default_colors() {

  white_.reset();
  grey_.reset();
  black_.reset();
  magenta_.reset();
  loaderrorImage_.reset();

  white_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  grey_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  black_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  magenta_ = std::make_unique<AllocatedTexture>(device_, allocator_);
  loaderrorImage_ = std::make_unique<AllocatedTexture>(device_, allocator_);

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

  white_->createBuffer(reinterpret_cast<void *>(&white), VkExtent3D{1, 1, 1},
                       VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  grey_->createBuffer(reinterpret_cast<void *>(&grey), VkExtent3D{1, 1, 1},
                      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  black_->createBuffer(reinterpret_cast<void *>(&black), VkExtent3D{1, 1, 1},
                       VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  magenta_->createBuffer(reinterpret_cast<void *>(&magenta),
                         VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_USAGE_SAMPLED_BIT);

  loaderrorImage_->createBuffer(reinterpret_cast<void *>(pixels.data()),
                                VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_USAGE_SAMPLED_BIT);
}

} // namespace graphic
} // namespace engine
