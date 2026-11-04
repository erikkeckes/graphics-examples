#include "common_pbr.hlsl"

ConstantBuffer<CameraData> Camera : register(b0);
ConstantBuffer<DefferedLigth> Textures : register(b1);
ConstantBuffer<IBLEnviroment> enviroment : register(b2);

cbuffer Screen : register(b3)
{
    float width;
    float height;
}

float4 main(float4 clip : SV_POSITION) : SV_TARGET0
{
    if (enviroment.envirometMask != 1)
    {
        discard;
    }
    
    Texture2D<float> depthBuff = ResourceDescriptorHeap[Textures.depthBuffer];
    SamplerState bufferSampler = ResourceDescriptorHeap[Textures.writeBuffer];
    
    float2 uv = clip.xy * float2(rcp(width), rcp(height));
    
    float depth = depthBuff.Sample(bufferSampler, uv).r;
    if (depth >= 1)
    {
        discard;
    }
    
    Texture2D<float4> albedoRs = ResourceDescriptorHeap[Textures.albodoRougness];
    Texture2D<float4> emisiveMt = ResourceDescriptorHeap[Textures.emisiveMetal];
    Texture2D<float4> nomralao = ResourceDescriptorHeap[Textures.normalAo];
    
    float3 albedo = albedoRs.Sample(bufferSampler, uv).rgb;
    float roughness = nomralao.Sample(bufferSampler, uv).a;
    float metallic = nomralao.Sample(bufferSampler, uv).b;
    float3 emisive = emisiveMt.Sample(bufferSampler, uv).rgb;
    float ao = albedoRs.Sample(bufferSampler, uv).a;
    
    float3 w_pos = GetWorldPos(depth, uv, Camera.projection, Camera.view);
    
    float3 N = DecodeNormalOct(nomralao.Sample(bufferSampler, uv).rg);
    float3 V = normalize(Camera.eyePosition - w_pos);
    
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float NoV = saturate(dot(N, V));
    float3 F = F_Schlick(f0, NoV);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
        
    TextureCube specularIBL = ResourceDescriptorHeap[enviroment.prefilterdIndex];
    TextureCube irradianceIBL = ResourceDescriptorHeap[enviroment.irradianceIndex];
    Texture2D LUT = ResourceDescriptorHeap[enviroment.lutIndex];
    SamplerState enviromentSampler = ResourceDescriptorHeap[enviroment.envSamplerIndex];
    SamplerState lutSampler = ResourceDescriptorHeap[enviroment.lutSamplerIndex];
        
    float3 ambientDiffuse = albedo * irradianceIBL.Sample(enviromentSampler, N).rgb;
        
    float3 samplingDir = normalize(reflect(-V, N));
    float lod = float(enviroment.filterMips) * (roughness);
    float3 prefilteredColor = specularIBL.SampleLevel(enviromentSampler, samplingDir, lod).rgb;
    float2 envBRDF = LUT.Sample(lutSampler, float2(NoV, roughness)).rg;
    float3 ambientSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    float3 fcolor = (kD * ambientDiffuse + ambientSpecular) * ao + emisive;
    
    return float4(fcolor, 1);
}