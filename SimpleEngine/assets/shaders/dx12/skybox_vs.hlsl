#include "shader_structs.hlsl"

cbuffer Camera : register(b0)
{
    matrix mvp;
}

CubeMapGeneration main(CubeMapGenerationVertex In)
{
    CubeMapGeneration toPs;
    
    toPs.position = mul(mvp, float4(In.position, 1)).xyww;
    toPs.w_pos = In.position;
    
    return toPs;
}