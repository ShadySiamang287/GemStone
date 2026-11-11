
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
    int lightCount;
    float3 _padding;
}


// cbuffer AtmosphereData : register(b1) {
//     float3 atmosphereColour;
//     float planetRadius;
//     float atmosphereRadius;
//     float densityFalloff;
//     float strengthMultiplier;
//     int steps;
// }

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
    uint instanceID;
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

    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin = 0.0f;
    ray.TMax = 1e30;

    Payload p;
    p.hitT = -1.0f;

    TraceRay(accelStructure, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0x02, 0, 1, 0, ray, p);
    if (p.hitT < 0.0) {
        return;
    }
    tFar = p.hitT;
    p.hitT = -1.0f;
    TraceRay(accelStructure, RAY_FLAG_CULL_FRONT_FACING_TRIANGLES, 0x02, 0, 1, 0, ray, p);
    tNear = (p.hitT >= 0.0) ? p.hitT : 0.0;
    
    uint instanceID = p.instanceID;

    ray.TMin = tNear;
    ray.TMax = tFar;
    p.hitT = -1.0f;
    TraceRay(accelStructure, RAY_FLAG_NONE, 0x01, 0, 1, 0, ray, p);
    if (p.hitT >= 0.0) tFar = min(tFar, p.hitT);
    
    AtmosphereData atmosphere = atmosphereBuffer[instanceID - 4];

    float pathLength = tFar - tNear;
    if (pathLength <= 0.0) {
        return;
    }

    float stepSize = pathLength / (float)atmosphere.steps;
    float thickness = atmosphere.atmosphereRadius - atmosphere.planetRadius;

    float opticalDepth = 0.0;

    for (int i = 0; i < atmosphere.steps; i++)
    {
        float3 samplePoint = rayOrigin + rayDir * (tNear + (i + 0.5) * stepSize);

        float distFromCenter = length(samplePoint - atmosphere.position);
        float altitude = saturate((distFromCenter - atmosphere.planetRadius) / thickness);
        float density = exp(-altitude * atmosphere.densityFalloff);

        opticalDepth += density * stepSize;
    }

    float alpha = 1.0 - exp(-opticalDepth * atmosphere.strengthMultiplier);
    float4 col = float4(atmosphere.atmosphereColour, alpha);

    outputTexture[launchIndex.xy] = float4(lerp(outputTexture[launchIndex].rgb, col.rgb, col.a), 1.0);
}

[shader("miss")]
void miss(inout Payload payload) {
    payload.hitT = -1.0f;
}

[shader("closesthit")]
void closestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr) {
    payload.hitT = RayTCurrent();
    payload.instanceID = InstanceID();
}