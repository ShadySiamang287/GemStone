#include "Graphics/DXR/AccelerationStructs.h"

using namespace GEM;

void TLAS::AddInstance(BLAS& targetBlas, UINT instanceID, UINT hitContribution, UINT mask, glm::mat4x3 transform) {
    if (mInstanceDescs.size() >= mCapacity) return;

    D3D12_RAYTRACING_INSTANCE_DESC desc = {};

    glm::mat3x4 t = glm::transpose(transform);
    memcpy(desc.Transform, &t, sizeof(desc.Transform));

    desc.InstanceID = instanceID;
    desc.InstanceMask = mask;
    desc.InstanceContributionToHitGroupIndex = hitContribution;
    desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    desc.AccelerationStructure = targetBlas.GetResource()->GetGPUVirtualAddress();

    mInstanceDescs.push_back(desc);
    mBLASes.push_back(&targetBlas);
}

void TLAS::Clear() {
    mInstanceDescs.clear();
    mBLASes.clear();
}

TLAS::~TLAS() {
    if (mInstanceMapped) {
        mInstanceAllocation->GetResource()->Unmap(0, nullptr);
    }
}