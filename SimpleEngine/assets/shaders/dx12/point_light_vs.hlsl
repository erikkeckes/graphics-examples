#include "shader_structs.hlsl"

ConstantBuffer<CameraData> Camera : register(b0);

cbuffer ModelMatrix : register(b3)
{
    float4x4 worldMatrix;
}

float4 main(Vertex In) : SV_POSITION
{
    float4 worldPos = mul(worldMatrix, float4(In.position, 1.0));
    float4 viewPos = mul(Camera.view, worldPos);
    return mul(Camera.projection, viewPos);
}