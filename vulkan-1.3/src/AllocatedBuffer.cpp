#include <config.h>
#include <spdlog/spdlog.h>
#include <AllocatedBuffer.hpp>

namespace engine {
          namespace v2 {

                    AllocatedBuffer2::AllocatedBuffer2(VmaAllocator allocator, const std::string& name)
                              :allocator_(allocator)
                              , name_(name)
                              , staging_(allocator)
                              , isinit_(false)
                              , configured_(false)
                    {}

                    AllocatedBuffer2::~AllocatedBuffer2() {
                              destroy();
                    }

                    void AllocatedBuffer2::configure(const size_t allocSize,
                              VkBufferUsageFlags bufferUsage,
                              VmaMemoryUsage memoryUsage) {

                              if (allocSize <= 0) [[unlikely]] {
                                        throw std::runtime_error("Buffer size cannot be 0");
                              }

                              allocSize_ = allocSize;
                              bufferUsage_ = bufferUsage;
                              memoryUsage_ = memoryUsage;

                              configured_ = true;
                    }

                    void AllocatedBuffer2::updateCpuStaging(const void* data, const std::size_t length) {

                              if (!configured_) throw std::runtime_error("Please configure() first");
                              if (length != allocSize_) throw std::runtime_error("size mismatch");

                              //Making sure staging buffer is destroyed!
                              staging_.destroy();
                              staging_.create(length,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_ONLY,
                                        name_ + std::string("::staging").c_str());

                              void* src = staging_.map();
                              std::memcpy(reinterpret_cast<unsigned char*>(src), data, length);
                              staging_.unmap();

                              cpuReady_ = true;
                    }

                    VkBuffer AllocatedBuffer2::buffer() {
                              return buffer_;
                    }

                    void AllocatedBuffer2::__createGpuBuffer() {
                              if (gpuAllocated_) return;
                              VkBufferCreateInfo bufferInfo{};
                              bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                              bufferInfo.size = allocSize_;
                              bufferInfo.usage = bufferUsage_;

                              VmaAllocationCreateInfo vmaallocInfo = {};
                              vmaallocInfo.usage = memoryUsage_;
                              //vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                              // allocate the buffer
                              vmaCreateBuffer(allocator_, &bufferInfo, &vmaallocInfo, &buffer_, &allocation_,
                                        &allocInfo_);

#if ENABLE_VALIDATION_LAYERS
                              vmaSetAllocationName(allocator_, allocation_, name_.c_str());
#endif // ENABLE_VALIDATION_LAYERS

                              gpuAllocated_ = true;
                    }

                    void AllocatedBuffer2::__destroyGpuBuffer(){
                              if (gpuAllocated_) {
                                        vmaDestroyBuffer(allocator_, buffer_, allocation_);
                                        buffer_ = VK_NULL_HANDLE;
                                        allocation_ = {};
                                        allocInfo_ = {};
                                        gpuAllocated_ = false;
                              }
                    }

                    void AllocatedBuffer2::recordUpload(VkCommandBuffer cmd) {
                              auto st = state();
                              if (!(st == ResourceState::UploadScheduled ||
                                        st == ResourceState::CpuOnly ||
                                        st == ResourceState::UnInstalled)) return;

                              if (!cpuReady_) throw std::runtime_error("CPU staging missing");

                              if (!gpuAllocated_) {
                                        __createGpuBuffer(); // 
                              }

                              VkBufferCopy copy{};
                              copy.size = allocSize_;

                              vkCmdCopyBuffer(cmd, staging_.buffer, buffer_, 1, &copy);

                              //Switch the state
                              if (st == ResourceState::CpuOnly) Cpu2UploadScheduled();
                              else if (st == ResourceState::UnInstalled) Uninstall2UploadSched();

                              pendingUpload_ = true;
                    }

                    void AllocatedBuffer2::purgeReleaseStaging(uint64_t observedValue) {
                              if (state() == ResourceState::UploadScheduled &&
                                        this->isUploadComplete(observedValue)) [[likely]]{

                                        UploadSched2GpuResident();

                                        staging_.destroy();
                                        cpuReady_ = false;
                                        pendingUpload_ = false;
                                        return;
                              }

#if ENABLE_VALIDATION_LAYERS
                              spdlog::warn("[{}]: Purge Release Staging Failed!", name_);
#endif
                    }

                    bool AllocatedBuffer2::tryUninstall(uint64_t observedValue) {
                              if (state() != ResourceState::GpuResident)
                                        return false;

                              if (!isNoLongerUsed(observedValue))
                                        return false;

                              __destroyGpuBuffer();
                              GpuResident2Uninstall();
                              return true;
                    }

                    void AllocatedBuffer2::forceUninstall() {
                              if (state() == ResourceState::GpuResident) {
                                        __destroyGpuBuffer();
                                        GpuResident2Uninstall();
                                        return;
                              }
                    }

                    void AllocatedBuffer2::destroy() {
                              if (isDestroyed()) return;
                              staging_.destroy();
                              __destroyGpuBuffer();
                              this->state_ = ResourceState::Destroyed;
                    }
          }
}