#include <QueueScheduler.hpp>
#include <spdlog/spdlog.h>
#include <task/AsyncSubmitHandler.hpp>

namespace engine {

AsyncSubmitHandler::AsyncSubmitHandler(VkQueueFlagBits type,
                                       std::size_t queueCount)
    : currentIndex{0}, queueCounter_(queueCount), type_(type) {}

AsyncSubmitHandler::AsyncSubmitHandler(VkQueueFlagBits whichTypeIWant,
                                       const QueueScheduler &s)
    : currentIndex{0} {
  auto it = s.queueDispatcher_.find(whichTypeIWant);
  if (it == s.queueDispatcher_.end()) {
    throw std::runtime_error("Invalid Queue Type!");
  }

  type_ = whichTypeIWant;
  queueCounter_ = it->second->get_queue_size();
}

AsyncSubmitHandler::~AsyncSubmitHandler() { terminate(); }

void AsyncSubmitHandler::start() {
  if (isinit_)
    return;

  if (queueCounter_ <= 0) {
    spdlog::warn("[AsyncSubmitHandler Warn]: Illegal Queue Count Detected! "
                 "Automatically set to one");
    queueCounter_ = ((queueCounter_ <= 0) ? 1 : queueCounter_);
  }

  workers_.reserve(queueCounter_);
  for (std::size_t index = 0; index < queueCounter_; ++index) {
    auto worker = std::make_shared<LogicWorker>(type_);
    workers_.push_back(worker);
    worker->start();
  }

  isinit_ = true;
}

void AsyncSubmitHandler::terminate() {
  if (isinit_) {
    for (std::size_t index = 0; index < queueCounter_; ++index) {
      workers_[index]->terminate();
    }
    isinit_ = false;
  }
}

std::size_t AsyncSubmitHandler::dispatch() {
  if (queueCounter_ <= 1) {
    return 0;
  }

  uint64_t expected = currentIndex.load(std::memory_order_acquire);
  while (true) {

    uint64_t temp = expected;
    uint64_t desired = (temp + 1) % queueCounter_; //

    if (currentIndex.compare_exchange_weak(
            expected, // compare with "currentIndex"
            desired,  // write next index
            std::memory_order_release, std::memory_order_acquire)) {
      return desired; // success!
    }
  }

  auto str =
      fmt::format("[AsyncSubmitHandler Error]: QueueInfo Submission Error!"
                  " The Code should not be there!");
  spdlog::error(str);
  throw std::runtime_error(str);
}
} // namespace engine
