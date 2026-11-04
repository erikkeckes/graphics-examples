// Shader is taken from:
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

struct BloomDownSample
{
    uint src;
    uint dest;
    uint smp;
    float treshold;
};

ConstantBuffer<BloomDownSample> DownSample : register(b0);

// Function taken from.
// https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
float3 PreFilter(float3 c, float treshold)
{
    float brightness = max(c.r, max(c.g, c.b));
    float contribution = max(0, brightness - treshold);
    contribution /= max(brightness, 0.00001);
    return c * contribution;
}

[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID)
{
    RWTexture2D<float4> dst = ResourceDescriptorHeap[DownSample.dest];
    Texture2D<float4> src = ResourceDescriptorHeap[DownSample.src];
    SamplerState smp = ResourceDescriptorHeap[DownSample.smp];
    
    uint width, height;
    dst.GetDimensions(width, height);
    
    float x = 1 / float(width);
    float y = 1 / float(height);
    
    float2 texCoord = float2(dTid.xy + 0.5.xx) / float2(width, height);

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
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

        // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    float3 outColor = e * 0.125;
    outColor += (a + c + g + i) * 0.03125;
    outColor += (b + d + f + h) * 0.0625;
    outColor += (j + k + l + m) * 0.125;
    outColor = max(outColor, 0.00001.rrr);
    outColor = PreFilter(outColor, DownSample.treshold);
    dst[dTid.xy] = float4(outColor.rgb, 1.0);
}