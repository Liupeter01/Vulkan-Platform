#include <spdlog/spdlog.h>
#include <builder/MultiQueueDeviceBuilder.hpp>

namespace engine {

          MultiQueueDeviceBuilder::QueueBlock::QueueBlock(std::size_t index, VkQueueFamilyProperties prot)
                    : familyIndex(index)
                    , count(prot.queueCount)
                    , original(prot.queueCount)
                    , flags(prot.queueFlags)
                    , startPos(0)
          { }

          int MultiQueueDeviceBuilder::score(const VkQueueFlags f) {
                    int s = 0;
                    if (f & VK_QUEUE_TRANSFER_BIT) s += 1;
                    if (f & VK_QUEUE_COMPUTE_BIT)  s += 2;
                    if (f & VK_QUEUE_GRAPHICS_BIT) s += 4;
                    return s;
          }

          MultiQueueDeviceBuilder::MultiQueueDeviceBuilder(vkb::DeviceBuilder& builder, 
                                                                                                    vkb::PhysicalDevice& pdev)
                    :builder_{ builder }
                    , pdev_(pdev)
                    , pq_graphics(cmp)
                    , pq_compute(cmp)
                    , pq_transfer(cmp){

                    //print out queue family parameters, and generate a global queue block list!
                    blockPool_ = collectQueueFamilies();

                    pq_graphics = BuildPQ(VK_QUEUE_GRAPHICS_BIT);
                    pq_compute = BuildPQ( VK_QUEUE_COMPUTE_BIT);
                    pq_transfer = BuildPQ(VK_QUEUE_TRANSFER_BIT);

                    queuePlan[VK_QUEUE_GRAPHICS_BIT] = QueueRequest{ VK_QUEUE_GRAPHICS_BIT };
                    queuePlan[VK_QUEUE_COMPUTE_BIT] = QueueRequest{ VK_QUEUE_COMPUTE_BIT };
                    queuePlan[VK_QUEUE_TRANSFER_BIT] = QueueRequest{ VK_QUEUE_TRANSFER_BIT };
          }

          MultiQueueDeviceBuilder& MultiQueueDeviceBuilder::graphics(uint32_t count, bool presentQueueSupport, bool reserveImgui) {
                    auto& q = queuePlan[VK_QUEUE_GRAPHICS_BIT];
                    q.requested = count;
                    q.reserveImgui = reserveImgui;
                    q.presentQueueStatus = presentQueueSupport;
                    return *this;
          }

          MultiQueueDeviceBuilder& MultiQueueDeviceBuilder::computes(uint32_t count) {
                    queuePlan[VK_QUEUE_COMPUTE_BIT].requested = count;
                    return *this;
          }

          MultiQueueDeviceBuilder& MultiQueueDeviceBuilder::transfers(uint32_t count) {
                    queuePlan[VK_QUEUE_TRANSFER_BIT].requested = count;
                    return *this;
          }

          vkb::Device MultiQueueDeviceBuilder::build() {

                    consume(VK_QUEUE_GRAPHICS_BIT, queuePlan[VK_QUEUE_GRAPHICS_BIT]);
                    consume(VK_QUEUE_COMPUTE_BIT, queuePlan[VK_QUEUE_COMPUTE_BIT]);
                    consume(VK_QUEUE_TRANSFER_BIT, queuePlan[VK_QUEUE_TRANSFER_BIT]);

                    std::vector<vkb::CustomQueueDescription> descs = build_descriptions_from_blockPool();

                    auto vkb = builder_.custom_queue_setup(descs).build().value();
                    device_ = vkb.device;

                    assert(device_ && "Device cannot be null");

                    return vkb;
          }

