#include "common_pbr.hlsl"


ConstantBuffer<CameraData> Camera : register(b0);
ConstantBuffer<Material> material : register(b2);

float2 EncodeNormalOctahedral(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    float2 enc = n.xy;
    if (n.z < 0.0)
        enc = (1.0 - abs(enc.yx)) * sign(enc.xy);
    return enc * 0.5 + 0.5;
}

GbufferOutput main(VStoPS In)
{
    GbufferOutput Out;
    
    float3 albedo = float3(0, 0, 0);
    
    float3 N = In.normal;
    float3 emisive = float3(0, 0, 0);
    float roughness = material.rougnessFactor;
    float metallic = material.metalicFactor;
    float ao = 0;
    
    if (material.textureMask & ALBEDO_MASK)
    {
        albedo = SampleTexture(material.albedoTexture, In.uv).rgb;
        albedo *= material.baseColorFactor.rgb;
    }
    
    if (material.textureMask & METALROUGNESS_MASK)
    {
        float3 metalRougness = SampleTexture(material.metailcRougnessTexture, In.uv).rgb;
        metallic = metalRougness.b;
        roughness = metalRougness.g;
    }
    
    if (material.textureMask & NORMAL_MASK)
    {
        float3 textureNormal = SampleTexture(material.normalTexture, In.uv).rgb;
        N = NormalMapping(N, textureNormal, In.tangent);
    }
    
    if (material.textureMask & EMISIVE_MASK)
    {
        emisive = SampleTexture(material.emisiveTexture, In.uv).rgb;
        emisive *= material.emisiveFactor;
    }
    
    if (material.textureMask & OCCLUSION_MASK)
    {
        ao = SampleTexture(material.occlusionTexture, In.uv).r;
        ao *= material.occlusionStrenght;
    }
    
    Out.albedo = float4(albedo, 1);
    Out.emisive = float4(emisive, ao);
    Out.normal = float4(EncodeNormalOctahedral(N), metallic, roughness);
    
    return Out;
}