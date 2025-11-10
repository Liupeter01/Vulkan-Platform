#pragma once
#ifndef _COMPUTE_PARTICLESYS_HPP_
#define _COMPUTE_PARTICLESYS_HPP_
#include <array>
#include <random>
#include <compute/Compute_EffectBase.hpp>

namespace engine {

          struct ParticlePushConstant {
                    float     deltaTime{};   //
                    uint32_t  frameIndex{};  // 
                    uint32_t  _pad0{};       // 
                    uint32_t  _pad1{};
          };

          struct GPUParticle {
                    glm::vec4 position;
                    glm::vec4 velocity;
                    glm::vec4 color;
          };

          struct ParticleSysDataBuffer {
                    ParticleSysDataBuffer(VulkanEngine* eng);
                    virtual ~ParticleSysDataBuffer();
                    void destroy();
                    void invalid();
                    bool isValid() const;
                    void create(const std::size_t particle_count);
                    void swap() noexcept;
                    void submit(VkCommandBuffer cmd);
                    void flush(VkFence fence);
                    VkBuffer get_in_buffer() const noexcept;
                    VkBuffer get_out_buffer() const noexcept;

                    /*one is for particle in, another is for particle out;
                    * buffer[0] will be the first one and it's going to swap in every frame
                    */
                    bool index = 0;
                    std::size_t particleCount{ 0 };
                    std::array<std::shared_ptr<AllocatedBuffer>, 2> buffers;

          protected:
                    std::random_device rd;
                    std::mt19937 mt;
                    std::uniform_real_distribution<float> rndDist;

                    //This is the staging buffer to init the first buffer slot
                    AllocatedBuffer staging;

                    void fill(void* buffer, const std::size_t particle_count);

          private:
                    bool isinit_ = false;
                    VulkanEngine* engine_;
                    bool pendingUpload_ = false;
          };

          struct ParticleResources {
                    VkImageView colorImage{};

                    //Ping Pong Swapping structure
                    VkBuffer particlesIn;
                    VkBuffer particlesOut;
                    std::size_t bufferSize;
          };

          inline namespace compute {

                    template<typename PushConstantType = ParticlePushConstant,
                              typename ResourcesType = ParticleResources >
                    class Compute_ParticleSys        
                              : public Compute_EffectBase< PushConstantType, ResourcesType> {

                    public:

                              Compute_ParticleSys(VkDevice device)
                                        : Compute_EffectBase< PushConstantType, ResourcesType>(device)
                              {}

                              void init() override {
                                        if (this->isinit_) return;

                                        DescriptorLayoutBuilder builder{ this->device_ };
                                        this->computeLayout_ = builder
                                                  //.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                                                  .add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                                  .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                                  .build(VK_SHADER_STAGE_COMPUTE_BIT); // add bindings

                                        auto layout = { this->computeLayout_ };

                                        this->defaultComputePipeline_.template create<PushConstantType>(ComputePass::DEFAULT, layout);
                                        this->specialConstantPipeline_.template create<PushConstantType>(ComputePass::SPECIALCONSTANT, layout);
                                        this->isinit_ = true;
                              }

                              void dispatch(VkCommandBuffer cmd, const ComputeInstance& ins) override {
                              }

                              ComputeInstance  generate_instance(
                                        ResourcesType& resources,
                                        DescriptorPoolAllocator& globalDescriptorAllocator) override {

                                        this->writer_.clear();

                                        ComputeInstance ins{};
                                        ins.pipeline = &this->defaultComputePipeline_;
                                        ins.computeSet = globalDescriptorAllocator.allocate(this->computeLayout_);

                                        this->writer_.write_buffer(
                                                  0, resources.particlesIn, resources.bufferSize,
                                                  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

                                       this->writer_.write_buffer(
                                                  1, resources.particlesOut, resources.bufferSize,
                                                  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

                                       this->writer_.update_set(ins.computeSet);
                                        return ins;
                              }
                    };
          } // namespace compute
} // namespace engine

#endif //_COMPUTE_PARTICLESYS_HPP_
