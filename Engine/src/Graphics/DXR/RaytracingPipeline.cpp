#include "Graphics/DXR/RaytracingPipeline.h"

using namespace GEM;

void ShaderTable::FillDispatchDesc(D3D12_DISPATCH_RAYS_DESC& desc){
    D3D12_GPU_VIRTUAL_ADDRESS base = mAllocation->GetResource()->GetGPUVirtualAddress();

    // Single record: start address + size (no stride)
    desc.RayGenerationShaderRecord.StartAddress = base + mRayGen.offset;
    desc.RayGenerationShaderRecord.SizeInBytes  = mRayGen.size;

    // Tables: start address + total size + stride between records
    desc.MissShaderTable.StartAddress  = base + mMiss.offset;
    desc.MissShaderTable.SizeInBytes   = mMiss.stride * mMiss.count;
    desc.MissShaderTable.StrideInBytes = mMiss.stride;

    desc.HitGroupTable.StartAddress  = base + mHit.offset;
    desc.HitGroupTable.SizeInBytes   = mHit.stride * mHit.count;
    desc.HitGroupTable.StrideInBytes = mHit.stride;
}