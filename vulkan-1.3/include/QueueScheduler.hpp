#pragma once
#ifndef _QUEUE_SCHEDULER_HPP_
#define _QUEUE_SCHEDULER_HPP_
#include <builder/MultiQueueDeviceBuilder.hpp>
#include <vulkan/vulkan.hpp>

namespace engine {
using QueueRequest = typename MultiQueueDeviceBuilder::QueueRequest;
class QueueScheduler;

struct Pack {
  Pack() = default;
  Pack(uint32_t familyIndex, VkQueue q) : family(familyIndex), queue(q) {}
  uint32_t family{};
  VkQueue queue = VK_NULL_HANDLE;
};

struct QueueDispatcher {
  friend class QueueScheduler;

  QueueDispatcher(const VkQueueFlagBits queueType,
                  std::vector<Pack> &&normalPool);
  std::size_t get_queue_size() const;
  Pack get_queue();
  virtual std::size_t dispatch();
  VkQueueFlagBits getQueueType() const;

protected:
  VkQueueFlagBits queueType_{};

  std::size_t normalIndex{};
  std::size_t normalCounter{};

  std::vector<Pack> normalPools_;
};

class AsyncSubmitHandler;
class QueueScheduler {
  friend class AsyncSubmitHandler;
  using QueueRequestMappingStruct =
      std::unordered_map<VkQueueFlagBits, QueueRequest>;
  using QueueDispatcherMappingStruct =
      std::unordered_map<VkQueueFlagBits, std::unique_ptr<QueueDispatcher>>;

public:
  QueueScheduler(VkDevice device, const QueueRequestMappingStruct &request);

  Pack get_queue(VkQueueFlagBits flag);
  Pack imgui_queue();
  Pack present_queue();

protected:
  void finalizeByCategory(VkQueueFlagBits flag);
  void extractQueuesByCategory(VkQueueFlagBits flag);

protected:
  bool reserveImgui_ = false;
  bool reservePresent = false;
  // special queue support(Graphic Only)
  Pack imguiQueue_{};
  Pack presentQueue_{};

private:
  QueueRequestMappingStruct queuePlan_;
  QueueDispatcherMappingStruct queueDispatcher_;
  VkDevice device_ = VK_NULL_HANDLE;
};
} // namespace engine

#endif //_QUEUE_SCHEDULER_HPP_
