// I-0132 (the root aliases): pipeline_cache_test's two DXIL programmes embedding one root signature (as two builds' versions of one
// of the module's programmes): two blobs, one RTS0 chunk.
#define Root "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"
[RootSignature(Root)]
float4 red(float4 position:SV_Position):SV_Target{return float4(1,0,0,1);}
[RootSignature(Root)]
float4 green(float4 position:SV_Position):SV_Target{return float4(position.x*0.001,1,0,1);}
