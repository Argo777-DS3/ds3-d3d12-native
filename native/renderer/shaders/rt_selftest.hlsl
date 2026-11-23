// I-0109 (R0): one inline ray against one triangle, its acceleration structure and its result reached through the bindless
// heap (SM 6.6 ResourceDescriptorHeap). Compiled by dxc at build time (Build-GameBootstrap.ps1), run once at start.
#define SelfTestRoot "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), RootConstants(num32BitConstants=2, b0)"
struct Indices {uint scene;uint result;};
ConstantBuffer<Indices> indices:register(b0);
[RootSignature(SelfTestRoot)]
[numthreads(1,1,1)]
void main(){
    RaytracingAccelerationStructure scene=ResourceDescriptorHeap[indices.scene];
    RWByteAddressBuffer result=ResourceDescriptorHeap[indices.result];
    // The triangle (0,0,0) (1,0,0) (0,1,0) lies in z=0: a ray from z=-1 along +z through (0.25,0.25) hits it at t=1.
    RayDesc ray;ray.Origin=float3(0.25,0.25,-1);ray.Direction=float3(0,0,1);ray.TMin=0;ray.TMax=10;
    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;query.TraceRayInline(scene,RAY_FLAG_NONE,0xFF,ray);
    while(query.Proceed()){}
    const uint status=query.CommittedStatus();
    result.Store2(0,uint2(status,asuint(status==COMMITTED_TRIANGLE_HIT?query.CommittedRayT():-1.0f)));
}
