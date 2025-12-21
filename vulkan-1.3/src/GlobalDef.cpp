#include <GlobalDef.hpp>
#include <Tools.hpp>
#include <config.h>
#include <spdlog/spdlog.h>

namespace engine {
          namespace v2 {
                    ResourcesStateManager::ResourcesStateManager(std::string root)
                              :state_(ResourceState::CpuOnly)
                              , root_(root)
                    {
                    }

                    ResourceState ResourcesStateManager::state() const {
                              return state_;
                    }

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
                              spdlog::info("[{}::ResourcesStateManager]: CurrentFrameCounter = {}", root_, frameIndex);
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

                    uint64_t ResourcesStateManager::framesSinceLastTouch(uint64_t frameIndex) const {
#if ENABLE_VALIDATION_LAYERS
                              if (frameIndex < lastTouchedFrame_) [[unlikely]] {
                                        throw std::runtime_error("Frame Counter Internal Error!");
                              }
#endif

                              auto diff = frameIndex - lastTouchedFrame_;

#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: FramesSinceLastTouch Diff = {}", root_, diff);
#endif
                              return diff;
                    }

                    void ResourcesStateManager::Cpu2Destroy() {
                              if (state_ != ResourceState::CpuOnly) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not CpuOnly, Please check your state!");
                              }
#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::Destroyed));
#endif
                              state_ = ResourceState::Destroyed;
                    }

                    void ResourcesStateManager::Cpu2UploadScheduled() {
                              if (state_ != ResourceState::CpuOnly) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not CpuOnly, Please check your state!");
                              }
#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::UploadScheduled));
#endif
                              state_ = ResourceState::UploadScheduled;
                    }

                    void ResourcesStateManager::UploadSched2Destroy() {
                              if (state_ != ResourceState::UploadScheduled) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not UploadScheduled, Please check your state!");
                              }
#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::Destroyed));
#endif
                              state_ = ResourceState::Destroyed;
                    }

                    void ResourcesStateManager::UploadSched2GpuResident() {
                              if (state_ != ResourceState::UploadScheduled) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not UploadScheduled, Please check your state!");
                              }
#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::GpuResident));
#endif
                              state_ = ResourceState::GpuResident;
                    }

                    void ResourcesStateManager::GpuResident2Uninstall() {
                              if (state_ != ResourceState::GpuResident) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not GpuResident, Please check your state!");
                              }

#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::UnInstalled));
#endif
                              state_ = ResourceState::UnInstalled;
                    }

                    void ResourcesStateManager::GpuResident2Destroy() {
                              if (state_ != ResourceState::GpuResident) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not GpuResident, Please check your state!");
                              }

#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::Destroyed));
#endif
                              state_ = ResourceState::Destroyed;
                    }

                    void ResourcesStateManager::Uninstall2UploadSched() {
                              if (state_ != ResourceState::UnInstalled) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not UnInstalled, Please check your state!");
                              }
