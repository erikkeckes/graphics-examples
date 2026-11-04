#pragma once
#include "glm/glm.hpp"
#include <memory>

namespace SE
{
class Mesh;
class Model;
class Image;
class Skybox;

struct Camera
{
    Camera(const glm::mat4& proj) : projection(proj) {}

    glm::mat4 projection = glm::mat4(1);
};

struct Light
{
    Light() = default;
    Light(const Model& model, int index);

    enum LightType : uint32_t
    {
        Direct = 1,
        Point = 1 << 1,
        Spot = 1 << 2
    };

    glm::vec3 color{};
    float intensity = 0;
    float range = 1.f;
    LightType type = LightType::Direct;
};

class Sampler
{
public:
    Sampler(const Model& model, int index);
    Sampler() = delete;

    void HashSampler();

    enum class Filter
    {
        Nearest = 1,
        Linear = 1 << 1,
        NearestMipNearest = 1 << 2,
        LinearMipNearest = 1 << 3,
        NearestMipLinear = 1 << 4,
        LinearMipLinear = 1 << 5
    };

    enum class Wrap
    {
        ClampEdge = 1,
        MiroredRepeat = 1 << 1,
        Repeat = 1 << 2
    };

    Filter magFiler = Filter::Nearest;
    Filter minFiler = Filter::Nearest;
    Wrap wrapS = Wrap::ClampEdge;
    Wrap wrapT = Wrap::ClampEdge;

private:
    friend class Renderer;

    uint64_t m_hash = 0;
};

struct Texture
{
    Texture(const Model& model, int index);
    std::shared_ptr<Image> image;
    std::shared_ptr<Sampler> sampler;
};

struct Material
{
    enum TextureMask : uint8_t
    {
        Albedo = 1,
        Emisive = 1 << 1,
        Normal = 1 << 2,
        Occlusion = 1 << 3,
        MetalRougness = 1 << 4
    };

    Material(const Model& model, int index);
    Material() = default;

    glm::vec4 baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
    float normalTextureScale = 0.0f;
    float occlusionTextureStrength = 0.0f;
    float metallicFactor = 0.0f;
    float roughnessFactor = 1.0f;

    uint32_t textureMask = 0;

    std::shared_ptr<Texture> baseColorTexture;
    std::shared_ptr<Texture> normalTexture;
    std::shared_ptr<Texture> metallicRoughnessTexture;
    std::shared_ptr<Texture> emissiveTexture;
    std::shared_ptr<Texture> occlusionTexture;
};

struct MeshInstance
{
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> materail;
};

struct Enviroment
{
    std::shared_ptr<Skybox> m_skyox;
};

}  // namespace SE