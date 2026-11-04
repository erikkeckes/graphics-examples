#define invAtan float2(0.1591, 0.3183)

#include "shader_structs.hlsl"

cbuffer texture : register(b1)
{
    uint hdrTexture;
};

cbuffer sampelr : register(b2)
{
    uint samplerIndex;
}

float2 SampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

float4 main(CubeMapGeneration In) : SV_TARGET
{
    float2 uv = SampleSphericalMap(normalize(In.w_pos));
    Texture2D<float4> text = ResourceDescriptorHeap[hdrTexture];
    SamplerState s = ResourceDescriptorHeap[samplerIndex];
    
    return text.Sample(s, uv);

}