#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::UploadScheduled));
#endif
                              state_ = ResourceState::UploadScheduled;
                    }

                    void ResourcesStateManager::Uninstall2Destroy() {
                              if (state_ != ResourceState::UnInstalled) [[unlikely]] {
                                        throw std::runtime_error("The Start State is not UnInstalled, Please check your state!");
                              }

#if ENABLE_VALIDATION_LAYERS
                              spdlog::info("[{}::ResourcesStateManager]: Switching State From {} => {}", root_,
                                        static_cast<int>(state_), static_cast<int>(ResourceState::Destroyed));
#endif
                              state_ = ResourceState::Destroyed;
                    }
          }

                    AllocatedImage::AllocatedImage(VkDevice device, VmaAllocator allocator)
                              : device_(device), allocator_(allocator), isinit_(false) {
                    }

                    AllocatedImage::~AllocatedImage() { destroy(); }

                    void AllocatedImage::create_image(VkExtent3D extent, VkFormat format,
                              VkImageUsageFlags usage, bool mipmapped,
                              const std::string& name) {

                              if (isinit_)
                                        return;
                              imageFormat = format;
                              imageExtent = extent;

                              VkImageCreateInfo rimg_info =
                                        tools::image_create_info(imageFormat, usage, extent);

                              if (mipmapped) {
                                        rimg_info.mipLevels = static_cast<uint32_t>(
                                                  util::generate_mipmap_levels({ extent.width, extent.height }));
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

                    AllocatedBuffer::AllocatedBuffer(VmaAllocator allocator)
                              : allocator_(allocator), isinit(false) {
                    }
                    AllocatedBuffer::~AllocatedBuffer() { destroy(); }

                    void AllocatedBuffer::create(size_t allocSize, VkBufferUsageFlags usage,
                              VmaMemoryUsage memoryUsage,
                              const std::string& name) {

                              if (isinit)
                                        return;

                              VkBufferCreateInfo bufferInfo{};
                              bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                              bufferInfo.size = allocSize;
                              bufferInfo.usage = usage;

                              VmaAllocationCreateInfo vmaallocInfo = {};
                              vmaallocInfo.usage = memoryUsage;
                              vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                              // allocate the buffer
                              vmaCreateBuffer(allocator_, &bufferInfo, &vmaallocInfo, &buffer, &allocation,
                                        &info);

#if ENABLE_VALIDATION_LAYERS
                              vmaSetAllocationName(allocator_, allocation, name.c_str());
#endif // ENABLE_VALIDATION_LAYERS

                              isinit = true;
                    }

                    void AllocatedBuffer::clear() {
                              if (info.pMappedData && info.size > 0) {
                                        std::memset(info.pMappedData, 0, info.size);
                              }
                    }
                    void AllocatedBuffer::reset(size_t newSize, VkBufferUsageFlags usage,
                              VmaMemoryUsage memoryUsage) {

                              destroy();
                              create(newSize, usage, memoryUsage);
                    }

                    void* AllocatedBuffer::map() {
                              void* src;
                              vmaMapMemory(allocator_, allocation, &src);
                              return src;
                    }
                    void AllocatedBuffer::unmap() { vmaUnmapMemory(allocator_, allocation); }

                    void AllocatedBuffer::destroy() {
                              if (isinit) {
                                        vmaDestroyBuffer(allocator_, buffer, allocation);
                                        isinit = false;
                              }
                    }

                    AllocatedTexture::AllocatedTexture(VkDevice device, VmaAllocator allocator)
                              : allocator_(allocator), device_{ device }, dstImage_{ device, allocator },
                              srcBuffer_{ allocator } {
                    }

                    AllocatedTexture::~AllocatedTexture() { destroy(); }

                    void AllocatedTexture::createBuffer(void* data, VkExtent3D size,
                              VkFormat format, VkImageUsageFlags usage,
                              bool mipmapped) {

                              if (isinit)
                                        return;
                              extent_ = size;
                              const size_t data_flat_size =
                                        size.depth * size.height * size.width * tools::bytes_per_pixel(format);

                              srcBuffer_.create(data_flat_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU, "AllocatedTexture::SrcBuffer");

                              void* mapped = srcBuffer_.map();
                              memcpy(mapped, data, data_flat_size);
                              srcBuffer_.unmap();

                              dstImage_.create_image(size, format,
                                        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                        mipmapped, "AllocatedTexture::DstImage");

                              mipmapped_ = mipmapped;

                              isinit = true;
                    }

                    VkImage& AllocatedTexture::getImage() const { return dstImage_.image; }

                    VkImageView& AllocatedTexture::getImageView() const {
                              return dstImage_.imageView;
                    }

                    void AllocatedTexture::uploadBufferToImage(VkCommandBuffer cmd) {

                              util::transition_image(cmd, dstImage_.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                              VkBufferImageCopy copyRegion{};
                              copyRegion.bufferOffset = 0;
                              copyRegion.bufferRowLength = 0;
                              copyRegion.bufferImageHeight = 0;
                              copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                              copyRegion.imageSubresource.mipLevel = 0; // LOD0
                              copyRegion.imageSubresource.baseArrayLayer = 0;
                              copyRegion.imageSubresource.layerCount = 1;
                              copyRegion.imageExtent = extent_;

                              vkCmdCopyBufferToImage(cmd, srcBuffer_.buffer, dstImage_.image,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                              if (mipmapped_) {
                                        util::generate_mipmaps(
                                                  cmd, dstImage_.image,
                                                  { dstImage_.imageExtent.width, dstImage_.imageExtent.height });
                              }
                              else {
                                        util::transition_image(cmd, dstImage_.image,
                                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                              }

                              pendingUpload_ = true;
                    }

                    void AllocatedTexture::invalid() {
                              destroy();
                              pendingUpload_ = false;
                    }

                    bool AllocatedTexture::isValid() const { return isinit; }

                    void AllocatedTexture::flushUpload(VkFence fence) {
                              if (!pendingUpload_)
                                        return;

                              vkWaitForFences(device_, 1, &fence, true,
                                        std::numeric_limits<uint64_t>::max());

                              // srcBuffer_.destroy();
                              pendingUpload_ = false;
                    }

                    void AllocatedTexture::destroy() {
                              if (isinit) {
                                        srcBuffer_.destroy();
                                        dstImage_.destroy();
                                        isinit = false;
                              }
                    }
} // namespace engine
