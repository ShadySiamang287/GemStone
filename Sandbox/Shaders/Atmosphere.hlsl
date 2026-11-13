#include "common.hlsli"

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
    int lightCount;
    float3 _padding;
}

cbuffer AtmosphereData : register(b1) {
    float3 atmosphereColour;
    float planetRadius;
    float atmosphereRadius;
    float densityFalloff;
    float strengthMultiplier;
    int steps;
}

Texture2D<float> depthTex : register(t0);


struct VertexOutput {
    float4 position : SV_Position;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

VertexOutput atmosphereVS(VertexInput vertIn) {
    VertexOutput vert;
    float3 scaledPosition = normalize(vertIn.position) * atmosphereRadius;
    vert.position = mul(ProjectionMatrix, mul(ViewMatrix, float4(scaledPosition, 1.0)));
    vert.worldPos = scaledPosition;
    vert.normal = normalize(vertIn.normal);

    return vert;
}

bool RaySphere(float3 origin, float3 direction, float radius, out float t0, out float t1) {
    t0 = 0.0;
    t1 = 0.0;

    float b = dot(origin, direction);
    float c = dot(origin, origin) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0)
        return false;

    float s = sqrt(disc);
    t0 = -b - s;
    t1 = -b + s;
    return true;
}

float4 atmospherePS(VertexOutput vertIn) : SV_Target0{
    float3 rayOrigin = mul(
                                invViewMatrix,
                                float4(0, 0, 0, 1)
    ) .xyz;

    float3 rayDir = normalize(
        vertIn.worldPos - rayOrigin
    );

    float tNear, tFar;
    if (!RaySphere(rayOrigin, rayDir, atmosphereRadius, tNear, tFar)) {
        discard;
    }
    tNear = max(tNear, 0.0);

    // the planet blocks it duh
    float pNear, pFar;
    if (RaySphere(rayOrigin, rayDir, planetRadius, pNear, pFar) && pNear > 0.0) {
        tFar = min(tFar, pNear);
    }

    float depth = depthTex.Load(int3(int2(vertIn.position.xy), 0), 0);
    if (depth < 1.0) // flip this check if you use reversed-Z
    {
        uint w, h;
        depthTex.GetDimensions(w, h);
        float2 uv = vertIn.position.xy / float2(w, h);

        float3 ndc = float3(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth);
        float4 viewPos = mul(invProjectionMatrix, float4(ndc, 1.0));
        viewPos /= viewPos.w;
        float3 surfaceWorld = mul(invViewMatrix, viewPos).xyz;

        tFar = min(tFar, length(surfaceWorld - rayOrigin));
    }

    float pathLength = tFar - tNear;
    if (pathLength <= 0.0) {
        discard;
    }

    float stepSize = pathLength / (float)steps;
    float thickness = atmosphereRadius - planetRadius;

    float opticalDepth = 0.0;

    for (uint i = 0; i < steps; i++)
    {
        float3 p = rayOrigin + rayDir * (tNear + (i + 0.5) * stepSize);

        float altitude = saturate((length(p) - planetRadius) / thickness);
        float density = exp(-altitude * densityFalloff);

        opticalDepth += density * stepSize;
    }

    float alpha = 1.0 - exp(-opticalDepth * strengthMultiplier);
    return float4(atmosphereColour, alpha);
}