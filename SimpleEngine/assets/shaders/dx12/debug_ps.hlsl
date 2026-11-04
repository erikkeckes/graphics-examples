#include "common_pbr.hlsl"

struct ToPs
{
    float4 clip : SV_POSITION;
    float2 uv : TEXCOORD0;
};

ConstantBuffer<CameraData> Camera : register(b0);
ConstantBuffer<DefferedLigth> Textures : register(b1);
ConstantBuffer<IBLEnviroment> enviroment : register(b2);

cbuffer DebugMode : register(b3)
{
    uint mode;
}

cbuffer Screen : register(b4)
{
    float width;
    float height;
}

float4 main(float4 clip : SV_POSITION) : SV_TARGET0
{
    Texture2D<float4> albedoRs = ResourceDescriptorHeap[Textures.albodoRougness];
    Texture2D<float4> emisiveMt = ResourceDescriptorHeap[Textures.emisiveMetal];
    Texture2D<float4> nomralao = ResourceDescriptorHeap[Textures.normalAo];
    Texture2D<float> depthBuff = ResourceDescriptorHeap[Textures.depthBuffer];
    
    SamplerState bufferSampler = ResourceDescriptorHeap[Textures.writeBuffer];
    
    float2 uv = clip.xy * float2(rcp(width), rcp(height));
    
    float3 albedo = albedoRs.Sample(bufferSampler, uv).rgb;
    float roughness = nomralao.Sample(bufferSampler, uv).a;
    float metallic = nomralao.Sample(bufferSampler, uv).b;
    float3 normal = DecodeNormalOct(nomralao.Sample(bufferSampler, uv).rg);
    float3 emisive = emisiveMt.Sample(bufferSampler, uv).rgb;
    
    float ao = emisiveMt.Sample(bufferSampler, uv).a;
    
    if (mode == 1)
    {
        return float4(albedo, 1);
    }
    else if (mode == 2)
    {
        return float4(normal, 1);
    }
    else if (mode == 3)
    {
        return float4(metallic.xxx, 1);
    }
    else if (mode == 4)
    {
        return float4(roughness.xxx, 1);
    }
    else if (mode == 5)
    {
        return float4(emisive, 1);
    }
    else if(mode == 6)
    {
        return float4(ao.xxx, 1);
    }
    
    float depth = depthBuff.Sample(bufferSampler, uv).r;
    
    if(depth >= 1)
    {
        return float4(0.xxx, 1);
    }
    
    float3 w_pos = GetWorldPos(depth, uv, Camera.projection, Camera.view);
    float3 V = normalize(Camera.eyePosition - w_pos);
    
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float NoV = saturate(dot(normal, V));
    float3 F = F_Schlick(f0, NoV);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
        
    TextureCube specularIBL = ResourceDescriptorHeap[enviroment.prefilterdIndex];
    TextureCube irradianceIBL = ResourceDescriptorHeap[enviroment.irradianceIndex];
    Texture2D LUT = ResourceDescriptorHeap[enviroment.lutIndex];
    SamplerState enviromentSampler = ResourceDescriptorHeap[enviroment.envSamplerIndex];
    SamplerState lutSampler = ResourceDescriptorHeap[enviroment.lutSamplerIndex];
        
    float3 ambientDiffuse = albedo * irradianceIBL.Sample(enviromentSampler, normal).rgb;
    
    float3 samplingDir = normalize(reflect(-V, normal));
    float lod = float(enviroment.filterMips) * (roughness);
    float3 prefilteredColor = specularIBL.SampleLevel(enviromentSampler, samplingDir, lod).rgb;
    float2 envBRDF = LUT.Sample(lutSampler, float2(NoV, roughness)).rg;
    float3 ambientSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    
    if (mode == 8)
    {
        return float4(ambientDiffuse, 1);
    }
    else if (mode == 9)
    {
        return float4(ambientSpecular, 1);
    }
    else if (mode == 11)
    {
        return float4(prefilteredColor, 1);
    }
    else if (mode == 12)
    {
        return float4((F * envBRDF.x + envBRDF.y), 1);
    }

    float3 fcolor = (kD * ambientDiffuse + ambientSpecular) * ao + emisive;
    
    return float4(fcolor, 1);
}