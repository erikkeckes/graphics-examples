// Shader is taken from:
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

struct BloomUpSample
{
    uint src;
    uint dest;
    uint smp;
    float strength;
};

ConstantBuffer<BloomUpSample> UpSample : register(b0);

cbuffer Composite : register(b1)
{
    uint composite;
}


[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID)
{
    RWTexture2D<float4> dst = ResourceDescriptorHeap[UpSample.dest];
    Texture2D<float4> src = ResourceDescriptorHeap[UpSample.src];
    SamplerState smp = ResourceDescriptorHeap[UpSample.smp];
    
    uint width, height;
    dst.GetDimensions(width, height);
    
    float2 texelSize = 1.0 / float2(width, height);
    float2 texCoord = float2(dTid.xy + 0.5.xx) * texelSize;
    
    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    float x = 0.0005;
    float y = 0.0005;
    
    float3 a = src.SampleLevel(smp, float2(texCoord.x - 2 * x, texCoord.y + 2 * y), 0).rgb;
    float3 b = src.SampleLevel(smp, float2(texCoord.x, texCoord.y + 2 * y), 0).rgb;
    float3 c = src.SampleLevel(smp, float2(texCoord.x + 2 * x, texCoord.y + 2 * y), 0).rgb;
    float3 d = src.SampleLevel(smp, float2(texCoord.x - 2 * x, texCoord.y), 0).rgb;
    float3 e = src.SampleLevel(smp, float2(texCoord.x, texCoord.y), 0).rgb;
    float3 f = src.SampleLevel(smp, float2(texCoord.x + 2 * x, texCoord.y), 0).rgb;
    float3 g = src.SampleLevel(smp, float2(texCoord.x - 2 * x, texCoord.y - 2 * y), 0).rgb;
    float3 h = src.SampleLevel(smp, float2(texCoord.x, texCoord.y - 2 * y), 0).rgb;
    float3 i = src.SampleLevel(smp, float2(texCoord.x + 2 * x, texCoord.y - 2 * y), 0).rgb;
    float3 j = src.SampleLevel(smp, float2(texCoord.x - x, texCoord.y + y), 0).rgb;
    float3 k = src.SampleLevel(smp, float2(texCoord.x + x, texCoord.y + y), 0).rgb;
    float3 l = src.SampleLevel(smp, float2(texCoord.x - x, texCoord.y - y), 0).rgb;
    float3 m = src.SampleLevel(smp, float2(texCoord.x + x, texCoord.y - y), 0).rgb;

    float3 upsample = e * 0.125;
    upsample += (a + c + g + i) * 0.03125;
    upsample += (b + d + f + h) * 0.0625;
    upsample += (j + k + l + m) * 0.125;
    dst[dTid.xy] += float4(upsample.rgb, 1.0);
    
    if (UpSample.strength > 0)
    {
        RWTexture2D<float4> comp = ResourceDescriptorHeap[composite];
        comp[dTid.xy] = float4(comp[dTid.xy].rgb + dst[dTid.xy].rgb * UpSample.strength, 1.0f);
    }
}