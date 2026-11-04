#include "shader_structs.hlsl"

ConstantBuffer<DefferedLigth> Textures : register(b0);

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
    
    RWTexture2D<float4> precombined = ResourceDescriptorHeap[Textures.readBuffer];
    
    float3 color = precombined[In.DispatchThreadID.xy].rgb;
    
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
    {
        combinedRv[In.DispatchThreadID.xy] = float4(color, 1);
    }
    else
    {
        combinedRv[In.DispatchThreadID.xy] = float4(0, 0, 0, 1);
    }
}