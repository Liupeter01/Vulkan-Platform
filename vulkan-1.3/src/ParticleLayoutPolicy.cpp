#include <particle/ParticleLayoutPolicy.hpp>

namespace engine {
namespace particle {
namespace policy {

namespace details {

BufferArray2::BufferArray2(VkDevice dev, VmaAllocator alloc, std::string name)
    : device_(dev), allocator_(alloc), name_(name) {}

BufferArray2::~BufferArray2() { destroy(); }

void BufferArray2::create() {

  if (!gpuAllocated_) {
    __createGpuBuffer();
  }
}

void BufferArray2::clear() {
  if (gpuAllocated_) {
    __destroyGpuBuffer();

    gpuAllocated_ = false;
  }
}

void BufferArray2::recreate(const std::size_t allocSize,
                            VkBufferUsageFlags bufferUsage,
                            VmaMemoryUsage memoryUsage) {

  configure(allocSize, bufferUsage, memoryUsage);

  create();
}

void BufferArray2::updateUploadingStatus(uint64_t observedValue) {
  if (pendingUpload_) {
    bufs[0]->updateUploadingStatus(observedValue);
    pendingUpload_ = false;
  }
}

void BufferArray2::recordUpload(VkCommandBuffer cmd) {

  if (!cpuReady_)
    throw std::runtime_error("[BufferArray2]: CPU staging missing");

  bufs[0]->recordUpload(cmd);
  pendingUpload_ = true;
}

void BufferArray2::setUploadCompleteTimeline(uint64_t value) {
  bufs[0]->setUploadCompleteTimeline(value);
}

void BufferArray2::purgeReleaseStaging(uint64_t observedValue) {
  if (cpuReady_) {
    bufs[0]->purgeReleaseStaging(observedValue);
    cpuReady_ = false;
  }
}

void BufferArray2::destroy() noexcept { __destroyGpuBuffer(); }

VkBuffer BufferArray2::in(bool idx) const noexcept {
  return bufs[idx]->buffer();
}

VkBuffer BufferArray2::out(bool idx) const noexcept {
  return bufs[idx ^ 1]->buffer();
}

void BufferArray2::configure(const std::size_t allocSize,
                             VkBufferUsageFlags bufferUsage,
                             VmaMemoryUsage memoryUsage) {

  if (gpuAllocated_) {
    throw std::runtime_error(
        "configure() after GPU allocated; call destroy() first");
  }

  if (allocSize <= 0) {
    throw std::runtime_error("Alloc Size Must Be Higher than Zero!");
  }

  allocSize_ = allocSize;

  bufferUsage_ = bufferUsage;
  memoryUsage_ = memoryUsage;

  configure_ = true;
}

void BufferArray2::__createGpuBuffer() {
  if (gpuAllocated_)
    return;

  if (!configure_)
    throw std::runtime_error("Please configure() first");

  for (auto &buf : bufs) {
    buf.reset();
    buf = std::make_shared<::engine::v2::AllocatedBuffer2>(
        device_, allocator_, "BufferArray2::PingPongBuffer");

    buf->configure(allocSize_, bufferUsage_, memoryUsage_);
    buf->forceCreate();
  }

  gpuAllocated_ = true;
}

void BufferArray2::__destroyGpuBuffer() {
  if (gpuAllocated_) {
    for (auto &buf : bufs) {
      if (buf) {
        buf->destroy();
        buf.reset();
      }
    }
    gpuAllocated_ = false;
  }
}
} // namespace details
} // namespace policy
} // namespace particle
} // namespace engine
