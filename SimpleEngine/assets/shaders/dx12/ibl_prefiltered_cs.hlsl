#include "shader_structs.hlsl"
#include "ibl_common.hlsl"

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

struct PrefilterMapSettings
{
    uint EnvironmentMap;
    uint PrefilterMap;
    uint CubeSampler;
};

ConstantBuffer<PrefilterMapSettings> PrefilterSettings : register(b0);
cbuffer RoughnessData : register(b1)
{
    float Roughness;
}

cbuffer SamplingData : register(b2)
{
    uint SourceWidth;
}

[numthreads(32, 32, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
    RWTexture2DArray<float4> PrefilterMap = ResourceDescriptorHeap[PrefilterSettings.PrefilterMap];
	
    uint outputWidth, outputHeight, outputDepth;
    PrefilterMap.GetDimensions(outputWidth, outputHeight, outputDepth);
    if (ThreadID.x >= outputWidth || ThreadID.y >= outputHeight)
    {
        return;
    }

    TextureCube EnvironmentMap = ResourceDescriptorHeap[PrefilterSettings.EnvironmentMap];
    SamplerState CubeSampler = ResourceDescriptorHeap[PrefilterSettings.CubeSampler];
    
    
    float3 direction = GetSamplingVector(ThreadID, PrefilterMap);

    float a = Roughness * Roughness;
    float3 V = direction;
    float3 N = direction;
    
    float3 color = 0.xxx;
    float weight = 0;
    float3x3 mat = GetRotationMatrix(N);
    
    for (int i = 0; i < NumSamples; i++)
    {
        float4 impSample = GGX(SampleHammersley(i, NumSamples), Roughness);
        
        float3 H = mul(impSample.xyz, mat);
        float3 L = normalize(reflect(-V, H));
        
        float NoL = saturate(dot(N, L));

        if (NoL > 0)
        {
            float lod = GetLod(SourceWidth, NumSamples, impSample.w);
            
            color += EnvironmentMap.SampleLevel(CubeSampler, L, lod).rgb * NoL;
            weight += NoL;
        }
    }
    
    color =  color / weight;
    
    PrefilterMap[ThreadID] = float4(color, 1);
}