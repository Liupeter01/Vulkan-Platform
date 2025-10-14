#include <mesh/MeshBuffers.hpp>

namespace engine {

namespace mesh {
GPUGeoMeshBuffers::GPUGeoMeshBuffers(VkDevice device, VmaAllocator allocator)
    : device_{device}, allocator_(allocator), indexBuffer{allocator},
      vertexBuffer{allocator}, staging{allocator}, isinit(false) {}

GPUGeoMeshBuffers::~GPUGeoMeshBuffers() {
  if (isinit) {
    indexBuffer.destroy();
    vertexBuffer.destroy();
    staging.destroy();
    isinit = false;
  }
}

void GPUGeoMeshBuffers::createMesh(std::vector<Vertex> &&vertices,
                                   std::vector<uint32_t> &&indices) {

  vertex_ = std::move(vertices);
  indicies_ = std::move(indices);

  const std::size_t vertexBufferSize = vertex_.size() * sizeof(vertex_[0]);
  const std::size_t indiciesBufferSize =
      indicies_.size() * sizeof(indicies_[0]);

  vertexBuffer.create(vertexBufferSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |

                          // Shader Device Addr
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,

                      VMA_MEMORY_USAGE_GPU_ONLY, 
                    "GPUGeoMeshBuffers::VertexBuffer"
            );

  indexBuffer.create(indiciesBufferSize,
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,

                     VMA_MEMORY_USAGE_GPU_ONLY,
            "GPUGeoMeshBuffers::IndiciesBuffer");

  // find the adress of the vertex buffer
  VkBufferDeviceAddressInfo deviceAdressInfo{};
  deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAdressInfo.buffer = vertexBuffer.buffer;
  vertexBufferAddress = vkGetBufferDeviceAddress(device_, &deviceAdressInfo);
}

void GPUGeoMeshBuffers::createMesh(const Mesh &mesh) {

  const std::size_t vertexBufferSize =
      mesh.vertices.size() * sizeof(mesh.vertices[0]);
  const std::size_t indiciesBufferSize =
      mesh.indices.size() * sizeof(mesh.indices[0]);

  vertex_ = mesh.vertices;
  indicies_ = mesh.indices;

  vertexBuffer.create(vertexBufferSize,
                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |

                          // Shader Device Addr
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,

                      VMA_MEMORY_USAGE_GPU_ONLY);

  indexBuffer.create(indiciesBufferSize,
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,

                     VMA_MEMORY_USAGE_GPU_ONLY);

  // find the adress of the vertex buffer
  VkBufferDeviceAddressInfo deviceAdressInfo{};
  deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  deviceAdressInfo.buffer = vertexBuffer.buffer;
  vertexBufferAddress = vkGetBufferDeviceAddress(device_, &deviceAdressInfo);
}

void GPUGeoMeshBuffers::submitMesh(VkCommandBuffer cmd) {
  staging.destroy();

  const std::size_t vertexBufferSize = vertex_.size() * sizeof(vertex_[0]);
  const std::size_t indiciesBufferSize =
      indicies_.size() * sizeof(indicies_[0]);

  staging.create(indiciesBufferSize + vertexBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void *src = staging.map();
  memcpy(reinterpret_cast<unsigned char *>(src), vertex_.data(),
         vertexBufferSize);
  memcpy(reinterpret_cast<unsigned char *>(src) + vertexBufferSize,
         indicies_.data(), indiciesBufferSize);
  staging.unmap();

  VkBufferCopy vertexCopy{};
  vertexCopy.size = vertexBufferSize;

  VkBufferCopy indexCopy{};
  indexCopy.srcOffset = vertexBufferSize;
  indexCopy.size = indiciesBufferSize;

  vkCmdCopyBuffer(cmd, staging.buffer, vertexBuffer.buffer, 1, &vertexCopy);
  vkCmdCopyBuffer(cmd, staging.buffer, indexBuffer.buffer, 1, &indexCopy);

  pendingUpload_ = true;
}

void GPUGeoMeshBuffers::flushUpload(VkFence fence) {
  if (!pendingUpload_)
    return;

  vkWaitForFences(device_, 1, &fence, true,
                  std::numeric_limits<uint64_t>::max());

  staging.destroy();
  pendingUpload_ = false;
}
} // namespace mesh

} // namespace engine
