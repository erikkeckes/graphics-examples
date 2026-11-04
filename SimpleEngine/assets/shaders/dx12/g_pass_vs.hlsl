#include "shader_structs.hlsl"

ConstantBuffer<CameraData> Camera : register(b0);

cbuffer ModelMatrix : register(b1)
{
    float4x4 worldMatrix;
}

VStoPS main(Vertex In)
{
    VStoPS toPS;
    
    toPS.uv = In.uv;
    float4 world = mul(worldMatrix, float4(In.position, 1));
    toPS.w_pos = world.xyz;
    toPS.position = mul(Camera.projection, mul(Camera.view, world));
    toPS.normal = normalize(mul(worldMatrix, float4(In.normal, 0))).xyz;
    toPS.tangent = float4(normalize(mul(worldMatrix, float4(In.tangent.xyz, 0)).xyz), In.tangent.w);
    return toPS;
}