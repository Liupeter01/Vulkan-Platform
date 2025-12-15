#pragma once
#ifndef _ASYNC_SUBMIT_HANDLER_HPP_
#define _ASYNC_SUBMIT_HANDLER_HPP_
#include <atomic>
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <task/LogicWorker.hpp>

namespace engine {
          class QueueScheduler;
          class AsyncSubmitHandler {
          public:
                    AsyncSubmitHandler(VkQueueFlagBits type, std::size_t queueCount);
                    AsyncSubmitHandler(VkQueueFlagBits whichTypeIWant, const QueueScheduler& s);
                    virtual ~AsyncSubmitHandler();

          public:
                    void start();
                    void terminate();

                    template<typename Func, typename ...Args>
                    auto commit(Func&& func, Args&&... args)
                              -> std::future< std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>> {

                              return workers_[dispatch()]->commit(
                                        std::forward<Func>(func), 
                                        std::forward<Args>(args)...);
                    }

          private:
                    virtual std::size_t dispatch();

          private:
                    bool isinit_ = false;
                    VkQueueFlagBits type_;
                    std::atomic<std::size_t> currentIndex{0};
                    std::size_t queueCounter_{};
                    std::atomic<std::size_t> availableThreadNumber_{ 0 };
                    std::vector<std::shared_ptr<LogicWorker>> workers_;
          };
}

#endif //_ASYNC_SUBMIT_HANDLER_HPP_