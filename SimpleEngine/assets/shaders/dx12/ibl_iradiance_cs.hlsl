#include "shader_structs.hlsl"
#include "ibl_common.hlsl"

ConstantBuffer<TetureData> Enviroment : register(b1);

cbuffer IradianceMap : register(b2)
{
    uint iradianceMap;
}

cbuffer SamplingData : register(b3)
{
    uint SourceWidth;
}

static const uint NumSamples = 8 * 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

float3 SampleBrute(float3 N)
{
    TextureCube<float4> envMap = ResourceDescriptorHeap[Enviroment.textureIndex];
    SamplerState envSampler = ResourceDescriptorHeap[Enviroment.samplerIndex];
    
    float3x3 rotMatrix = GetRotationMatrix(N);
    
    float3 irradiance = 0.0;
    float totalMips = 0;
    
    for (uint i = 0; i < NumSamples; ++i)
    {
        float4 sample = Lambertian(SampleHammersley(i, NumSamples));
        float3 Li = mul(sample.xyz, rotMatrix);
        float lod = GetLod(SourceWidth, NumSamples, sample.w);
         
        irradiance += envMap.SampleLevel(envSampler, Li, lod).rgb;
        totalMips += lod;
    }
   
    return irradiance * InvNumSamples;
}

[numthreads(32, 32, 1)]
void main(ComputeShaderInput In)
{
    RWTexture2DArray<float4> irradianceMap = ResourceDescriptorHeap[iradianceMap];
    
    float3 N = GetSamplingVector(In.DispatchThreadID, irradianceMap);
    
    irradianceMap[In.DispatchThreadID] = float4(SampleBrute(N).rgb, 1);
}