#pragma once
#define ALBEDO_MASK 1
#define EMISIVE_MASK 2
#define NORMAL_MASK 4
#define OCCLUSION_MASK 8
#define METALROUGNESS_MASK 16
#define POINT_LIGHTS_COUNT 12
#define DIRECT_LIGHTS_COUNT 2

struct Vertex
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT0;
};

struct VStoPS
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT0;
    float3 w_pos : POSITIONT0;
};

struct CubeMapGenerationVertex
{
    float3 position : POSITION;
};

struct CubeMapGeneration
{
    float4 position : SV_POSITION;
    float3 w_pos : POSITIONT0;
};

struct CameraData
{
    float4x4 view;
    float4x4 projection;
    float3 eyePosition;
};

struct TetureData
{
    uint textureIndex;
    uint samplerIndex;
};

struct Material
{
    float4 baseColorFactor;
    float3 emisiveFactor;
    
    float normalScale;
    float occlusionStrenght;
    float metalicFactor;
    float rougnessFactor;
  
    uint textureMask;
    
    uint2 albedoTexture;
    uint2 normalTexture;
    uint2 metailcRougnessTexture;
    uint2 emisiveTexture;
    uint2 occlusionTexture;
    
    // TextureData struct has extra padding, until I found out why using uint2
    //TetureData albedoTexture;
    //TetureData normalTexture;
    //TetureData metailcRougnessTexture;
    //TetureData emisiveTexture;
    //TetureData occlusionTexture;
};

struct PointLight
{
    float3 position;
    float intensity;
    float3 color;
    float range;
};

struct DirectLight
{
    float intensity;
    float3 direction;
    float3 color;
};

struct PointLights
{
    uint pointCount;
    PointLight pointLights[POINT_LIGHTS_COUNT];
    
};

struct DirectLights
{
    uint directCount;
    DirectLight directLights[DIRECT_LIGHTS_COUNT];    
};

struct IBLEnviroment
{
    uint lutIndex;
    uint irradianceIndex;
    uint prefilterdIndex;
    uint filterMips;
    uint envSamplerIndex;
    uint lutSamplerIndex;
    uint envirometMask;
};

struct ComputeShaderInput
{
    uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
    uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

struct GbufferOutput
{
    float4 albedo : SV_TARGET; // alpha channel is used for rougness
    float4 emisive : SV_TARGET1; // alpha is used for metalic
    float4 normal : SV_TARGET2; // alpha is used for ao
};

struct DefferedLigth
{
    uint albodoRougness;
    uint emisiveMetal;
    uint normalAo;
    uint depthBuffer;
    uint readBuffer; 
    uint writeBuffer; // in light passes it is used as sampler index
};
