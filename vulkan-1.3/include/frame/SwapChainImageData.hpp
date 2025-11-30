#pragma once
#ifndef _IMAGE_DATA_HPP_
#define _IMAGE_DATA_HPP_
#include <vulkan/vulkan.hpp>

namespace engine {

          class VulkanEngine;
          struct SwapChainImageData {
                    SwapChainImageData(VulkanEngine* eng);
                    virtual ~SwapChainImageData();
                   void init(VkSemaphoreCreateInfo semaphoreCreateInfo,
                              VkImage image,
                              VkImageView imageView);

                   void destroy();

                   VkImage swapchainImage;         //
                   VkImageView swapchainImageView; //

                    VkSemaphore present; // Bin Semaphore, present queue wait for graphic

          private:
                    bool isinit_ = false;
                    VulkanEngine* engine_{};
          };
}

#endif //_IMAGE_DATA_HPP_