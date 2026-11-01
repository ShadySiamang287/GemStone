#include "common.hlsli"

Texture2D bloomImage : register(t0);
SamplerState LinearSampler : register(s0);

cbuffer Data : register(b0)
{
    bool horizontal;
}

static const float weights[5] = {
    0.227027,
    0.1945946,
    0.1216216,
    0.054054,
    0.016216
};

float4 BloomPS(FullScreenVertOut input) : SV_Target {
    uint width;
    uint height;
    bloomImage.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);

    float3 result = bloomImage.SampleLevel(LinearSampler, input.uv, 0).rgb * weights[0];

    if (horizontal != 0)
    {
        for (int i = 1; i < 5; ++i)
        {
            float2 offset = float2(texelSize.x * i, 0.0);

            result += bloomImage.SampleLevel(LinearSampler,input.uv + offset,0) .rgb * weights[i];
            result += bloomImage.SampleLevel(LinearSampler,input.uv - offset, 0) .rgb * weights[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            float2 offset = float2(0.0, texelSize.y * i);

            result += bloomImage.SampleLevel(LinearSampler,input.uv + offset,0) .rgb * weights[i];
            result += bloomImage.SampleLevel(LinearSampler,input.uv - offset,0) .rgb * weights[i];
        }
    }
    return float4(result, 1.0);
}
    