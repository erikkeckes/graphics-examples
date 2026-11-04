#include "shader_structs.hlsl"
#include "ibl_common.hlsl"

#define invAtan float2(0.1591, 0.3183)

cbuffer MapData : register(b2)
{
    uint hdrIndex;
    uint cubemapIndex;
    uint samplerIndex;
}

float2 SampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

[numthreads(32, 32, 1)]
void main(ComputeShaderInput In)
{
    RWTexture2DArray<float4> envMap = ResourceDescriptorHeap[cubemapIndex];
    Texture2D<float4> hdrMap = ResourceDescriptorHeap[hdrIndex];
    SamplerState envSampler = ResourceDescriptorHeap[samplerIndex];
    
    float3 N = GetSamplingVector(In.DispatchThreadID, envMap);
    
    float2 uv = SampleSphericalMap(N);
    
    envMap[In.DispatchThreadID] = float4(hdrMap.Sample(envSampler, uv).rgb, 1);
}