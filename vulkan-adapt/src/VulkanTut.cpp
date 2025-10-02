#include <Buffer.hpp>
#include <Camera.hpp>
#include <GlobalDef.hpp>
#include <Keyboard_Controller.hpp>
#include <VulkanTut.hpp>
#include <chrono>
#include <numeric>
#include <system/PointLightSystem.hpp>
#include <system/TutRenderSystem.hpp>

engine::VulkanTut::VulkanTut() {
  globalPool_.reset();
  globalPool_ = DescriptorPool::Builder(device_)
                    .setMaxSets(MyEngineSwapChain::MAX_FRAMES_IN_FLIGHT)
                    .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 MyEngineSwapChain::MAX_FRAMES_IN_FLIGHT)
                    .build();

  loadGameObjects();
}

engine::VulkanTut::~VulkanTut() {}

void engine::VulkanTut::run() {

  std::vector<std::unique_ptr<Buffer>> uboBuffers(
      MyEngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (std::size_t i = 0; i < uboBuffers.size(); ++i) {
    uboBuffers[i] = std::make_unique<Buffer>(
        device_, sizeof(GlobalUBO), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        device_.properties.limits.minUniformBufferOffsetAlignment);

    uboBuffers[i]->map();
  }

  /*SUPPORT ALL STAGE SHADER!*/
  auto globalSetLayout =
      DescriptorSetLayout::Builder(device_)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL)

          .build();

  std::vector<VkDescriptorSet> globalDescriptorSets(
      MyEngineSwapChain::MAX_FRAMES_IN_FLIGHT);

  for (std::size_t i = 0; i < globalDescriptorSets.size(); ++i) {
    auto buffer = uboBuffers[i]->descriptorInfo();
    DescriptorWriter(*globalSetLayout, *globalPool_)
        .writeBuffer(0, &buffer)
        .build(globalDescriptorSets[i]);
  }

  TutRenderSystem tutRenderSystem{device_, render_.getSwapChainRenderPass(),
                                  globalSetLayout->getDescriptorSetLayout()};

  PointLightSystem pointLightSystem{device_, render_.getSwapChainRenderPass(),
                                    globalSetLayout->getDescriptorSetLayout()};

  Camera camera;
  // camera.setViewDirection(glm::vec3{ 0.f }, glm::vec3(0.5f, 0.f, 1.f));
  camera.setViewTarget(glm::vec3{0.f}, glm::vec3(0.f, 0.f, 0.f));

  // We take camera as a object and move its position
  auto viewObject = GameObject::createGameObject();
  viewObject.transform.translation.z = -1.5f;
  KeyBoardController cameraController{};

  auto currTime = std::chrono::high_resolution_clock::now();

  while (!window_.shouldClose()) {
    glfwPollEvents();

    auto nowTime = std::chrono::high_resolution_clock::now();
    auto timeFrame = std::chrono::duration<float, std::chrono::seconds::period>(
                         nowTime - currTime)
                         .count();
    currTime = nowTime;

    float aspect = render_.getAspectRatio();
    // camera.setOrthoProjection(-aspect, aspect, /*DO NOT CHANGE*/-1, /*DO NOT
    // CHANGE*/1, 0.1f, 100);
    camera.setPerspectiveProjection(glm::radians(45.f), aspect, 0.1f, 100.f);

    cameraController.movePlaneYXZ(window_.getGLFWWindow(), timeFrame,
                                  viewObject);
    camera.setYXZ(viewObject.transform.translation,
                  viewObject.transform.rotation);

    if (auto commandBuffer = render_.beginFrame()) {

      const auto frameIdx = render_.getFrameIndex();
      FrameConfigInfo info{timeFrame, commandBuffer, gameObject_,
                           globalDescriptorSets[frameIdx], camera};

      /*uniform*/
      GlobalUBO ubo;
      ubo.P = camera.getProjectionMatrix();
      ubo.V = camera.getViewMatrix();
      ubo.inv_V = camera.getInverseViewMatrix();

      pointLightSystem.updateLightObjOnly(info, ubo);

      uboBuffers[frameIdx]->writeToBuffer((void *)&ubo);
      uboBuffers[frameIdx]->flush(); // must flush

      /*render*/
      render_.beginSwapChainRenderPass(commandBuffer);

      tutRenderSystem.renderGameObject(info);
      pointLightSystem.renderGameObject(info);

      render_.endSwapChainRenderPass(commandBuffer);
      render_.endFrame();
    }
  }

  vkDeviceWaitIdle(device_.device());
}

void engine::VulkanTut::loadGameObjects() {

  auto floorObj = GameObject::createGameObject();
  auto smoothObj = GameObject::createGameObject();
  auto flatObj = GameObject::createGameObject();

  std::vector<glm::vec3> lightColors{
      {1.f, .1f, .1f}, {.1f, .1f, 1.f}, {.1f, 1.f, .1f},
      {1.f, 1.f, .1f}, {.1f, 1.f, 1.f}, {1.f, 1.f, 1.f} //
  };

  for (std::size_t i = 0; i < lightColors.size(); ++i) {
    auto pointLightObj = GameObject::createPointLight(0.5f, 0.1f);
    pointLightObj.color = lightColors[i];

    auto R = glm::rotate(glm::one<glm::mat4>(),
                         i * glm::two_pi<float>() / lightColors.size(),
                         glm::vec3{0.f, -1.f, 0.f});

    pointLightObj.transform.translation =
        glm::vec3(R * glm::vec4(-1.f, -1.f, -1.f, 1.f));

    gameObject_.try_emplace(pointLightObj.getId(), std::move(pointLightObj));
  }

  floorObj.model_ =
      std::make_shared<Model>(device_, CONFIG_HOME "assets/objs/floor.obj");
  floorObj.transform.translation = {0.f, 0.5f, .1f};
  floorObj.transform.scale = glm::vec3{2.f};

  flatObj.model_ =
      std::make_shared<Model>(device_, CONFIG_HOME "assets/objs/flat_vase.obj");
  flatObj.transform.translation = {0.5f, 0.5f, .1f};
  flatObj.transform.scale = glm::vec3{2.f};

  smoothObj.model_ = std::make_shared<Model>(device_, CONFIG_HOME
                                             "assets/objs/smooth_vase.obj");
  smoothObj.transform.translation = {-0.5f, 0.5f, .1f};
  smoothObj.transform.scale = glm::vec3{2.f};

  gameObject_.try_emplace(flatObj.getId(), std::move(flatObj));
  gameObject_.try_emplace(smoothObj.getId(), std::move(smoothObj));
  gameObject_.try_emplace(floorObj.getId(), std::move(floorObj));
}
