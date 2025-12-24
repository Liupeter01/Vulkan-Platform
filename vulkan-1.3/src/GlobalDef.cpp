#include <GlobalDef.hpp>
#include <Tools.hpp>
#include <config.h>
#include <spdlog/spdlog.h>

namespace engine {
namespace v2 {
ResourcesStateManager::ResourcesStateManager(std::string root)
    : state_(ResourceState::CpuOnly), root_(root) {}

ResourceState ResourcesStateManager::state() const { return state_; }

bool ResourcesStateManager::isGpuResident() const {
  return state_ == ResourceState::GpuResident;
}

bool ResourcesStateManager::isCpuOnly() const {
  return state_ == ResourceState::CpuOnly;
}

bool ResourcesStateManager::isDestroyed() const {
  return state_ == ResourceState::Destroyed;
}

void ResourcesStateManager::setUploadCompleteTimeline(uint64_t value) {
  waitingTimelineValue_ = value;
}

void ResourcesStateManager::markTouched(uint64_t frameIndex) {
#if ENABLE_VALIDATION_LAYERS
  if (frameIndex < lastTouchedFrame_) [[unlikely]] {
    throw std::runtime_error("Frame Counter Internal Error!");
  }
  spdlog::info("[{}::ResourcesStateManager]: CurrentFrameCounter = {}", root_,
               frameIndex);
#endif
  lastTouchedFrame_ = frameIndex;
}

bool ResourcesStateManager::isUploadComplete(uint64_t observed) const {
  if (!waitingTimelineValue_ || observed >= waitingTimelineValue_) [[likely]] {
    return true;
  }
  return false;
}

bool ResourcesStateManager::isNoLongerUsed(uint64_t observed) const {
  return framesSinceLastTouch(observed) != 0;
}

uint64_t
ResourcesStateManager::framesSinceLastTouch(uint64_t frameIndex) const {
#if ENABLE_VALIDATION_LAYERS
  if (frameIndex < lastTouchedFrame_) [[unlikely]] {
    throw std::runtime_error("Frame Counter Internal Error!");
  }
#endif

  auto diff = frameIndex - lastTouchedFrame_;

#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: FramesSinceLastTouch Diff = {}",
               root_, diff);
#endif
  return diff;
}

void ResourcesStateManager::Cpu2Destroy() {
  if (state_ != ResourceState::CpuOnly) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not CpuOnly, Please check your state!");
  }
#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::Destroyed));
#endif
  state_ = ResourceState::Destroyed;
}

void ResourcesStateManager::Cpu2UploadScheduled() {
  if (state_ != ResourceState::CpuOnly) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not CpuOnly, Please check your state!");
  }
#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::UploadScheduled));
#endif
  state_ = ResourceState::UploadScheduled;
}

void ResourcesStateManager::UploadSched2Destroy() {
  if (state_ != ResourceState::UploadScheduled) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not UploadScheduled, Please check your state!");
  }
#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::Destroyed));
#endif
  state_ = ResourceState::Destroyed;
}

void ResourcesStateManager::UploadSched2GpuResident() {
  if (state_ != ResourceState::UploadScheduled) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not UploadScheduled, Please check your state!");
  }
#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::GpuResident));
#endif
  state_ = ResourceState::GpuResident;
}

void ResourcesStateManager::GpuResident2Uninstall() {
  if (state_ != ResourceState::GpuResident) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not GpuResident, Please check your state!");
  }

#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::UnInstalled));
#endif
  state_ = ResourceState::UnInstalled;
}

void ResourcesStateManager::GpuResident2Destroy() {
  if (state_ != ResourceState::GpuResident) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not GpuResident, Please check your state!");
  }

#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::Destroyed));
#endif
  state_ = ResourceState::Destroyed;
}

void ResourcesStateManager::Uninstall2UploadSched() {
  if (state_ != ResourceState::UnInstalled) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not UnInstalled, Please check your state!");
  }
#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::UploadScheduled));
#endif
  state_ = ResourceState::UploadScheduled;
}

void ResourcesStateManager::Uninstall2Destroy() {
  if (state_ != ResourceState::UnInstalled) [[unlikely]] {
    throw std::runtime_error(
        "The Start State is not UnInstalled, Please check your state!");
  }

#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}",
               root_, static_cast<int>(state_),
               static_cast<int>(ResourceState::Destroyed));
#endif
  state_ = ResourceState::Destroyed;
}

void ResourcesStateManager::Any2Destroy() {
#if ENABLE_VALIDATION_LAYERS
  spdlog::info("[{}::ResourcesStateManager]: Switching State From Any => {}",
               root_, static_cast<int>(ResourceState::Destroyed));
#endif
  state_ = ResourceState::Destroyed;
}
} // namespace v2

AllocatedImage::AllocatedImage(VkDevice device, VmaAllocator allocator)
    : device_(device), allocator_(allocator), isinit_(false) {}

AllocatedImage::~AllocatedImage() { destroy(); }

void AllocatedImage::create_image(VkExtent3D extent, VkFormat format,
                                  VkImageUsageFlags usage, bool mipmapped,
                                  const std::string &name) {

  if (isinit_)
    return;
  imageFormat = format;
  imageExtent = extent;

  VkImageCreateInfo rimg_info =
      tools::image_create_info(imageFormat, usage, extent);

  if (mipmapped) {
    rimg_info.mipLevels = static_cast<uint32_t>(
        util::generate_mipmap_levels({extent.width, extent.height}));
  }

  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(allocator_, &rimg_info, &rimg_allocinfo, &image, &allocation,
                 nullptr);

#if ENABLE_VALIDATION_LAYERS
  vmaSetAllocationName(allocator_, allocation, name.c_str());
#endif // ENABLE_VALIDATION_LAYERS

  // if the format is a depth format, we will need to have it use the correct
  // aspect flag
  VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == VK_FORMAT_D32_SFLOAT) {
    aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info =
      tools::imageview_create_info(imageFormat, image, aspectFlag);
  rview_info.subresourceRange.levelCount = rimg_info.mipLevels;

  vkCreateImageView(device_, &rview_info, nullptr, &imageView);
  isinit_ = true;
}

void AllocatedImage::destroy() {
  if (isinit_) {
    vkDestroyImageView(device_, imageView, nullptr);
    vmaDestroyImage(allocator_, image, allocation);
    isinit_ = false;
  }
}
} // namespace engine
