#include "common_pbr.hlsl"
#include "tone_mapping.hlsl"

ConstantBuffer<CameraData> Camera : register(b0);
ConstantBuffer<Material> material : register(b3);

cbuffer DebugMode : register(b4)
{
    uint mode;
}

ConstantBuffer<PointLights> pointLights : register(b5);
ConstantBuffer<DirectLights> directLights : register(b6);
ConstantBuffer<IBLEnviroment> enviroment : register(b7);

float3 F_Schlick(float cosTheta, float3 f0, float roughness)
{
    return f0 + (roughness.xxx - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 main(VStoPS In) : SV_TARGET
{
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
    
    // view vector   
    float3 V = normalize(Camera.eyePosition - In.w_pos.xyz);

    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 f90 = float3(1, 1, 1);
    
    float3 Diffuse = float3(0, 0, 0);
    float3 Specular = float3(0, 0, 0);
    
    for (int i = 0; i < pointLights.pointCount; i++)
    {
        float3 toLight = pointLights.pointLights[i].position - In.w_pos;
        float3 intensity = pointLights.pointLights[i].color * pointLights.pointLights[i].intensity;
        intensity *= Attenuation(toLight, pointLights.pointLights[i].range);
        
        Specular += SpecularLight(N, normalize(toLight), V, f0, intensity, roughness);
        Diffuse += DiffuseLight(albedo, metallic, f0, saturate(dot(N, V))) * intensity * saturate(dot(N, normalize(toLight)));
    }
    
    for (int i = 0; i < directLights.directCount; i++)
    {
        float3 toLight = directLights.directLights[i].direction;
        float3 intensity = directLights.directLights[i].color * directLights.directLights[i].intensity;
        
        Specular += SpecularLight(N, normalize(toLight), V, f0, intensity, roughness);
        Diffuse += DiffuseLight(albedo, metallic, f0, saturate(dot(N, V))) * intensity * saturate(dot(N, normalize(toLight)));
    }
    
    float3 ambientSpecular = 0.xxx;
    float3 ambientDiffuse = 0.xxx;
    float3 ambientSpecularMip = 0.xxx;
    float3 ambient = 0.xxx;
    float3 debugColor = 0.xxx;
    if (enviroment.envirometMask == 1)
    {
        float NoV = saturate(dot(N, V));
        float3 F = F_Schlick(f0, NoV);

        float3 kS = F;
        float3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;
        
        TextureCube specularIBL = ResourceDescriptorHeap[enviroment.prefilterdIndex];
        TextureCube irradianceIBL = ResourceDescriptorHeap[enviroment.irradianceIndex];
        Texture2D LUT = ResourceDescriptorHeap[enviroment.lutIndex];
        SamplerState enviromentSampler = ResourceDescriptorHeap[enviroment.samplerIndex];
        
        ambientDiffuse = albedo * irradianceIBL.Sample(enviromentSampler, N).rgb;
        
        float3 samplingDir = normalize(reflect(-V, N));
        float lod = float(enviroment.filterMips) * (roughness);
        float3 prefilteredColor = specularIBL.SampleLevel(enviromentSampler, samplingDir, lod).rgb;
        float2 envBRDF = LUT.Sample(enviromentSampler, float2(NoV, roughness)).rg;
        // split sum is bad
        ambientSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
        if (mode == 14)
        {
            debugColor = prefilteredColor * F;
        }
        else if (mode == 15)
        {
            debugColor = (F * envBRDF.x + envBRDF.y);
        }
        ambientSpecularMip = lod / float(enviroment.filterMips);
        ambient = (kD * ambientDiffuse + ambientSpecular) * ao;
    }
    
    float3 fcolor = ambient + (Diffuse + Specular) + emisive;
    
    // mode 0 is final color
    if (mode == 1)
    {
        fcolor = SampleTexture(material.albedoTexture, In.uv).rgb;
    }
    else if (mode == 2)
    {
        fcolor = SampleTexture(material.normalTexture, In.uv).rgb;
    }
    else if (mode == 3)
    {
        fcolor = SampleTexture(material.metailcRougnessTexture, In.uv).rgb;
    }
    else if (mode == 4)
    {
        fcolor = SampleTexture(material.emisiveTexture, In.uv).rgb * material.emisiveFactor;
    }
    else if (mode == 5)
    {
        fcolor = SampleTexture(material.occlusionTexture, In.uv).rgb;
    }
    else if (mode == 6)
    {
        fcolor = GetNormalColor(In.normal);
    }
    else if (mode == 7)
    {
        fcolor = GetNormalColor(In.tangent.rgb);
    }
    else if (mode == 8)
    {
        float3 textureNormal = SampleTexture(material.normalTexture, In.uv).rgb;
        fcolor = NormalMapping(In.normal, textureNormal, In.tangent);
    }
    else if (mode == 9)
    {
        fcolor = ambientDiffuse;
    }
    else if (mode == 10)
    {
        fcolor = ambientSpecular;
    }
    else if (mode == 11)
    {
        fcolor = ambientSpecularMip;
    }
    else if (mode == 12)
    {
        fcolor = (Diffuse + Specular) + emisive;
    }
    else if (mode == 13)
    {
        fcolor = ambient;
    }
    else if(mode == 14 || mode == 15)
    {
        fcolor = debugColor;
    }
    else if (mode == 16)
    {
        fcolor = ReinhardToneMapping(fcolor);
    }
    else if (mode == 17)
    {
        fcolor = FilmicToneMapping(fcolor);
    }
    else if (mode == 18)
    {
        fcolor = AcesToneMapping(fcolor);
    }
    
    return float4(fcolor, 1);
}