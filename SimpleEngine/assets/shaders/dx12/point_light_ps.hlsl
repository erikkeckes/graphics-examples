#include "common_pbr.hlsl"

ConstantBuffer<DefferedLigth> Textures : register(b1);
ConstantBuffer<PointLight> Light : register(b2);

ConstantBuffer<CameraData> InvCamera : register(b4);

cbuffer Screen : register(b5)
{
    float width;
    float height;
}

float4 main(float4 clip : SV_POSITION) : SV_TARGET0
{
    Texture2D<float> depthBuff = ResourceDescriptorHeap[Textures.depthBuffer];
    SamplerState bufferSampler = ResourceDescriptorHeap[Textures.writeBuffer];
    
    float2 uv = clip.xy * float2(rcp(width), rcp(height));
 
    float depth = depthBuff.Sample(bufferSampler, uv).r;
    if (depth >= 1)
    {
        discard;
    }
    
    Texture2D<float4> albedoRs = ResourceDescriptorHeap[Textures.albodoRougness];
    Texture2D<float4> nomralao = ResourceDescriptorHeap[Textures.normalAo];
    
    float3 albedo = albedoRs.Sample(bufferSampler, uv).rgb;
    float roughness = nomralao.Sample(bufferSampler, uv).a;
    float metallic = nomralao.Sample(bufferSampler, uv).b;
    
    float3 w_pos = GetWorldPos(depth, uv, InvCamera.projection, InvCamera.view);
    
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 f90 = float3(1, 1, 1);
    
    float3 L = Light.position - w_pos;
    float3 intensity = Light.color * Light.intensity * Attenuation(L, Light.range);
    
    float3 N = DecodeNormalOct(nomralao.Sample(bufferSampler, uv).rg);
    float3 V = normalize(InvCamera.eyePosition - w_pos.xyz);
    L = normalize(L);
    float3 H = normalize(V + L);
        
    float NoV = max(dot(N, V), 0.0001);
    float NoL = max(dot(N, L), 0.0001);
    float NoH = max(dot(N, H), 0.0001);
        
    float3 Specular = SpecularLight(NoL, NoV, NoH, f0, roughness) * intensity * NoL;
    float3 Diffuse = DiffuseLight(albedo, metallic, f0, NoV) * intensity * NoL;
    
    float3 fcolor = Diffuse + Specular;
    
    return float4(fcolor, 1);
}