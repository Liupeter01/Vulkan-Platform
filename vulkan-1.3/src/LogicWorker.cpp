#include <task/LogicWorker.hpp>

namespace engine {

LogicWorker::LogicWorker(VkQueueFlagBits type) : type_(type) {}

LogicWorker::~LogicWorker() { terminate(); }

bool LogicWorker::state() const {
  return isBusy_.load(std::memory_order_acquire);
}

void LogicWorker::start() {
  if (isinit_)
    return;
  th_ = std::move(std::thread(&LogicWorker::run, this));
  isinit_ = true;
}

void LogicWorker::terminate() {
  if (isinit_) {
    stop_.store(true);
    cv_.notify_all();
    if (th_.joinable()) {
      th_.join();
    }

    isinit_ = false;
  }
}

void LogicWorker::run() {
  while (true) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(mtx_);

      cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });

      if (stop_ && queue_.empty())
        break;

      task = std::move(queue_.front());
      queue_.pop();
    }

    isBusy_ = true;
    task.operator()();
    isBusy_ = false;
  }

  std::unique_lock<std::mutex> lock(mtx_);
  while (!queue_.empty()) {
    auto task = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    task.operator()();
    lock.lock();
  }
}
} // namespace engine
