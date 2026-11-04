#pragma once
#include "shader_structs.hlsl"
#define PI 3.1415926535897932384626433832795
#define TwoPI (2 * PI)
#define Epsilon 0.0001

float4 SampleTexture(TetureData textureData, float2 uv)
{
    // uintmax is invalid texture
    if (textureData.textureIndex == 0xffffffff)
    {
        return float4(0, 0, 0, 0);
    }
    Texture2D<float4> texture = ResourceDescriptorHeap[textureData.textureIndex];
    SamplerState textureSampler = ResourceDescriptorHeap[textureData.samplerIndex];
    return texture.Sample(textureSampler, uv);
}

float4 SampleTexture(uint2 textureData, float2 uv)
{
   // uintmax is invalid texture
    if (textureData.x == 0xffffffff)
    {
        return float4(0, 0, 0, 0);
    }
    Texture2D<float4> texture = ResourceDescriptorHeap[textureData.x];
    SamplerState textureSampler = ResourceDescriptorHeap[textureData.y];
    return texture.Sample(textureSampler, uv);
}

float Attenuation(float3 direction, float range)
{
    float distance = length(direction);
    float3 attenuation = 1.0 / (distance * distance + 0.0001);
    return smoothstep(range, 0.0, distance);
}

float3 GetNormalColor(float3 normal)
{
    return (normal + float3(1, 1, 1)) * 0.5;
}

float3 NormalMapping(float3 N, float3 sampledNormal, float4 tangent)
{
    float3x3 TBN = float3x3(tangent.xyz, normalize(cross(N, tangent.xyz)) * tangent.w, N);
    float3 newN = sampledNormal * 2 - 1;
    return normalize(mul(newN, TBN).xyz);
}

float Attenuation(float distance, float range)
{
    // GLTF 2.0 uses quadratic attenuation, but with range
    float distance2 = distance * distance;
    return max(min(1.0 - pow(distance / range, 4), 1), 0) / distance2;
}

float3 F_Schlick(float3 f0, float LoH)
{
    return (f0 + (1.0f - f0) * pow(1.0f - LoH, 5.0f));
}

float D_GGX(float NdotH, float a)
{
    float k = (NdotH * NdotH) * (a * a - 1) + 1;
    return a / (PI * k * k);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_GeometrySmith(float NoV, float NoL, float k)
{
    float ggx1 = GeometrySchlickGGX(NoV, k);
    float ggx2 = GeometrySchlickGGX(NoL, k);

    return ggx1 * ggx2;
}

float G_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
    float a2 = pow(roughness, 4.0);
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float3 BRDF_Lambertian(float3 f0, float3 f90, float3 diffuseColor, in float VoH)
{
    return (diffuseColor / PI);
}

float3 DiffuseModifier(float metallic, float3 F)
{
    float3 kD = 1.0.rrr - F.rrr;
    return kD * 1.0 - metallic;
}

float3 SpecularLight(float NoL, float NoV, float NoH, float3 f0, float roughness)
{  
    float3 F = F_Schlick(f0, NoV);
    float G = G_GeometrySmith(NoV, NoL, roughness);
    float NDF = D_GGX(NoH, roughness * roughness);
    
    return (NDF * G * F) / (4.0 * NoV * NoL + 0.0001);
}

float3 DiffuseLight(float3 albedo, float metalic, float3 f0, float NoV)
{
    float3 F = F_Schlick(f0, NoV);
    return DiffuseModifier(metalic, F) * albedo / PI;
}

float3 GetWorldPos(float depth, float2 uv, float4x4 invPorj, float4x4 invView)
{
    float3 ndc;
    ndc.xy = uv * 2.0f - 1.0f;
    ndc.y *= -1.0f;
    ndc.z = depth;
   
    float4 viewPos = mul(invPorj, float4(ndc, 1));
    viewPos /= viewPos.w;

    float4 worldPos = mul(invView, viewPos);
    return worldPos.xyz;
}

float3 DecodeNormalOct(float2 e)
{
    e = e * 2.0 - 1.0;
    float3 n = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
    float t = saturate(-n.z); // clamp(-n.z, 0, 1)
    n.xy -= t * sign(n.xy); // subtract t when n.xy >= 0, add t when n.xy < 0
    return normalize(n);
}