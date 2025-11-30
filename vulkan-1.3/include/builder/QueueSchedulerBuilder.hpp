#pragma once
#ifndef _QUEUE_SCHEDULER_BUILDER_HPP_
#define _QUEUE_SCHEDULER_BUILDER_HPP_
#include <QueueScheduler.hpp>

namespace engine {

          struct QueueSchedulerBuilder {
                    QueueSchedulerBuilder(const MultiQueueDeviceBuilder& builder)
                              :queuePlan_(builder.queuePlan), device_(builder.device_)
                    { }

                    std::unique_ptr<QueueScheduler> build() {
                              assert(device_ && "Device cannot be null");
                              return std::make_unique<QueueScheduler>(device_, queuePlan_);
                    }

          private:
                    VkDevice device_ = VK_NULL_HANDLE;
                    std::unordered_map<VkQueueFlagBits, QueueRequest > queuePlan_;
          };
}

#endif //_QUEUE_SCHEDULER_BUILDER_HPP_