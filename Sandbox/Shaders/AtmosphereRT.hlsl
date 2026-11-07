RaytracingAccelerationStructure accelStructure : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

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

struct Payload {
    float hitT;
};

[shader("raygeneration")]
void rayGen() {

    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = DispatchRaysDimensions().xy;
    float2 ndc = ((float2(launchIndex) + 0.5) / dims) * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float4 clip = float4(ndc, 0.0, 1.0);
    float4 view = mul(invProjectionMatrix, clip);
    float3 viewPos = view.xyz / view.w;
    float3 rayDir = normalize(mul(invViewMatrix, float4(viewPos, 0.0)).xyz);
    float3 rayOrigin = mul(invViewMatrix, float4(0, 0, 0, 1)).xyz;

    float tNear, tFar;
    if (!RaySphere(rayOrigin, rayDir, atmosphereRadius, tNear, tFar)) {
        return;
    }

    tNear = max(tNear, 0.0f);
    if (tFar <= tNear) return;

    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin = tNear;
    ray.TMax = tFar;

    Payload p;
    p.hitT = tFar;
    TraceRay(accelStructure, RAY_FLAG_NONE, 0x01, 0, 1, 0, ray, p);

    tFar = min(tFar, p.hitT);

    float pathLength = tFar - tNear;
    if (pathLength <= 0.0) {
        return;
    }

    float stepSize = pathLength / (float)steps;
    float thickness = atmosphereRadius - planetRadius;

    float opticalDepth = 0.0;

    for (int i = 0; i < steps; i++)
    {
        float3 samplePoint = rayOrigin + rayDir * (tNear + (i + 0.5) * stepSize);

        float altitude = saturate((length(samplePoint) - planetRadius) / thickness);
        float density = exp(-altitude * densityFalloff);

        opticalDepth += density * stepSize;
    }

    float alpha = 1.0 - exp(-opticalDepth * strengthMultiplier);
    float4 col = float4(atmosphereColour, alpha);

    outputTexture[launchIndex.xy] = float4(lerp(outputTexture[launchIndex].rgb, col.rgb, col.a), 1.0);
}

[shader("miss")]
void miss(inout Payload payload) {
    payload.hitT = 1e30f;
}

[shader("closesthit")]
void closestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr) {
    payload.hitT = RayTCurrent();
}