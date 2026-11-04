#include "shader_structs.hlsl"

ConstantBuffer<CameraData> Camera : register(b0);

cbuffer MVP : register(b1)
{
    float4x4 world;
};

float4 main(Vertex In) : SV_Position
{
    float4 w = mul(world, float4(In.position, 1));
    return mul(Camera.projection, mul(Camera.view, w));
}