#include <VulkanEngine.hpp>
#include <frame/SwapChainImageData.hpp>

namespace engine {
          SwapChainImageData::SwapChainImageData(VulkanEngine* eng)
                    :engine_(eng)
          {
          }

          SwapChainImageData::~SwapChainImageData() {
                    destroy();
          }

          void  SwapChainImageData::init(VkSemaphoreCreateInfo semaphoreCreateInfo,
                    VkImage image,
                    VkImageView imageView) {

                    if (isinit_)return;

                    swapchainImage = image;
                    swapchainImageView = imageView;

                    //vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr, &acquire);
                    vkCreateSemaphore(engine_->device_, &semaphoreCreateInfo, nullptr, &present);

                    isinit_ = true;
          }

          void  SwapChainImageData::destroy() {
                    if (isinit_) {
                              //vkDestroySemaphore(engine_->device_, acquire, nullptr);
                              vkDestroySemaphore(engine_->device_, present, nullptr);
                              vkDestroyImageView(engine_->device_, swapchainImageView, nullptr);
                              isinit_ = false;
                    }
          }
}