          std::list<MultiQueueDeviceBuilder::QueueBlock> MultiQueueDeviceBuilder::collectQueueFamilies() {
                    const auto& families = pdev_.get_queue_families();
                    std::list<MultiQueueDeviceBuilder::QueueBlock> res;

                    spdlog::info("=========== Vulkan Queue Family Report ===========");
                    for (uint32_t i = 0; i < families.size(); ++i) {
                              const auto& f = families[i];
                              res.emplace_back(QueueBlock{ i, f });
                              std::string flags;
                              if (f.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                                        flags += " GRAPHICS";
                              if (f.queueFlags & VK_QUEUE_COMPUTE_BIT)
                                        flags += " COMPUTE";
                              if (f.queueFlags & VK_QUEUE_TRANSFER_BIT)
                                        flags += " TRANSFER";
                              if (f.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                                        flags += " SPARSE";
                              if (f.queueFlags & VK_QUEUE_PROTECTED_BIT)
                                        flags += " PROTECTED";
                              if (flags.empty())
                                        flags = " NONE";

                              spdlog::info("QueueFamilyIndex: {}\n\t\t\t\t"
                                        "queueCount: {}\n\t\t\t\t"
                                        "QueueFlags: {}\n\t\t\t\t"
                                        "---------------------------------------------------", i, f.queueCount, flags);
                    }
                    spdlog::info("===================================================");
                    return res;
          }

          typename MultiQueueDeviceBuilder::PQ MultiQueueDeviceBuilder::BuildPQ(VkQueueFlagBits requiredFlags) {

                    PQ pq(cmp);
                    for (auto &it = blockPool_.begin(); it != blockPool_.end(); ++it) {
                              if (it->count == 0) continue;
                              if ((it->flags & requiredFlags) != requiredFlags) continue;

                              PQNode node{ it, score(it->flags) };
                              if (node.score == 0) continue;
                              pq.push(node);
                    }
                    return pq;
          }

          typename MultiQueueDeviceBuilder::PQ& MultiQueueDeviceBuilder::select_pq(VkQueueFlagBits requiredFlags) {
                    if (requiredFlags & VK_QUEUE_GRAPHICS_BIT) {
                              return pq_graphics;
                    }
                    if (requiredFlags & VK_QUEUE_COMPUTE_BIT) {
                              return pq_compute;
                    }
                    if (requiredFlags & VK_QUEUE_TRANSFER_BIT) {
                              return pq_transfer;
                    }
                    throw std::runtime_error("Invalid Queue Type!");
          }

          void MultiQueueDeviceBuilder::consume(VkQueueFlagBits requiredFlags, QueueRequest& req) {

                    auto& pq = select_pq(requiredFlags);

                    uint32_t requested = req.requested;// +(req.reserveImgui ? 1u : 0u) + (req.presentQueueStatus ? 1u : 0u);
                    req.queueType = requiredFlags;

                    while (requested > 0){
                              if (pq.empty())
                                        break;

                              // Copy node (priority_queue::top is const)
                              PQNode node = pq.top();
                              pq.pop();

                              QueueBlock& block = *node.block; // list iterator ¡ú actual block

                              if (!block.count)
                                        continue;

                              uint32_t alloc = std::min(requested, block.count);

                              // Record allocation
                              req.allocatedQueueInfo.emplace_back(
                                        block.familyIndex,
                                        QueueRequest::Span{
                                            block.startPos,
                                            alloc
                                        }
                              );

                              req.allocated += alloc;

                              // Deduct from block
                              block.startPos += alloc;
                              block.count -= alloc;
                              requested -= alloc;

                              // If this block still has leftover queues, re-insert to PQ
                              if (block.count > 0) {
                                        pq.push(PQNode{ node.block, node.score });
                              }
                    }

                    if (!req.allocated) {
                              throw std::runtime_error("Nothing Allocated for this queue!");
                    }
          }

          std::vector<vkb::CustomQueueDescription>
          MultiQueueDeviceBuilder::build_descriptions_from_blockPool() {

                    std::unordered_map<uint32_t, uint32_t> familyToCount;

                    for (const auto& block : blockPool_) {
                              uint32_t used = block.original - block.count;
                              if (!used) continue;
                              auto& cur = familyToCount[block.familyIndex];
                              if (used > cur)
                                        cur = used;
                    }

                    std::vector<vkb::CustomQueueDescription> descs;
                    for (const auto& [familyIndex, count] : familyToCount) {
                              std::vector<float> pr(count, 1.0f);
                              descs.emplace_back(familyIndex, std::move(pr));
                    }
                    return descs;
          }
}