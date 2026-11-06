#include "Graphics/DXR/AccelerationStructs.h"

using namespace GEM;

static constexpr int rayTypeCount = 1;

void TLAS::AddInstance(BLAS& targetBlas, UINT instanceID, UINT mask, glm::mat4x3 transform) {
    if (mInstanceDescs.size() >= mCapacity) return;
    constexpr UINT kRayTypes = 1; // change if you have e.g. primary + shadow hit groups

    D3D12_RAYTRACING_INSTANCE_DESC desc = {};

    // glm is column-major (4 columns of 3), D3D wants 3 rows of 4.
    // Transposing gives a mat3x4 whose memory is exactly that layout.
    glm::mat3x4 t = glm::transpose(transform);
    memcpy(desc.Transform, &t, sizeof(desc.Transform)); // 12 floats

    desc.InstanceID = instanceID;                       // 24 bits
    desc.InstanceMask = mask;                           // 8 bits
    desc.InstanceContributionToHitGroupIndex = mNextHitGroupOffset;
    desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    desc.AccelerationStructure = targetBlas.GetResource()->GetGPUVirtualAddress();

    mNextHitGroupOffset += static_cast<UINT>(targetBlas.mGeometries.size()) * kRayTypes;

    mInstanceDescs.push_back(desc);
    mBLASes.push_back(&targetBlas);
}

void TLAS::Clear() {
    mInstanceDescs.clear();
    mBLASes.clear();
    mNextHitGroupOffset = 0;
}

TLAS::~TLAS() {
    if (mInstanceMapped) {
        mInstanceAllocation->GetResource()->Unmap(0, nullptr);
    }
}