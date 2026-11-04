#include "shader_structs.hlsl"
#include "ibl_common.hlsl"

cbuffer Source : register(b0)
{
    uint sourceIndex;
    uint sourceSampler;
}

cbuffer Target : register(b1)
{
    uint mipIndex;
}

[numthreads(32, 32, 1)]
void main(ComputeShaderInput In)
{
    TextureCube EnvironmentMap = ResourceDescriptorHeap[sourceIndex];
    RWTexture2DArray<float4> PrefilterMap = ResourceDescriptorHeap[mipIndex];
    SamplerState CubeSampler = ResourceDescriptorHeap[sourceSampler];
	
    uint outputWidth, outputHeight, outputDepth;
    PrefilterMap.GetDimensions(outputWidth, outputHeight, outputDepth);
    if (In.DispatchThreadID.x >= outputWidth || In.DispatchThreadID.y >= outputHeight)
    {
        return;
    }

    float3 N = GetSamplingVector(In.DispatchThreadID, PrefilterMap);
    PrefilterMap[In.DispatchThreadID] = float4(EnvironmentMap.SampleLevel(CubeSampler, N, 0).rgb, 1.0);
}