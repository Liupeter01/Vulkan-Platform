#include <Tools.hpp>
#include <AllocatedTexture.hpp>

namespace engine {
          namespace v2 {
                    AllocatedTexture2::AllocatedTexture2(VkDevice device, VmaAllocator allocator, const std::string& name)
                              :ResourcesStateManager(name)
                              , name_(name)
                              , device_(device)
                              , staging_(allocator)
                    {}

                    AllocatedTexture2::~AllocatedTexture2() {
                              destroy();
                    }

                    VkImage AllocatedTexture2::image() {
                              return image_;
                    }

                    VkImageView AllocatedTexture2::imageView() {
                              return imageView_;
                    }

                    void AllocatedTexture2::__createGpuImage() {

                              if (gpuAllocated_)
                                        return;
                              VkImageCreateInfo rimg_info =
                                        tools::image_create_info(imageFormat_, imageUsage_, imageExtent_);

                              if (mipMapped_) {
                                        rimg_info.mipLevels = static_cast<uint32_t>(
                                                  util::generate_mipmap_levels({ imageExtent_.width,    imageExtent_.height }));
                              }

                              VmaAllocationCreateInfo rimg_allocinfo = {};
                              rimg_allocinfo.usage = memoryUsage_;
                              if (memoryUsage_ == VMA_MEMORY_USAGE_GPU_ONLY) {
                                        rimg_allocinfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                              }

                              // allocate and create the image
                              vmaCreateImage(allocator_, &rimg_info, &rimg_allocinfo, &image_, &allocation_,
                                        nullptr);

#if ENABLE_VALIDATION_LAYERS
                              vmaSetAllocationName(allocator_, allocation, name.c_str());
#endif // ENABLE_VALIDATION_LAYERS

                              // if the format is a depth format, we will need to have it use the correct
                              // aspect flag
                              aspectFlag_ = VK_IMAGE_ASPECT_COLOR_BIT;
                              if (imageFormat_ == VK_FORMAT_D32_SFLOAT) {
                                        aspectFlag_ = VK_IMAGE_ASPECT_DEPTH_BIT;
                              }

                              // build a image-view for the draw image to use for rendering
                              VkImageViewCreateInfo rview_info =
                                        tools::imageview_create_info(imageFormat_, image_, aspectFlag_);
                              rview_info.subresourceRange.levelCount = rimg_info.mipLevels;

                              vkCreateImageView(device_, &rview_info, nullptr, &imageView_);

                              gpuAllocated_ = true;
                    }

                    void AllocatedTexture2::__destroyGpuImage() {
                              if (gpuAllocated_) {
                                        vkDestroyImageView(device_, imageView_, nullptr);
                                        vmaDestroyImage(allocator_, image_, allocation_);
                                        allocation_ = {};
                                        gpuAllocated_ = false;
                              }
                    }

                    void AllocatedTexture2::perpareTransferData(const void* data, const std::size_t length) {
                              if (!configured_)
                                        throw std::runtime_error("Please configure() first");

                              if (length != textureSize_)
                                        throw std::runtime_error("size mismatch");

                              updateCpuBuffer(data, length);
                    }

                    void AllocatedTexture2::updateCpuBuffer(const void* data, const std::size_t length) {
                              if (!configured_)
                                        throw std::runtime_error("You Must Configure parameters first!");
                              if (length != textureSize_)
                                        throw std::runtime_error("size mismatch");
                              memcpy(textureBuffer_.data(), data, length);
                    }

                    // Prepare Cpu staging(Transfer src) data
                    // Note:  recordUpload function WILL NOT ACCEPT DATA MODIFICATION
                    void AllocatedTexture2::updateCpuStaging() {

                              // Making sure staging buffer is destroyed!
                              staging_.destroy();

                              cpuStaging_ = false;

                              staging_.create(textureBuffer_.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU,
                                        name_ + std::string("::staging").c_str());

                              void* src = staging_.map();
                              std::memcpy(reinterpret_cast<unsigned char*>(src), textureBuffer_.data(), textureBuffer_.size());
                              staging_.unmap();

                              cpuStaging_ = true;
                    }

