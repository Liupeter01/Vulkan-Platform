#pragma once
#ifndef _MESH_NODE_HPP_
#define  _MESH_NODE_HPP_
#include <nodes/BaseNode.hpp>
#include <mesh/MeshLoader.hpp>

namespace engine {
          namespace node {
                    struct MeshNode : public BaseNode {
                              MeshNode(std::shared_ptr<BaseNode> base = nullptr);

                              virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;

                              std::shared_ptr<MeshAsset> mesh_{};
                    };
          }
}

#endif // _MESH_NODE_HPP_