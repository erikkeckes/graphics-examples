#include "shader_structs.hlsl"
#include "ibl_common.hlsl"

cbuffer LutData : register(b0)
{
    uint lutIndex;
}

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

float3 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

    float3 N = float3(0.0, 0.0, 1.0);

    float A = 0.0;
    float B = 0.0;
    float C = 0.0;

    for (int i = 0; i < NumSamples; ++i)
    {
        float4 impSample = GGX(SampleHammersley(i, NumSamples), roughness);
        
        float3 H = impSample.xyz;
        float3 L = normalize(reflect(-V, H));

        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));
        if (NdotL > 0.0)
        {
            float V_pdf = V_SmithGGXCorrelated(NdotV, NdotL, roughness) * VdotH * NdotL / NdotH;
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * V_pdf;
            B += Fc * V_pdf;
        }
    }

    return float3(4.0 * A, 4.0 * B, 0) * InvNumSamples;
}


[numthreads(32, 32, 1)]
void main(ComputeShaderInput In)
{
    RWTexture2D<float2> lutTexture = ResourceDescriptorHeap[lutIndex];
    
    uint width, height;
    lutTexture.GetDimensions(width, height);
    
    if (In.DispatchThreadID.x > width || In.DispatchThreadID.y >= height)
    {
        return;
    }
    
    float2 uv = float2(In.DispatchThreadID.xy + 0.5.xx) / float2(width, height);
    
    lutTexture[In.DispatchThreadID.xy] = IntegrateBRDF(uv.x, uv.y).xy;
}