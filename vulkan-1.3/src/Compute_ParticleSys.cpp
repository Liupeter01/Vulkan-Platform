#include <VulkanEngine.hpp>
#include <compute/Compute_ParticleSys.hpp>
#include <spdlog/fmt/fmt.h>

namespace engine {

ParticleSysDataBuffer::ParticleSysDataBuffer(VulkanEngine *eng)
    : engine_(eng), staging(eng->allocator_), rndDist(0.f, 1.f), mt(rd()) {}

ParticleSysDataBuffer::~ParticleSysDataBuffer() { destroy(); }

void ParticleSysDataBuffer::invalid() {
  destroy();
  pendingUpload_ = false;
}

bool ParticleSysDataBuffer::isValid() const { return isinit_; }

void ParticleSysDataBuffer::destroy() {
  if (isinit_) {
    staging.destroy();
    for (auto &buf : buffers) {
      buf->destroy();
      buf.reset();
    }
    isinit_ = false;
  }
}

void ParticleSysDataBuffer::create(const std::size_t particle_count) {
  std::size_t bufferSize = sizeof(GPUParticle) * particle_count;

  if (isinit_)
    return;

  for (auto &buf : buffers) {
    buf.reset();
    buf = std::make_shared<AllocatedBuffer>(engine_->allocator_);
    buf->create(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,

        VMA_MEMORY_USAGE_GPU_ONLY, "ParticleSysDataBuffer::PingPongBuffer");
  }

  isinit_ = true;
}

void ParticleSysDataBuffer::submit(VkCommandBuffer cmd) {

  staging.destroy();

  std::size_t bufferSize = sizeof(GPUParticle) * particleCount;

  /*create staging buffer for upload*/
  staging.create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 "ParticleSysDataBuffer::StagingBuffer");

  // fill random data
  fill(staging.map(), particleCount);
  staging.unmap();

  VkBufferCopy bufferCopy{0, 0, bufferSize};
  vkCmdCopyBuffer(cmd, staging.buffer, buffers[0]->buffer, 1, &bufferCopy);

  pendingUpload_ = true;
}

void ParticleSysDataBuffer::flush(VkFence fence) {
  if (!pendingUpload_)
    return;

  vkWaitForFences(engine_->device_, 1, &fence, true,
                  std::numeric_limits<uint64_t>::max());

  // staging.destroy();
  pendingUpload_ = false;
}

void ParticleSysDataBuffer::fill(void *buffer,
                                 const std::size_t particle_count) {
  GPUParticle *src = reinterpret_cast<GPUParticle *>(buffer);
  for (std::size_t i = 0; i < particle_count; ++i) {
    auto &particle = src[i];

    const float r = std::sqrtf(rndDist(mt));
    const float theta = 2.f * rndDist(mt) * glm::pi<float>();
    const float x = r * cosf(theta) * engine_->drawExtent_.height /
                    engine_->drawExtent_.width;
    const float y = r * sinf(theta);
    particle.position = glm::vec4(x, y, 0, 0);

    glm::vec2 dir = glm::vec2(x, y);
    if (glm::length(dir) < 1e-6f) {
      float angle = 2.f * rndDist(mt) * glm::pi<float>();
      dir = glm::vec2(cosf(angle), sinf(angle));
    }

    particle.velocity = glm::vec4(normalize(dir) * 0.00025f, 0.f, 0.f);
    particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
  }
}

void ParticleSysDataBuffer::swap() noexcept {
  index ^= 1; // index = (index + 1) % 2
}

VkBuffer ParticleSysDataBuffer::get_in_buffer() const noexcept {
  return buffers[index]->buffer;
}

VkBuffer ParticleSysDataBuffer::get_out_buffer() const noexcept {
  return buffers[index ^ 1]->buffer;
}
} // namespace engine
