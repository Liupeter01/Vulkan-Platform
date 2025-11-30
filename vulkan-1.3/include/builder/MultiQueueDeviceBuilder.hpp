#pragma once
#ifndef _MULTI_QUEUE_DEVICE_BUILDER_HPP_
#define _MULTI_QUEUE_DEVICE_BUILDER_HPP_
#include <VkBootstrap.h>
#include <list>
#include <queue>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace engine {
struct QueueSchedulerBuilder;
struct MultiQueueDeviceBuilder {
  friend struct QueueSchedulerBuilder;

  struct QueueBlock {
    QueueBlock(std::size_t index, VkQueueFamilyProperties prot);
    uint32_t familyIndex; // Vulkan QueueFamily Index
    uint32_t
        startPos{}; //  position index in current queue family, usually be zero!
    uint32_t count{}; //
    const uint32_t original;
    VkQueueFlags flags; //
  };

  struct QueueRequest {
    QueueRequest(VkQueueFlagBits type = {}) : queueType(type) {}
    VkQueueFlagBits queueType{}; // GRAPHICS / COMPUTE / TRANSFER
    uint32_t requested = 1;      // how many resources does the user want?
    uint32_t allocated{};
    bool reserveImgui = false; // FOR GRAPHIC ONLY, reseve imgui?
    bool presentQueueStatus = false;

    struct Span {
      uint32_t startPos{}; //  position index in current queue family, usually
                           //  be zero!
      uint32_t count{};
    };

    std::vector<std::pair</*familyIndex*/ uint32_t, Span>> allocatedQueueInfo;
  };

  static int score(const VkQueueFlags f);

  MultiQueueDeviceBuilder(vkb::DeviceBuilder &builder,
                          vkb::PhysicalDevice &pdev);
  MultiQueueDeviceBuilder &graphics(uint32_t count,
                                    bool presentQueueSupport = true,
                                    bool reserveImgui = true);
  MultiQueueDeviceBuilder &computes(uint32_t count);
  MultiQueueDeviceBuilder &transfers(uint32_t count);
  vkb::Device build();

protected:
  struct PQNode {
    std::list<QueueBlock>::iterator block;
    int score;
  };

  std::function<bool(const PQNode &, const PQNode &)> cmp =
      [](const PQNode &a, const PQNode &b) {
        return a.score > b.score; // min-heap
      };

  using PQ = std::priority_queue<PQNode, std::vector<PQNode>, decltype(cmp)>;

protected:
  std::list<QueueBlock> collectQueueFamilies();
  PQ BuildPQ(VkQueueFlagBits requiredFlags);
  void consume(VkQueueFlagBits requiredFlags, QueueRequest &req);
  PQ &select_pq(VkQueueFlagBits requiredFlags);

  [[nodiscard]]
  std::vector<vkb::CustomQueueDescription> build_descriptions_from_blockPool();

  std::unordered_map<VkQueueFlagBits, QueueRequest> queuePlan;
  std::list<QueueBlock> blockPool_;

  PQ pq_graphics;
  PQ pq_compute;
  PQ pq_transfer;

private:
  VkDevice device_;
  vkb::DeviceBuilder &builder_;
  vkb::PhysicalDevice &pdev_;
};
} // namespace engine

#endif //_MULTI_QUEUE_DEVICE_BUILDER_HPP_
