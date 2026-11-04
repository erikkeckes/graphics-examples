#pragma once
#include "glm/glm.hpp"

namespace SE
{
constexpr uint32_t POINT_LIGHTS_COUNT = 12;
constexpr uint32_t DIRECT_LIGHTS_COUNT = 2;

struct CameraData
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 eyePosition;
};

struct TextureData
{
    uint32_t textureIndex = UINT32_MAX;
    uint32_t samplerIndex = UINT32_MAX;
};

struct MaterialData
{
    glm::vec4 baseColorFactor;
    glm::vec3 emisiveFactor;

    float normalScale;
    float occlusionStrenght;
    float metalicFactor;
    float rougnessFactor;
    uint32_t textureMask;

    TextureData albedaTexture;
    TextureData normalTexture;
    TextureData metalRougnessTexture;
    TextureData emisiveTexture;
    TextureData occlusionTexture;
};

struct PointLightData
{
    glm::vec3 position;
    float intensity;
    glm::vec3 color;
    float range;
};

struct DirectLightData
{
    float intensity;
    glm::vec3 direction;
    glm::vec3 color;
    float pad;  // can be changed for range for shadows
};

struct PointLights
{
    uint32_t pointCount = 0;
    uint32_t pad[3];  // not fully sure why is it needed
    PointLightData pointLights[POINT_LIGHTS_COUNT];
};

struct DirectLights
{
    uint32_t directCount = 0;
    uint32_t pad[3];  // not fully sure why is it needed
    DirectLightData directLights[DIRECT_LIGHTS_COUNT];
};

struct IBLEnviroment
{
    uint32_t lutIndex = 0;
    uint32_t irradianceIndex = 0;
    uint32_t prefilterdIndex = 0;
    uint32_t filterMips = 0;
    uint32_t envSamplerIndex = 0;
    uint32_t lutSamplerIndex = 0;
    uint32_t envirometMask = 0;
};

struct DefferedLigth
{
    uint32_t albodoRougness = 0;
    uint32_t emisiveMetal = 0;
    uint32_t normalAo = 0;
    uint32_t depthBuffer = 0;
    // ping buffers
    uint32_t readBuffer = 0;
    // in light passes used as sampler index
    uint32_t writeBuffer = 0;
};

struct BloomData
{
    uint32_t srv = 0;
    uint32_t uav = 0;
    uint32_t sampler = 0;
    float strength = 0;
};

}  // namespace SE