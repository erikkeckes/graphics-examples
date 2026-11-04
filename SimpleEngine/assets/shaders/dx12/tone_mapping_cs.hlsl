#include "shader_structs.hlsl"

float3 ReinhardToneMapping(float3 color)
{
    return color / (color + float3(1.0, 1.0, 1.0));
}

float3 FilmicToneMapping(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//https://www.shadertoy.com/view/XsGfWV
float3 AcesToneMapping(float3 color)
{
    const float3x3 ACESInputMat = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );
    const float3x3 ACESOutputMat = float3x3(
        1.60475, -0.53108, -0.07367,
        -0.10208, 1.10813, -0.00605,
        -0.00327, -0.07276, 1.07602
    );
    color = mul(ACESInputMat, color);
    color = (color * (color + 0.0245786f) - 0.000090537f) /
            (color * (0.983729f * color + 0.4329510f) + 0.238081f);
    return mul(ACESOutputMat, color);
}

ConstantBuffer<DefferedLigth> Textures : register(b0);

cbuffer ToneMapping : register(b1)
{
    uint mapping;
}

[numthreads(32, 32, 1)]
void main(ComputeShaderInput In)
{
    RWTexture2D<float4> combinedRv = ResourceDescriptorHeap[Textures.writeBuffer];
  
    uint width = 0, height = 0;
    combinedRv.GetDimensions(width, height);
    
    if (In.DispatchThreadID.x > width || In.DispatchThreadID.y >= height)
    {
        return;
    }
    
    float3 color = combinedRv[In.DispatchThreadID.xy].rgb;
    
    if (mapping == 1)
    {
        color = ReinhardToneMapping(color);
    }
    else if (mapping == 2)
    {
        color = FilmicToneMapping(color);
    }
    else if (mapping == 3)
    {
        color = AcesToneMapping(color);
    }
    
    combinedRv[In.DispatchThreadID.xy] = float4(color, 1);
}