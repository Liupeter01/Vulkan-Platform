#pragma once
#ifndef _NODE_MANAGER_HPP_
#define _NODE_MANAGER_HPP_
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <nodes/BaseNode.hpp>

namespace engine {

          namespace mesh {
                    struct MeshAsset;
          }

          namespace node {
                    struct NodeManager {
                              virtual ~NodeManager();

                              void init(const std::string& root_name = "/root");
                              void destroy();
                              void draw(const glm::mat4& parentMatrix, DrawContext& ctx);

                              //CRUD
                              bool attachChildren(const std::string& parentName, std::shared_ptr<MeshAsset> asset);
                              bool attachChildren(const std::string& parentName, std::shared_ptr<node::BaseNode> child);
                              bool attachChildrens(const std::string& parentName,
                                        const std::vector<std::shared_ptr<node::BaseNode>>& childrens);
                              bool attachChildrens(const std::string& parentName,
                                        const std::vector<std::shared_ptr<MeshAsset>>& childrens);

                              [[nodiscard]] std::optional<std::shared_ptr<node::BaseNode>> findNode(const std::string& name) const;
                              [[nodiscard]] std::optional<std::shared_ptr<MeshAsset>>findMesh(const std::string& name) const;

                    protected:
                              //Node Query System(automatically recurse all the children nodes)
                              bool insert(std::shared_ptr<node::BaseNode> parent, std::shared_ptr<node::BaseNode> child);         //push_back + 
                              bool insert_to_lookup(std::shared_ptr<node::BaseNode> node, const std::string& parentPath);

                              void create_root(const std::string& root_name = "/root");

                              //remove all tree recursively!
                              void remove_nodes(std::shared_ptr<node::BaseNode> root);

                    private:
                              bool isinit = false;

                              std::shared_ptr<node::BaseNode> scene_root_ = nullptr;
                              std::unordered_map</*path = */std::string, std::shared_ptr<node::BaseNode>> lookup_path;
                              std::unordered_map</*mesh_name = */std::string, std::shared_ptr<MeshAsset>> lookup_mesh;
                    };
          }
}

#endif //_NODE_MANAGER_HPP_