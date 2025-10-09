#include <GlobalDef.hpp>

namespace engine {
AllocatedBuffer::AllocatedBuffer(VmaAllocator allocator)
    : allocator_(allocator), isinit(false) {}
AllocatedBuffer::~AllocatedBuffer() { destroy(); }

void AllocatedBuffer::create(size_t allocSize, VkBufferUsageFlags usage,
                             VmaMemoryUsage memoryUsage) {

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

void *AllocatedBuffer::map() {
  void *src;
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
} // namespace engine
