#include "common.hlsli"

Texture2D ImageIn : register(t0);
SamplerState LinearSampler : register(s0);

static const float sigma = 10.f;
static const float bSigma = 0.5f;
static const uint mSize = 15;

float normpdf(float x, float sigma)
{
    return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

float normpdf4(float4 v, float sigma)
{
    return 0.39894 * exp(-0.5 * dot(v, v) / (sigma * sigma)) / sigma;
}

float4 BilateralFilter(FullScreenVertOut input) : SV_Target {
    uint width;
    uint height;
    ImageIn.GetDimensions(width, height);

    float2 texelSize = 1.0 / float2(width, height);

    float4 c = ImageIn.SampleLevel(LinearSampler, input.uv, 0);

    const int kSize = (mSize - 1) / 2;
    float kernel[mSize];
    float4 finalValue = float4(0.f, 0.f, 0.f, 0.f);

    float Z = 0.0f;
    for (int k = 0; k <= kSize; ++k) {
        kernel[kSize + k] = kernel[kSize - k] = normpdf(float(k), sigma);
    }

    float4 cc;
    float factor;
    float bZ = 1.f / normpdf(0.f, bSigma);

    for (int i = -kSize; i <= kSize; ++i)
    {
        for (int j = -kSize; j <= kSize; ++j)
        {
            float2 offset = float2(float(i), float(j)) * texelSize;
            float2 uv = (input.uv + offset); 
            cc = ImageIn.SampleLevel(LinearSampler, uv, 0);
            factor = normpdf4(cc - c, bSigma) * bZ * kernel[kSize + j] * kernel[kSize + i];
            Z += factor;
            finalValue += factor * cc;
        }
    }

    return finalValue / Z;
} 