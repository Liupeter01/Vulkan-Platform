#pragma once
#ifndef _LOGIC_WORKER_HPP_
#define _LOGIC_WORKER_HPP_
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <functional>
#include <type_traits>
#include <condition_variable>
#include <vulkan/vulkan.hpp>

namespace engine {

          class LogicWorker {
                    LogicWorker(const LogicWorker&) = delete;
                    LogicWorker operator=(const LogicWorker&) = delete;

          public:
                    using Task = std::function<void()>;
                    LogicWorker(VkQueueFlagBits type);
                    virtual ~LogicWorker();

          public:
                    void start();
                    void terminate();
                    bool state() const;

                    template<typename Func, typename ...Args>
                    auto commit(Func&& func, Args&&... args)
                    ->std::future< std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>{
                              
                              if (!isinit_) {
                                        throw std::runtime_error("You have to call start() first!");
                              }

                              using RetType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;
                              using FutureType = std::future<RetType>;
                              using WrapFuncType = std::packaged_task<RetType()>;

                              if (stop_.load(std::memory_order_acquire)) {
                                        return {};
                              }

                              auto wrapped = std::make_shared<WrapFuncType>(std::bind(
                                        std::forward<Func>(func), 
                                        std::forward<Args>(args)...));

                              FutureType future = wrapped->get_future();

                              {
                                        std::lock_guard<std::mutex> _lckg(mtx_);
                                        queue_.emplace([wrapped]() {wrapped->operator()(); });
                              }
                              cv_.notify_one();
                              return future;
                    }

          protected:
                    void run();

          protected:
                    VkQueueFlagBits type_;

          private:
                    bool isinit_ = false;
                    std::atomic<bool> stop_ = false;
                    std::atomic<bool> isBusy_ = false;

                    std::thread th_;
                    std::mutex mtx_;
                    std::condition_variable cv_;
                    std::queue<Task> queue_;
          };
}

#endif //_LOGIC_WORKER_HPP_