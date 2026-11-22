struct AtmosphereData {
    float3 position;
    float3 atmosphereColour;
    float planetRadius;
    float atmosphereRadius;
    float densityFalloff;
    float strengthMultiplier;
    int steps;
};

RaytracingAccelerationStructure accelStructure : register(t0);
StructuredBuffer<AtmosphereData> atmosphereBuffer : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
    float3 ambientLight;
    float ambientStrenght;
    int lightCount;
    float3 padding;
}

#define MAX_ATMOS 5

struct Payload {
    float hitT;
    uint instanceID;

    float atmosphereTNear[MAX_ATMOS];
    float atmosphereTFar[MAX_ATMOS];
    uint seenMask;
};

float MarchOpticalDepth(AtmosphereData d, float3 origin, float3 dir, float tNear, float tFar) {
    float pathLength = tFar - tNear;
    if (pathLength <= 0.0) {
        return 0.0;
    }

    float stepSize = pathLength / (float)d.steps;
    float thickness = d.atmosphereRadius - d.planetRadius;

    float opticalDepth = 0.0;

    for (int i = 0; i < d.steps; i++)
    {
        float3 samplePoint = origin + dir * (tNear + (i + 0.5) * stepSize);

        float distFromCenter = length(samplePoint - d.position);
        float altitude = saturate((distFromCenter - d.planetRadius) / thickness);
        float density = exp(-altitude * d.densityFalloff);

        opticalDepth += density * stepSize;
    }

    return opticalDepth;
}

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

    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin = 0.0f;
    ray.TMax = 1e30;

    Payload p;
    p.hitT = -1.0f;
    p.instanceID = 0;
    p.seenMask = 0;

    AtmosphereData atmospheres[MAX_ATMOS];

    [unroll]
    for (uint k = 0; k < MAX_ATMOS; k++) {
        atmospheres[k] = atmosphereBuffer[k];
        p.atmosphereTNear[k] = -1.0f;
        p.atmosphereTFar[k] = 1e30f;
    }

    TraceRay(accelStructure, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, p);
    float sceneT = (p.hitT >= 0.0) ? p.hitT : 1e30;

    float spanT0[MAX_ATMOS];
    float spanT1[MAX_ATMOS];
    uint spanIdx[MAX_ATMOS];
    uint n = 0;

    

    for (uint i = 0; i < MAX_ATMOS; i++) {
        if ((p.seenMask & (1u << i)) == 0) continue;

        AtmosphereData d = atmosphereBuffer[i];
        bool inside = length(rayOrigin - d.position) < d.atmosphereRadius;

        float t0 = inside ? 0.0 : p.atmosphereTNear[i];
        if (!inside && t0 < 0.0) continue;

        float t1 = min(p.atmosphereTFar[i], sceneT);
        if (t1 <= t0) continue;

        spanT0[n] = t0;
        spanT1[n] = t1;
        spanIdx[n] = i;
        n++;
    }

    /// gpu insertion sort which is evil.
    for (uint a = 1; a < n; a++) {
        float k0 = spanT0[a], k1 = spanT1[a]; uint ki = spanIdx[a];
        int b = (int)a - 1;
        while (b >= 0 && spanT0[b] < k0) {
            spanT0[b + 1] = spanT0[b]; spanT1[b + 1] = spanT1[b]; spanIdx[b + 1] = spanIdx[b];
            b--;
        }
        spanT0[b + 1] = k0; spanT1[b + 1] = k1; spanIdx[b + 1] = ki;
    }

    float3 col = outputTexture[launchIndex.xy].rgb;
    for (uint s = 0; s < n; s++) {
        AtmosphereData d = atmospheres[spanIdx[s]];
        float opticalDepth = MarchOpticalDepth(d, rayOrigin, rayDir, spanT0[s], spanT1[s]);
        float alpha = 1.0 - exp(-opticalDepth * d.strengthMultiplier);
        col = lerp(col, d.atmosphereColour, alpha);
    }

    outputTexture[launchIndex.xy] = float4(col, 1.0);
}

[shader("miss")]
void miss(inout Payload payload) {
    payload.hitT = -1.0f;
}

[shader("closesthit")]
void OpaqueClosestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr) {
    payload.hitT = RayTCurrent();
    payload.instanceID = InstanceID();
}

[shader("anyhit")]
void AtmosphereAnyHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr) {
    float tCurrent = RayTCurrent();
    uint i = InstanceID() - 4;
    
    payload.seenMask |= (1u << i);
    if (HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE) {
        payload.atmosphereTNear[i]   = tCurrent;
    } else {
        payload.atmosphereTFar[i] = tCurrent;
    }

    IgnoreHit();
}