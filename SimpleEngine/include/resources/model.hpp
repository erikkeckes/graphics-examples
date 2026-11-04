#pragma once
#include "core/resource.hpp"
#include "core/ecs.hpp"
#include <vector>
#include <memory>
#include "tinygltf/tiny_gltf.h"

namespace SE
{
    class Mesh;
    class Image;
    struct Texture;
    struct Material;
    struct Light;
    class Sampler;
    struct MeshInstance;

    class Model : public Resource
    {
    public:
        Model(FileIO::Directory directory, const std::string& path);
        ~Model() = default;

        void InstanciateModel(Entity entity, bool lights = false) const;

        const tinygltf::Model& GetModel() const { return m_model; }

        const std::vector<std::vector<std::shared_ptr<Mesh>>>& GetMeshes() const { return m_meshes; }
        const std::vector<std::shared_ptr<Image>>& GetImages() const { return m_images; }
        const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_materials; }
        const std::vector<std::shared_ptr<Light>>& GetLights() const { return m_lights; }
        const std::vector<std::shared_ptr<Sampler>>& GetSamplers() const { return m_samplers; }
        const std::vector<std::shared_ptr<Texture>>& GetTextures() const { return m_textures; }

    private:
        void InstaciateNode(const uint32_t nodeIndex, Entity parent, bool lights) const;

        std::shared_ptr<Material> GetPrimitiveMaterial(const tinygltf::Primitive& primitive) const;

        tinygltf::Model m_model;

        std::vector<std::vector<std::shared_ptr<Mesh>>> m_meshes;
        std::vector<std::shared_ptr<Image>> m_images;
        std::vector<std::shared_ptr<Material>> m_materials;
        std::vector<std::shared_ptr<Light>> m_lights;
        std::vector<std::shared_ptr<Sampler>> m_samplers;
        std::vector<std::shared_ptr<Texture>> m_textures;
    };
}  // namespace SE