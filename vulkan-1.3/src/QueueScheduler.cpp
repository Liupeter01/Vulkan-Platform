#include <QueueScheduler.hpp>
#include <spdlog/spdlog.h>

namespace engine {

QueueDispatcher::QueueDispatcher(const VkQueueFlagBits queueType,
                                 std::vector<Pack> &&normalPool)
    : normalPools_(std::move(normalPool)), queueType_(queueType),
      normalIndex(0), normalCounter(normalPool.size()) {
  if (normalCounter < 1)
    throw std::runtime_error("Invalid Queue Size!");
}

std::size_t QueueDispatcher::dispatch() {
  if (normalCounter <= 1)
    return 0;

  normalIndex = (normalIndex++) % normalCounter;
  return normalIndex;
}

Pack QueueDispatcher::get_queue() { return normalPools_[dispatch()]; }

VkQueueFlagBits QueueDispatcher::getQueueType() const { return queueType_; }

QueueScheduler::QueueScheduler(VkDevice device,
                               const QueueRequestMappingStruct &request)
    : device_(device), queuePlan_(request) {
  // load all queues in this device!
  extractQueuesByCategory(VK_QUEUE_GRAPHICS_BIT);
  extractQueuesByCategory(VK_QUEUE_TRANSFER_BIT);
  extractQueuesByCategory(VK_QUEUE_COMPUTE_BIT);

  finalizeByCategory(VK_QUEUE_GRAPHICS_BIT);
}

void QueueScheduler::finalizeByCategory(VkQueueFlagBits flag) {

  // only graphics may have imgui/present
  if (!(flag & VK_QUEUE_GRAPHICS_BIT))
    return;

  auto &requestPlan = queuePlan_[flag];
  auto &dispatcher = queueDispatcher_[flag];

  if (!dispatcher) {
    throw std::runtime_error("nullptr");
  }

  if (dispatcher->normalPools_.empty()) {
    throw std::runtime_error(
        "[QueueScheduler] No queues available for graphics family!");
  }

  uint32_t available = dispatcher->normalPools_.size();

  // special requirement amount
  uint32_t req_present = requestPlan.presentQueueStatus ? 1 : 0;
  uint32_t req_imgui = requestPlan.reserveImgui ? 1 : 0;

  reservePresent = req_present;
  reserveImgui_ = req_imgui;

  uint32_t special = req_present + req_imgui;

  // minimal guarantee: special + 1 normal
  uint32_t minimum_required = special + 1u;

  if (available < minimum_required) {
    auto msg = fmt::format("[QueueScheduler] Insufficient graphics queues: "
                           "available={}, required(min)={}. "
                           "(present={} imgui={} normal-required=1)",
                           available, minimum_required, req_present, req_imgui);
    spdlog::error(msg);
    throw std::runtime_error(msg);
  }

  // ==== allocate PRESENT queue first (must guarantee swapchain safety) ====
  if (req_present) {
    presentQueue_ = dispatcher->normalPools_.front();
    dispatcher->normalPools_.erase(dispatcher->normalPools_.begin());
    available--;
    spdlog::info("[QueueScheduler] Present queue allocated.");
  }

  // ==== allocate IMGUI queue next (optional feature) ====
  if (req_imgui) {
    imguiQueue_ = dispatcher->normalPools_.front();
    dispatcher->normalPools_.erase(dispatcher->normalPools_.begin());
    available--;
    spdlog::info("[QueueScheduler] ImGui queue allocated.");
  }

  // ==== remaining queues for normal round-robin ====
  if (available <= 1) {
    spdlog::warn("[QueueScheduler] Only {} graphics queue left. "
                 "Pipeline overlap / frame pipelining will be limited.",
                 available);
  }

  dispatcher->normalIndex = 0;
  dispatcher->normalCounter = dispatcher->normalPools_.size();
}

void QueueScheduler::extractQueuesByCategory(VkQueueFlagBits flag) {

  if (!device_) {
    throw std::runtime_error("Device cannot be null");
  }

  std::vector<Pack> normalPools;
  auto &request = queuePlan_[flag];

  if (request.allocatedQueueInfo.empty()) {
    spdlog::warn("[QueueScheduler] No allocation found for flag {}",
                 uint32_t(flag));
    return;
  }

  for (const auto &[family, range] : request.allocatedQueueInfo) {
    for (uint32_t startIndex = range.startPos;
         startIndex < range.startPos + range.count; ++startIndex) {
      VkQueue queue;
      vkGetDeviceQueue(device_, family, startIndex, &queue);
      normalPools.push_back(Pack{family, queue});
    }
  }

  auto [_, status] = queueDispatcher_.try_emplace(
      flag, std::make_unique<QueueDispatcher>(flag, std::move(normalPools)));
  if (!status) {
    spdlog::error("[QueueScheduler Error]: Init {} Queue's Dispatcher Failed!",
                  static_cast<uint32_t>(flag));
  }
}

Pack QueueScheduler::get_queue(VkQueueFlagBits flag) {
  auto it = queueDispatcher_.find(flag);
  if (it == queueDispatcher_.end()) {
    throw std::runtime_error("Invalid Queue Flag!");
  }
  return it->second->get_queue();
}

Pack QueueScheduler::imgui_queue() {
  if (!reserveImgui_ || !imguiQueue_.queue) {
    throw std::runtime_error("IMGUI Queue Not Enabled!");
  }
  return imguiQueue_;
}

Pack QueueScheduler::present_queue() {
  if (!reservePresent || !presentQueue_.queue) {
    throw std::runtime_error("Present Queue Not Enabled!");
  }
  return presentQueue_;
}
} // namespace engine
