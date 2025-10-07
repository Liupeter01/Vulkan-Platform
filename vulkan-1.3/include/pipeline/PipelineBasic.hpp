#pragma once
#ifndef _PIPELINE_BASIC_HPP_
#define _PIPELINE_BASIC_HPP_
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {
          class PipelineBasic {
          public:
                    enum class PipelineType {
                              UNDEFINED,
                              COMPUTE,
                              GRAPHIC
                    };

                    PipelineBasic(VkDevice device, PipelineType type)
                              :device_(device), type_(type)
                    {
                    }

                    virtual ~PipelineBasic() {
                              destroy();
                    }

                    PipelineBasic(const PipelineBasic&) = delete;
                    PipelineBasic& operator=(const PipelineBasic&) = delete;

                    virtual void draw(VkCommandBuffer cmd, VkExtent2D drawExtent, VkImageView imageView = VK_NULL_HANDLE) = 0;
                    bool isCompute() const { return type_ == PipelineType::COMPUTE; }
                    bool isGraphics() const { return type_ == PipelineType::GRAPHIC; }
                    virtual void init() = 0;
                    virtual void destroy() {
                              if (isInit_) {
                                        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
                                        vkDestroyPipeline(device_, pipeline_, nullptr);
                                        reset_init();
                              }
                    }

          protected:
                    void reset_init() { isInit_ = false; }
                    void init_finished() { isInit_ = true; }

          protected:
                    bool isInit_ = false;
                    PipelineType type_ = PipelineType::UNDEFINED;
                    VkDevice device_ = VK_NULL_HANDLE;
                    VkPipeline pipeline_ = VK_NULL_HANDLE;
                    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
          };

}

#endif //_PIPELINE_BASIC_HPP_