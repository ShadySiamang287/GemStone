#include "Graphics/DXR/AccelerationStructs.h"

using namespace GEM;

static constexpr int rayTypeCount = 1;

void TLAS::AddInstance(BLAS& targetBlas, UINT instanceID, UINT mask, glm::mat4x3 transform) {
    if (mInstanceDescs.size() >= mCapacity) return;
    constexpr UINT kRayTypes = 1;

    D3D12_RAYTRACING_INSTANCE_DESC desc = {};

    glm::mat3x4 t = glm::transpose(transform);
    memcpy(desc.Transform, &t, sizeof(desc.Transform));

    desc.InstanceID = instanceID;
    desc.InstanceMask = mask;
    desc.InstanceContributionToHitGroupIndex = 0;
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