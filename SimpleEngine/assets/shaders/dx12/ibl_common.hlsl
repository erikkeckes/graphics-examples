#pragma once
#define PI 3.1415926535897932384626433832795
#define TwoPI (2 * PI)

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 SampleHammersley(uint i, uint samples)
{
    return float2(float(i) / float(samples), RadicalInverse_VdC(i));
}

float3 GetSamplingVector(uint3 ThreadID, RWTexture2DArray<float4> outputMap)
{
    float outputWidth, outputHeight, outputDepth;
    outputMap.GetDimensions(outputWidth, outputHeight, outputDepth);

    float2 st = (ThreadID.xy + 0.5.xx) / float2(outputWidth, outputHeight);
    float2 uv = 2.0 * float2(st.x, st.y) - 1.0;

	// Select vector based on cubemap face index.
    float3 dir;
    switch (ThreadID.z)
    {
        case 0:
            dir = float3(1, -uv.y, -uv.x);
            break; // +X
        case 1:
            dir = float3(-1, -uv.y, uv.x);
            break; // -X
        case 2:
            dir = float3(uv.x, 1, uv.y);
            break; // +Y
        case 3:
            dir = float3(uv.x, -1, -uv.y);
            break; // -Y
        case 4:
            dir = float3(uv.x, -uv.y, 1);
            break; // +Z
        case 5:
            dir = float3(-uv.x, -uv.y, -1);
            break; // -Z
    }
    
    return normalize(dir);
}

float3x3 GetRotationMatrix(float3 N)
{
    float3 bitangent = float3(0, 1, 0);

    float dotUp = dot(N, float3(0, 1, 0));
    if (1 - abs(dotUp) <= 0.00001)
    {
        if (dotUp > 0)
        {
            bitangent = float3(1, 0, 0);
        }
        else
        {
            bitangent = float3(0, 0, -1);
        }
    }

    float3 tangent = normalize(cross(bitangent, N));
    bitangent = cross(N, tangent);

    return float3x3(tangent, bitangent, N);
}

float D_GGX(float NdotH, float roughness)
{
    float a = NdotH * roughness;
    float k = roughness / (1.0 - NdotH * NdotH + a * a);
    return k * k * (1.0 / PI);
}

float4 GGX(float2 xi, float roughness)
{
    float alpha = roughness * roughness;
    float cosTheta = saturate(sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y)));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * xi.x;

    float pdf = D_GGX(cosTheta, alpha);
    pdf /= 4.0;
    
    float3 localSpaceDirection = normalize(float3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    ));

    return float4(localSpaceDirection, pdf);
}

float4 Lambertian(float2 xi)
{
    float cosTheta = sqrt(1.0 - xi.y);
    float sinTheta = sqrt(xi.y);
    float phi = 2.0 * PI * xi.x;

    float pdf = cosTheta / PI;
        
    float3 localSpaceDirection = normalize(float3(
        sinTheta * cos(phi),
        sinTheta * sin(phi),
        cosTheta
    ));
    
    return float4(localSpaceDirection, pdf);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
    float a2 = pow(roughness, 4.0);
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float GetLod(uint width, uint samples, float pdf)
{
    return 0.5 * log2(6.0 * float(width) * float(width) / (float(samples) * pdf));
}