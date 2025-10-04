#include <Model.hpp>
#include <tiny_obj_loader.h>
#include <unordered_map> //filter faces

void engine::Builder::loadObjModel(const std::string &path) {

  vertices.clear();
  indices.clear();

  tinyobj::attrib_t attribute;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::unordered_map<Vertex, std::size_t> uniqueVertices{};

  std::string warn, err;
  if (!tinyobj::LoadObj(&attribute, &shapes, &materials, &warn, &err,
                        path.c_str())) {
    throw std::runtime_error(err + warn);
  }

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex;

      // process vertices
      if (index.vertex_index >= 0) {
        vertex.position =
            glm::vec3(attribute.vertices[3 * size_t(index.vertex_index) + 0],
                      attribute.vertices[3 * size_t(index.vertex_index) + 1],
                      attribute.vertices[3 * size_t(index.vertex_index) + 2]);

        vertex.color =
            glm::vec3(attribute.colors[3 * size_t(index.vertex_index) + 0],
                      attribute.colors[3 * size_t(index.vertex_index) + 1],
                      attribute.colors[3 * size_t(index.vertex_index) + 2]);
      }

      // process normal
      if (index.normal_index >= 0) {
        vertex.normal = glm::normalize(
            glm::vec3(attribute.normals[3 * size_t(index.normal_index) + 0],
                      attribute.normals[3 * size_t(index.normal_index) + 1],
                      attribute.normals[3 * size_t(index.normal_index) + 2]));
      }

      // process textcoord
      if (index.texcoord_index >= 0) {
        vertex.uv = glm::vec2(
            attribute.texcoords[2 * size_t(index.texcoord_index) + 0],
            attribute.texcoords[2 * size_t(index.texcoord_index) + 1]);
      }

      auto [_, status] = uniqueVertices.try_emplace(vertex, vertices.size());
      indices.push_back(static_cast<uint32_t>(uniqueVertices[vertex]));
      if (status) {
        vertices.push_back(std::move(vertex));
      }
    }
  }
}

engine::Model::Model(MyEngineDevice &device, const Builder &builder)
    : device_(device) {
  createVertexBuffers(builder.vertices);
  createIndicesBuffers(builder.indices);
}

engine::Model::Model(MyEngineDevice &device, const std::string &path)
    : device_(device) {
  Builder builder{};
  builder.loadObjModel(path);

  createVertexBuffers(builder.vertices);
  createIndicesBuffers(builder.indices);
}

void engine::Model::createVertexBuffers(const std::vector<Vertex> &vertices) {

  vertexCounter_ = static_cast<uint32_t>(vertices.size());
  assert(vertexCounter_ >= 3 && "Vertex Must Be At Least 3!");

  auto bufferSize = sizeof(vertices[0]) * vertices.size();

  Buffer stagingBuffer(device_, sizeof(vertices[0]), vertexCounter_,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)(vertices.data()));

  vertexBuffer_ = std::make_unique<Buffer>(
      device_, sizeof(vertices[0]), vertexCounter_,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device_.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer_->getBuffer(),
                     bufferSize);
}

void engine::Model::createIndicesBuffers(const std::vector<uint32_t> &indices) {
  indicesCounter_ = static_cast<uint32_t>(indices.size());
  hasIndicesArgument_ = indicesCounter_ > 0;
  if (!hasIndicesArgument_) {
    return;
  }
  auto bufferSize = sizeof(indices[0]) * indices.size();

  Buffer stagingBuffer(device_, sizeof(indices[0]), indicesCounter_,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)(indices.data()));

  indexBuffer_ = std::make_unique<Buffer>(
      device_, sizeof(indices[0]), indicesCounter_,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device_.copyBuffer(stagingBuffer.getBuffer(), indexBuffer_->getBuffer(),
                     bufferSize);
}

void engine::Model::bind(VkCommandBuffer commandBuffer) {
  VkBuffer buffers[] = {vertexBuffer_->getBuffer()};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  if (hasIndicesArgument_) {
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_->getBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);
  }
}

void engine::Model::draw(VkCommandBuffer commandBuffer) {
  if (hasIndicesArgument_)
    vkCmdDrawIndexed(commandBuffer, indicesCounter_, 1, 0, 0, 0);
  else
    vkCmdDraw(commandBuffer, vertexCounter_, 1, 0, 0);
}

std::vector<VkVertexInputBindingDescription>
engine::Vertex::getBindingDescriptions() {
  static std::vector<VkVertexInputBindingDescription> ret(1);
  ret[0].binding = 0;
  ret[0].stride = sizeof(Vertex);
  ret[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return ret;
}

std::vector<VkVertexInputAttributeDescription>
engine::Vertex::getAttributeDescriptions() {
  static std::vector<VkVertexInputAttributeDescription> ret(4);
  ret[0].binding = 0;
  ret[0].location = 0; // glsl
  ret[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  ret[0].offset = offsetof(Vertex, position);

  ret[1].binding = 0;
  ret[1].location = 1; // glsl
  ret[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  ret[1].offset = offsetof(Vertex, color);

  ret[2].binding = 0;
  ret[2].location = 2; // glsl
  ret[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  ret[2].offset = offsetof(Vertex, normal);

  ret[3].binding = 0;
  ret[3].location = 3; // glsl
  ret[3].format = VK_FORMAT_R32G32_SFLOAT;
  ret[3].offset = offsetof(Vertex, uv);
  return ret;
}