                    void AllocatedTexture2::purgeReleaseStaging(uint64_t observedValue) {
                              if (!pendingUpload_ || !cpuStaging_) return;
                              if (state() == ResourceState::UploadScheduled &&
                                        this->isUploadComplete(observedValue)) [[likely]] {

                                        UploadSched2GpuResident();

                                        staging_.destroy(); 
                                        cpuStaging_ = false;
                                        pendingUpload_ = false;
                                        return;
                              }

#if ENABLE_VALIDATION_LAYERS
                              spdlog::warn("[{}]: Purge Release Staging Failed!", name_);
#endif
                    }

                    void AllocatedTexture2::recordUpload(VkCommandBuffer cmd) {
                              auto st = state();
                              if (!(st == ResourceState::UploadScheduled || st == ResourceState::CpuOnly ||
                                        st == ResourceState::UnInstalled))
                                        return;

                              updateCpuStaging();

                              if (!cpuStaging_)
                                        throw std::runtime_error("CPU staging missing");

                              if (!gpuAllocated_) {
                                        __createGpuImage(); //
                              }

                              // Switch the state
                              if (st == ResourceState::CpuOnly)
                                        Cpu2UploadScheduled();
                              else if (st == ResourceState::UnInstalled)
                                        Uninstall2UploadSched();

                              util::transition_image(cmd, image(), VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                              VkBufferImageCopy copyRegion{};
                              copyRegion.bufferOffset = 0;
                              copyRegion.bufferRowLength = 0;
                              copyRegion.bufferImageHeight = 0;
                              copyRegion.imageSubresource.aspectMask =  aspectFlag_;
                              copyRegion.imageSubresource.mipLevel = 0; // LOD0
                              copyRegion.imageSubresource.baseArrayLayer = 0;
                              copyRegion.imageSubresource.layerCount = 1;
                              copyRegion.imageExtent = imageExtent_;

                              // NOTE: staging_ uses v1 buffer wrapper; direct .buffer access is intentional.
                              vkCmdCopyBufferToImage(cmd, staging_.buffer, image(),
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                              if (mipMapped_) {
                                        util::generate_mipmaps(
                                                  cmd, image(),
                                                  { imageExtent_.width, imageExtent_.height });
                              }
                              else {
                                        util::transition_image(cmd, image(),
                                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                              }

                              pendingUpload_ = true;
                    }

                    bool AllocatedTexture2::tryUninstall(uint64_t observedValue) {
                              if (state() != ResourceState::GpuResident)
                                        return false;

                              if (!isNoLongerUsed(observedValue))
                                        return false;

                              __destroyGpuImage();
                              GpuResident2Uninstall();
                              return true;
                    }

                    void AllocatedTexture2::forceUninstall() {
                              if (state() == ResourceState::GpuResident) {
                                        __destroyGpuImage();
                                        GpuResident2Uninstall();
                                        return;
                              }
                    }

                    void AllocatedTexture2::destroy() {
                              if (isDestroyed())
                                        return;
                              textureBuffer_.clear();
                              staging_.destroy();
                              __destroyGpuImage();
                              Any2Destroy();
                    }

                    // configure Image Size and their usage!
                    void AllocatedTexture2::configure(VkExtent3D extent,
                              VkFormat format,
                              VkImageUsageFlags imageUsage,
                              bool mipmapped,
                              VmaMemoryUsage memoryUsage) {

                              imageFormat_ = format;
                              imageExtent_ = extent;

                              imageUsage_ = imageUsage;
                              mipMapped_ = mipmapped;
                              memoryUsage_ = memoryUsage;

                              textureSize_ =
                                        imageExtent_.depth * imageExtent_.height * imageExtent_.width * tools::bytes_per_pixel(imageFormat_);

                              textureBuffer_.resize(textureSize_);

                              configured_ = true;
                    }
          }
}