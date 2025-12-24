#pragma once
#ifndef _GLOBALDEF_HPP_
#define _GLOBALDEF_HPP_
#include <Descriptors.hpp>
#include <frame/FrameData.hpp>
#include <numeric>
#include <string>
#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS // no degresss
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {

struct EngineStats {
  float frametime{16.0f / 1000.0f};
  int triangle_count{};
  int drawcall_count{};
  float scene_update_time{};
  float mesh_draw_time{};
};

namespace v2 {
enum class ResourceState {
  CpuOnly,         // Only in CPU
  UploadScheduled, //  Upload Buffer
  GpuResident,     // avaible in GPU
  UnInstalled,     // Removed from GPU, but CPU data still exist
  Destroyed        // completely destroyed
};

struct ResourcesStateManager {
  ResourcesStateManager(std::string root = "");

  ResourceState state() const;
  bool isGpuResident() const;
  bool isCpuOnly() const;
  bool isDestroyed() const;

  virtual void recordUpload(VkCommandBuffer cmd) = 0; // CPU/Uninstall => GPU
  virtual void destroy() = 0;                         //* => Destroyed!

  virtual bool tryUninstall(uint64_t observedValue) = 0; // GPU =>Uninstall
  virtual void forceUninstall() = 0;

  void setUploadCompleteTimeline(uint64_t value = 0);

  void markTouched(uint64_t frameIndex);
  uint64_t framesSinceLastTouch(uint64_t frameIndex) const;

  bool isUploadComplete(
      uint64_t observed = std::numeric_limits<uint64_t>::max()) const;
  bool isNoLongerUsed(uint64_t observed) const;

protected:
  /*State Machine!*/
  void Cpu2Destroy();
  void Cpu2UploadScheduled();
  void UploadSched2Destroy();
  void UploadSched2GpuResident();
  void GpuResident2Uninstall();
  void GpuResident2Destroy();
  void Uninstall2UploadSched();
  void Uninstall2Destroy();
  void Any2Destroy();

private:
  std::string root_ = "";

  // For Cpu controlled frame
  uint64_t lastTouchedFrame_{};

  // waitingTimelineValue_ is a logical guard for state transitions.
  // It does NOT perform or guarantee GPU synchronization.
  // Actual correctness is enforced by Vulkan queue submission and fences.
  uint64_t waitingTimelineValue_{};

  // State Machine
  ResourceState state_ = ResourceState::CpuOnly;
};

} // namespace v2

struct AllocatedBuffer;

struct AllocatedImage {
  VkImage image;
  VkImageView imageView;
  VmaAllocation allocation;
  VkExtent3D imageExtent;
  VkFormat imageFormat;

  AllocatedImage(VkDevice device, VmaAllocator allocator);
  virtual ~AllocatedImage();

  void create_image(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage,
                    bool mipmapped = false,
                    const std::string &name = "AllocatedImage");

  void destroy();

private:
  bool isinit_ = false;
  VkDevice device_;
  VmaAllocator allocator_;
};
} // namespace engine

#endif //_GLOBALDEF_HPP_
