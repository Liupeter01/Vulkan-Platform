#pragma once
#ifndef _MESH_BUFFERS_HPP_
#define _MESH_BUFFERS_HPP_
#include <vector>
#include <GlobalDef.hpp>

namespace engine {
          struct Vertex {
                    glm::vec3 position;
                    float uv_x;
                    glm::vec3 normal;
                    float uv_y;
                    glm::vec4 color;
          };

          struct Mesh {
                    std::vector<Vertex> vertices;
                    std::vector<uint32_t> indices;
          };

          inline namespace mesh {
                    struct GPUGeoMeshBuffers {
                              GPUGeoMeshBuffers(VkDevice device,VmaAllocator allocator);
                              virtual ~GPUGeoMeshBuffers();

                              AllocatedBuffer indexBuffer;
                              AllocatedBuffer vertexBuffer;

                              std::vector<Vertex> vertex_;
                              std::vector<uint32_t> indicies_;
                              VkDeviceAddress vertexBufferAddress{ };

                              void createMesh(
                                        std::vector<Vertex>&& vertices,
                                        std::vector<uint32_t>&& indices);

                              void createMesh(const Mesh& mesh);
                              void submitMesh(VkCommandBuffer cmd);
                              void flushUpload(VkFence fence);

                    private:
                              bool isinit = false;
                              VkDevice device_;
                              VmaAllocator allocator_;

                              bool  pendingUpload_ = false;
                              AllocatedBuffer staging;
                    };

                    struct GPUGeoPushConstants {
                              glm::mat4 matrix;
                              VkDeviceAddress vertexBuffer;
                    };
          }
}

#endif //_MESH_BUFFERS_HPP_
