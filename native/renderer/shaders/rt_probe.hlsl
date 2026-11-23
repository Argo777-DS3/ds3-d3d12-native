// I-0117 (R2 T1): rays through a scene's top level, one a thread, for the unit test (rt_scene_test.cpp): each ray is
// {origin xyz, tmax} {direction xyz, mask} in a raw buffer; each answer {status, t, InstanceID, GeometryIndex} into another.
// T3: with a material table (its slot, else ~0: every candidate opaque) and the rows' (I-0119 R3a) the non-opaque candidates take
// rt_material.hlsli's test.
// Compiled by dxc at build time (Build-GameBootstrap.ps1), reached through the bindless heap as the start self-test.
#define ProbeRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), RootConstants(num32BitConstants=6, b0), "     "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU=TEXTURE_ADDRESS_WRAP, addressV=TEXTURE_ADDRESS_WRAP, addressW=TEXTURE_ADDRESS_WRAP)"
#include "rt_material.hlsli"
struct Indices {uint scene;uint rays;uint answers;uint count;uint table;uint rows;};
ConstantBuffer<Indices> indices:register(b0);
SamplerState wrap:register(s0);
[RootSignature(ProbeRoot)]
[numthreads(64,1,1)]
void main(uint id:SV_DispatchThreadID){
    if(id>=indices.count)return;
    RaytracingAccelerationStructure scene=ResourceDescriptorHeap[indices.scene];
    ByteAddressBuffer rays=ResourceDescriptorHeap[indices.rays];
    RWByteAddressBuffer answers=ResourceDescriptorHeap[indices.answers];
    const float4 a=asfloat(rays.Load4(32*id)),b=asfloat(rays.Load4(32*id+16));
    RayDesc ray;ray.Origin=a.xyz;ray.TMin=0;ray.Direction=b.xyz;ray.TMax=a.w;
    RayQuery<RAY_FLAG_NONE> query;query.TraceRayInline(scene,indices.table==RT_NONE?RAY_FLAG_FORCE_OPAQUE:RAY_FLAG_NONE,asuint(b.w)&0xFF,ray);
    while(query.Proceed()){
        if(query.CandidateType()!=CANDIDATE_NON_OPAQUE_TRIANGLE)continue;
        StructuredBuffer<RtMaterialRecord> table=ResourceDescriptorHeap[indices.table];ByteAddressBuffer rows=ResourceDescriptorHeap[indices.rows];
        const RtMaterialRecord record=table[query.CandidateInstanceContributionToHitGroupIndex()+query.CandidateGeometryIndex()];
        if(RtAnswers(record,asuint(b.w)&0xFF) && RtKeeps(record,RtRowOf(rows,query.CandidateInstanceIndex()).scroll,query.CandidatePrimitiveIndex(),
            query.CandidateTriangleBarycentrics(),wrap))query.CommitNonOpaqueTriangleHit();
    }
    const bool hit=query.CommittedStatus()==COMMITTED_TRIANGLE_HIT;
    answers.Store4(16*id,uint4(query.CommittedStatus(),asuint(hit?query.CommittedRayT():-1.0f),hit?query.CommittedInstanceID():~0u,
        hit?query.CommittedGeometryIndex():~0u));
    // I-0119 R3a: with a table, the hit's material after every ray's answer: {albedo, gloss} {shading normal, F0's red} {geometric
    // normal, its material flag}, 48 bytes a ray (the top mip: no cone).
    if(indices.table==RT_NONE)return;
    RtSurface s=(RtSurface)0;
    if(hit){StructuredBuffer<RtMaterialRecord> table=ResourceDescriptorHeap[indices.table];ByteAddressBuffer rows=ResourceDescriptorHeap[indices.rows];
        const RtInstanceRow row=RtRowOf(rows,query.CommittedInstanceIndex());
        if(query.CommittedGeometryIndex()<row.count)   // an instance without a set reads no record (step review L2)
        s=RtMaterialAt(table,table[query.CommittedInstanceContributionToHitGroupIndex()+query.CommittedGeometryIndex()],row,rows,query.CommittedGeometryIndex(),
            query.CommittedObjectToWorld3x4(),query.CommittedPrimitiveIndex(),query.CommittedTriangleBarycentrics(),ray.Direction,0,1,0,wrap);}
    const uint at=16*indices.count+48*id;
    answers.Store4(at,asuint(float4(s.albedo,s.gloss)));answers.Store4(at+16,asuint(float4(s.normal,s.f0.r)));
    answers.Store4(at+32,asuint(float4(s.geometric,s.material?1:0)));
}
