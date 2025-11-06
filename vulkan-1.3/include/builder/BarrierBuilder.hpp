#pragma once
#ifndef _BARRIER_BUILDER_HPP_
#define _BARRIER_BUILDER_HPP_
#include <array>
#include <vector>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace engine {

          namespace details {
                    template <typename T>
                    struct is_barrier2 : std::false_type {};

                    template <>
                    struct is_barrier2<VkImageMemoryBarrier2> : std::true_type {};

                    template <>
                    struct is_barrier2<VkBufferMemoryBarrier2> : std::true_type {};

                    template <>
                    struct is_barrier2<VkMemoryBarrier2> : std::true_type {};

                    template <typename T>
                    constexpr bool is_barrier2_v = is_barrier2<T>::value;
          }

          struct ImageBarrierBuilder {

                    struct ImageTransitionRule {
                              VkImageLayout oldLayout;
                              VkImageLayout newLayout;
                              VkPipelineStageFlags2 srcStage;
                              VkPipelineStageFlags2 dstStage;
                              VkAccessFlags2 srcAccess;
                              VkAccessFlags2 dstAccess;
                    };

                    ImageBarrierBuilder(VkImage image, VkFormat format = VK_FORMAT_B8G8R8A8_UNORM);
                    ImageBarrierBuilder& from(VkImageLayout oldLayout);
                    ImageBarrierBuilder& to(VkImageLayout newLayout);
                    ImageBarrierBuilder& aspect(VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
                    ImageBarrierBuilder& withRange(uint32_t baseMip = 0, 
                              uint32_t mipCount = VK_REMAINING_MIP_LEVELS,
                              uint32_t baseLayer = 0, 
                              uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

                    ImageBarrierBuilder& queueIndex(uint32_t srcQueueFamilyIndex= VK_QUEUE_FAMILY_IGNORED,
                              uint32_t dstQueueFamilyIndex= VK_QUEUE_FAMILY_IGNORED);

                    VkImageMemoryBarrier2 build();

                    static constexpr std::array<ImageTransitionRule, 16> rules_ = { {
                                        // UNDEFINED ¡ú GENERAL
                                        { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                          VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                          0, VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT },

                                          // GENERAL ¡ú COLOR_ATTACHMENT_OPTIMAL
                                          { VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                            VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT },

                                            // UNDEFINED ¡ú COLOR_ATTACHMENT_OPTIMAL
                                            { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                              VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                              0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT },

                                              // UNDEFINED ¡ú TRANSFER_DST_OPTIMAL
                                        { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 
                                          VK_PIPELINE_STAGE_2_TRANSFER_BIT,    
                                          0, VK_ACCESS_2_TRANSFER_WRITE_BIT },

                                          { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
  VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
  VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT },

  { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
  VK_ACCESS_2_TRANSFER_READ_BIT, VK_ACCESS_2_SHADER_READ_BIT },

                                          // TRANSFER_DST_OPTIMAL ¡ú COLOR_ATTACHMENT_OPTIMAL
{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
  VK_ACCESS_2_TRANSFER_WRITE_BIT,
  VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT },

                                                  { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                              VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                                              0,
                                              VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                              VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },

                              //  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                              { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                                0,
                                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },

                                              // UNDEFINED ¡ú DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                              { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                                                0, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },

                                                // COLOR_ATTACHMENT_OPTIMAL ¡ú TRANSFER_SRC_OPTIMAL
                                                { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT },

                                                  // TRANSFER_DST_OPTIMAL ¡ú SHADER_READ_ONLY_OPTIMAL
                                                  { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                    VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                                    VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT },

                                                    // GENERAL ¡ú TRANSFER_SRC_OPTIMAL
                                                    { VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                      VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                                                      VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT },

                                                      // TRANSFER_DST_OPTIMAL ¡ú GENERAL
                                                      { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                                                        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                                        VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT },

                                                        // COLOR_ATTACHMENT_OPTIMAL ¡ú PRESENT_SRC_KHR
                                                        { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE,
                                                          VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0 },

                                                          // TRANSFER_SRC_OPTIMAL ¡ú PRESENT_SRC_KHR
                                                          { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE,
                                                            VK_ACCESS_2_TRANSFER_READ_BIT, 0 }
                                                      } };

          private:
                    VkImage image_;
                    uint32_t srcQueueFamilyIndex_{ VK_QUEUE_FAMILY_IGNORED };
                    uint32_t dstQueueFamilyIndex_{ VK_QUEUE_FAMILY_IGNORED };
                    VkImageLayout oldLayout_{ VK_IMAGE_LAYOUT_UNDEFINED };
                    VkImageLayout newLayout_{ VK_IMAGE_LAYOUT_UNDEFINED };
                    VkImageAspectFlags aspectMask_{ VK_IMAGE_ASPECT_COLOR_BIT };
                    VkImageSubresourceRange range_{ 
                              aspectMask_ ,
                              0, 
                              VK_REMAINING_MIP_LEVELS, 
                              0, 
                              VK_REMAINING_ARRAY_LAYERS 
                    };
          };

          struct BarrierBuilder {
                    template <typename T>
                    std::enable_if_t<details::is_barrier2_v<std::decay_t<T>>, BarrierBuilder&> add(T&& barrier) {
                              addBarrier(std::forward<T>(barrier));
                              return *this;
                    }

                    template <typename... Ts>
                     std::enable_if_t<(details::is_barrier2_v<std::decay_t<Ts>> && ...), BarrierBuilder&> addMany(Ts&&... xs) {
                              (addBarrier(std::forward<Ts>(xs)), ...);
                               return *this;
                    }

                    [[nodiscard]] VkDependencyInfo build() ;
                    void createBarrier(VkCommandBuffer cmd);

          protected:
                    void clear();
                    void addBarrier(const VkMemoryBarrier2& b);
                    void addBarrier(const VkBufferMemoryBarrier2& b);
                    void addBarrier(const VkImageMemoryBarrier2& b);

          private:
                    std::vector<VkMemoryBarrier2> memBarriers_;
                    std::vector<VkBufferMemoryBarrier2> bufBarriers_;
                    std::vector<VkImageMemoryBarrier2> imgBarriers_;
          };
}

#endif //_IMAGE_BARRIER_BUILDER_HPP_