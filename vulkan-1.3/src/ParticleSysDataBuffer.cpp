#include <particle/ParticleDataBuffer.hpp>
#include <particle/PointSpriteParticleSystemBase.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

namespace engine {

template <>
ParticleSysDataBuffer<::engine::particle::GPUParticle>::ParticleSysDataBuffer(
    VkDevice device, VmaAllocator allocator)
    : device_(device), allocator_(allocator) {
  mt.seed(static_cast<unsigned>(std::random_device{}()));
  rndDist = std::uniform_real_distribution<float>(0.f, 1.f);
}

template <>
ParticleSysDataBuffer<
    ::engine::particle::GPUParticle>::~ParticleSysDataBuffer() {
  destroy();
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::destroy() {
  __destroyGpuBuffer();
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::configure(
    const std::size_t particle_count, ParticleDimension dim,
    VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) {

  if (gpuAllocated_) {
    throw std::runtime_error(
        "configure() after GPU allocated; call destroy() first");
  }

  dim_ = dim;

  particleCount = particle_count;
  particleBufferSize = particleCount * particleSize;

  bufferUsage_ = bufferUsage;
  memoryUsage_ = memoryUsage;

  configure_ = true;
}

template <>
std::size_t
ParticleSysDataBuffer<::engine::particle::GPUParticle>::getParticleCount()
    const noexcept {
  // if (!configure_)
  //           throw std::runtime_error("Please configure() first");
  return particleCount;
}

template <>
std::size_t
ParticleSysDataBuffer<::engine::particle::GPUParticle>::getBufferSize()
    const noexcept {
  // if (!configure_)
  //           throw std::runtime_error("Please configure() first");
  return particleBufferSize;
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::swap() noexcept {
  index ^= 1; // index = (index + 1) % 2
}

template <>
VkBuffer ParticleSysDataBuffer<::engine::particle::GPUParticle>::get_in_buffer()
    const noexcept {
  // if (!buffers_[index]) {
  //           throw std::runtime_error("ParticleSysDataBuffer: Invalid
  //           Buffer");
  // }
  return buffers_[index]->buffer();
}

template <>
VkBuffer
ParticleSysDataBuffer<::engine::particle::GPUParticle>::get_out_buffer()
    const noexcept {
  // if (!buffers_[index ^ 1]) {
  //           throw std::runtime_error("ParticleSysDataBuffer: Invalid
  //           Buffer");
  // }
  return buffers_[index ^ 1]->buffer();
}

template <>
void ParticleSysDataBuffer<
    ::engine::particle::GPUParticle>::__createGpuBuffer() {
  if (gpuAllocated_)
    return;

  for (auto &buf : buffers_) {
    buf.reset();
    buf = std::make_shared<::engine::v2::AllocatedBuffer2>(
        device_, allocator_, "ParticleSysDataBuffer::PingPongBuffer");

    buf->configure(particleBufferSize, bufferUsage_, memoryUsage_);
  }

  gpuAllocated_ = true;
}

template <>
void ParticleSysDataBuffer<
    ::engine::particle::GPUParticle>::__destroyGpuBuffer() {
  if (gpuAllocated_) {
    for (auto &buf : buffers_) {
      buf->destroy();
      buf.reset();
    }
    gpuAllocated_ = false;
  }
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::singleTimeUpload(
    VkCommandBuffer cmd) {

  std::call_once(upload_once_, [&] {
    if (!cpuReady_)
      throw std::runtime_error("CPU staging missing");
    for (auto &buf : buffers_)
      buf->recordUpload(cmd);
    pendingUpload_ = true;
  });
}

template <>
void ParticleSysDataBuffer<
    ::engine::particle::GPUParticle>::perpareTransferData() {

  if (!configure_)
    throw std::runtime_error("Please configure() first");

  if (!gpuAllocated_) {
    __createGpuBuffer();
  }

  std::vector<uint8_t> data =
      ((dim_ == ParticleDimension::Particle2D) ? fill2D() : fill3D());

  for (auto &buf : buffers_) {
    buf->perpareTransferData(data.data(), data.size());
  }
  cpuReady_ = true;
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::
    updateUploadingStatus(uint64_t observedValue) {
  if (pendingUpload_) {
    for (auto &buf : buffers_) {
      buf->updateUploadingStatus(observedValue);
    }
    pendingUpload_ = false;
  }
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::
    purgeReleaseStaging(uint64_t observedValue) {

  if (cpuReady_) {
    for (auto &buf : buffers_) {
      buf->purgeReleaseStaging(observedValue);
    }
    cpuReady_ = false;
  }
}

template <>
void ParticleSysDataBuffer<::engine::particle::GPUParticle>::
    setUploadCompleteTimeline(uint64_t value) {
  for (auto &buf : buffers_) {
    buf->setUploadCompleteTimeline(value);
  }
}

template <>
std::vector<uint8_t>
ParticleSysDataBuffer<::engine::particle::GPUParticle>::fill2D() {
  std::vector<uint8_t> ret;
  ret.resize(particleBufferSize);
  ParticleType *src = reinterpret_cast<ParticleType *>(ret.data());

  for (std::size_t i = 0; i < particleCount; ++i) {
    auto &particle = src[i];

    float r = std::sqrt(rndDist(mt));
    float theta = 2.0f * glm::pi<float>() * rndDist(mt);

    float x = r * cos(theta);
    float y = r * sin(theta);

    particle.position = glm::vec4(x, y, 0, 1);
    glm::vec3 pos = glm::vec3(particle.position);
    glm::vec3 vel = glm::normalize(glm::cross(pos, glm::vec3(0, 0, 1)));

    particle.velocity = glm::vec4(vel * 0.002f, 0.0f);
    particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
  }
  return ret;
}

template <>
std::vector<uint8_t>
ParticleSysDataBuffer<::engine::particle::GPUParticle>::fill3D() {
  std::vector<uint8_t> ret;
  ret.resize(particleBufferSize);
  ParticleType *src = reinterpret_cast<ParticleType *>(ret.data());
  for (std::size_t i = 0; i < particleCount; ++i) {
    auto &particle = src[i];

    float u = rndDist(mt);
    float v = rndDist(mt);

    float theta = 2.0f * glm::pi<float>() * u; // 0~2pi
    float phi = acos(2.0f * v - 1);            //  0~¦Ð

    float sx = sin(phi) * cos(theta);
    float sy = sin(phi) * sin(theta);
    float sz = cos(phi);

    float r = cbrt(rndDist(mt));

    glm::vec3 direction = glm::vec3(sx, sy, sz);
    particle.position = glm::vec4(direction * r, 1.0f);
    particle.velocity = glm::vec4(direction * 0.0005f, 0.f);
    particle.color = glm::vec4(rndDist(mt), rndDist(mt), rndDist(mt), 1.0f);
  }
  return ret;
}
} // namespace engine
