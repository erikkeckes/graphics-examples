#include "shader_structs.hlsl"

// group shared memory for color channels, shared between threads in group
groupshared float gs_R[64];
groupshared float gs_G[64];
groupshared float gs_B[64];
groupshared float gs_A[64];

struct MipDataGeneration
{
    // from which mip to sample
    uint srvView;
    
    // linear clamp sampler
    uint samplerIndex;
    
    // how many mips should be generated
    uint mipCount;
    
    // from whihc mip to sample
    uint srcMip;
    
    // target mips
    uint4 mips;
    
    float2 TexelSize; // 1.0 / OutMip1.Dimensions
    
    uint srgb;
};

ConstantBuffer<MipDataGeneration> MipData : register(b0);


float3 LinearToSrgb(float3 linearColor)
{
    return lerp(linearColor * 12.92, (linearColor + 0.055) * pow(1.055, 1.0 / 2.4) - 0.055, step(0.0031308, linearColor));;
}

void StoreColor(uint Index, float4 Color)
{
    gs_R[Index] = Color.r;
    gs_G[Index] = Color.g;
    gs_B[Index] = Color.b;
    gs_A[Index] = Color.a;
}

float4 LoadColor(uint Index)
{
    return float4(gs_R[Index], gs_G[Index], gs_B[Index], gs_A[Index]);
}

float4 PackMipColor(float4 color)
{
    if(MipData.srgb > 0)
    {
        return float4(LinearToSrgb(color.rgb), color.a);
    }
    return color;
}

[numthreads(8, 8, 1)]
void main(ComputeShaderInput In)
{
    float4 src0 = float4(0, 0, 0, 0);
    
    Texture2D src = ResourceDescriptorHeap[MipData.srvView];
    SamplerState linearSampler = ResourceDescriptorHeap[MipData.samplerIndex];
    
    // load from source mip
    float2 UV = MipData.TexelSize * (In.DispatchThreadID.xy + 0.5);
    src0 = src.SampleLevel(linearSampler, UV, MipData.srcMip);
    
    // first mip
    RWTexture2D<float4> m1 = ResourceDescriptorHeap[MipData.mips[0]];
    m1[In.DispatchThreadID.xy] = PackMipColor(src0);
    
    if(MipData.mipCount == 1)
    {
        return;
    }
    
    // store loaded color and synchronize group memory
    StoreColor(In.GroupIndex, src0);
    
    // synchronizes all thread in group
    GroupMemoryBarrierWithGroupSync();
    
    // checking for even values on X and Y thread ID
    // each uses 3 bits since we specified 8 on top of shader
    // 0x9 -> 001001 upper bits for Y lowwer bit for X
    if ((In.GroupIndex & 0x9) == 0)
    {
        float4 Src2 = LoadColor(In.GroupIndex + 0x01);
        float4 Src3 = LoadColor(In.GroupIndex + 0x08);
        float4 Src4 = LoadColor(In.GroupIndex + 0x09);
        src0 = 0.25 * (src0 + Src2 + Src3 + Src4);

        RWTexture2D<float4> m2 = ResourceDescriptorHeap[MipData.mips[1]];
        m2[In.DispatchThreadID.xy / 2] = PackMipColor(src0);
        StoreColor(In.GroupIndex, src0);
    }

    if (MipData.mipCount == 2)
    {
        return;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    // checking for multiples of 4, second mip has increased sampling size from 2 to 4
    // 0x1B -> 011011 will check will check for that
    if ((In.GroupIndex & 0x1B) == 0)
    {
        float4 Src2 = LoadColor(In.GroupIndex + 0x02);
        float4 Src3 = LoadColor(In.GroupIndex + 0x10);
        float4 Src4 = LoadColor(In.GroupIndex + 0x12);
        src0 = 0.25 * (src0 + Src2 + Src3 + Src4);

        RWTexture2D<float4> m3 = ResourceDescriptorHeap[MipData.mips[2]];
        m3[In.DispatchThreadID.xy / 4] = PackMipColor(src0);
        StoreColor(In.GroupIndex, src0);
    }

    if (MipData.mipCount == 3)
    {
        return;
    }

    GroupMemoryBarrierWithGroupSync();
    
    // last mip should be generated only once since it covers entire max texel size
    if (In.GroupIndex == 0)
    {
        float4 Src2 = LoadColor(In.GroupIndex + 0x04);
        float4 Src3 = LoadColor(In.GroupIndex + 0x20);
        float4 Src4 = LoadColor(In.GroupIndex + 0x24);
        src0 = 0.25 * (src0 + Src2 + Src3 + Src4);

        RWTexture2D<float4> m4 = ResourceDescriptorHeap[MipData.mips[3]];
        m4[In.DispatchThreadID.xy / 8] = PackMipColor(src0);
    }
}