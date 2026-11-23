// I-0132 R11i (the step review's M2): pipeline_cache_test's DXIL programmes, compiled by dxc as the module's own (ps_6_6,
// -Qstrip_reflect): one writing SV_Depth (its output signature in the container's OSG1 chunk), one writing colour only.
struct Out {float4 colour:SV_Target;float depth:SV_Depth;};
Out depth(float4 position:SV_Position){Out o;o.colour=float4(1,1,1,1);o.depth=position.z*0.5;return o;}
float4 colour(float4 position:SV_Position):SV_Target{return float4(position.xy,0,1);}
