#include "components/rendering_components.hpp"
#include "rendering/core/sampler_dx.hpp"
#include "resources/model.hpp"
#include "tools/tools.hpp"

void SE::Sampler::HashSampler() { m_hash = D3D12Sampler::CreateSampler(*this); }

SE::Material::Material(const Model& model, int index)
{
    const auto& mat = model.GetModel().materials[index];
    const auto& images = model.GetTextures();

    const auto& pbr = mat.pbrMetallicRoughness;

    baseColorFactor = to_vec4(pbr.baseColorFactor);
    emissiveFactor = to_vec3(mat.emissiveFactor);

    normalTextureScale = static_cast<float>(mat.normalTexture.scale);
    occlusionTextureStrength = static_cast<float>(mat.occlusionTexture.strength);

    metallicFactor = static_cast<float>(pbr.metallicFactor);
    roughnessFactor = static_cast<float>(pbr.roughnessFactor);

    if (pbr.baseColorTexture.index >= 0)
    {
        baseColorTexture = images[pbr.baseColorTexture.index];
        textureMask |= TextureMask::Albedo;
    }

    if (pbr.metallicRoughnessTexture.index >= 0)
    {
        metallicRoughnessTexture = images[pbr.metallicRoughnessTexture.index];
        textureMask |= TextureMask::MetalRougness;
    }

    if (mat.normalTexture.index >= 0)
    {
        normalTexture = images[mat.normalTexture.index];
        textureMask |= TextureMask::Normal;
    }

    if (mat.occlusionTexture.index >= 0)
    {
        occlusionTexture = images[mat.occlusionTexture.index];
        textureMask |= TextureMask::Occlusion;
    }

    if (mat.emissiveTexture.index >= 0)
    {
        emissiveTexture = images[mat.emissiveTexture.index];
        textureMask |= TextureMask::Emisive;
    }
}

SE::Sampler::Filter GetFilter(int filter)
{
    switch (filter)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return SE::Sampler::Filter::Nearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return SE::Sampler::Filter::Linear;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        return SE::Sampler::Filter::NearestMipNearest;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        return SE::Sampler::Filter::LinearMipNearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        return SE::Sampler::Filter::NearestMipLinear;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        return SE::Sampler::Filter::LinearMipLinear;
    default:
        return SE::Sampler::Filter::Nearest;
    }
}

SE::Sampler::Wrap GetWrap(int wrap)
{
    switch (wrap)
    {
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return SE::Sampler::Wrap::ClampEdge;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return SE::Sampler::Wrap::MiroredRepeat;
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return SE::Sampler::Wrap::Repeat;
    default:
        return SE::Sampler::Wrap::ClampEdge;
    }
}

SE::Sampler::Sampler(const Model& model, int index)
{
    const auto& sampler = model.GetModel().samplers[index];
    magFiler = GetFilter(sampler.magFilter);
    minFiler = GetFilter(sampler.minFilter);
    wrapS = GetWrap(sampler.wrapS);
    wrapT = GetWrap(sampler.wrapT);

    HashSampler();
}

SE::Light::LightType GetType(const std::string& light)
{
    if (light == "point")
    {
        return SE::Light::LightType::Point;
    }
    else if (light == "directional")
    {
        return SE::Light::LightType::Direct;
    }
    else
    {
        return SE::Light::LightType::Spot;
    }
}

SE::Light::Light(const Model& model, int index)
{
    const auto& light = model.GetModel().lights[index];
    color = to_vec3(light.color);
    intensity = static_cast<float>(light.intensity);
    type = GetType(light.type);
}

SE::Texture::Texture(const Model& model, int index)
{
    const auto& texture = model.GetModel().textures[index];

    image = model.GetImages()[texture.source];
    if (texture.sampler >= 0)
    {
        sampler = model.GetSamplers()[texture.sampler];
    }
}
