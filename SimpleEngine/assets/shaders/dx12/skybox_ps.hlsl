#include "shader_structs.hlsl"

ConstantBuffer<TetureData> skybox : register(b1);

float4 main(CubeMapGeneration In): SV_TARGET
{
    TextureCube<float4> cubemap = ResourceDescriptorHeap[skybox.textureIndex];
    SamplerState s = ResourceDescriptorHeap[skybox.samplerIndex];
    
    return cubemap.Sample(s, In.w_pos);
}