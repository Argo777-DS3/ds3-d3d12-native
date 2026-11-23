// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The Velo rows' occlusion against the scene depth (scene_pass_velocity.cpp): VS and PS, vs_5_0 / ps_5_0.
// I-0136 (the Velo rows' depth precision): the rows, drawn into a depth of their own, keep a pixel unless the scene depth is nearer
// than theirs by more than their rounding. The rows project absolute world positions (|p| about |camera| + the view distance w)
// where the G-buffer projects camera-relative ones: clip z and w each carry a few ulp(|p|) more (I-0134's Fable read: a few hundred
// depth ulps at Firelink's coordinates, against the former constant bias of 64). With depth d = zs (1 - n / w) (0 at near n, 1 at
// far, zs = f / (f - n)), an error e in clip z and w moves d by at most 2 e / w; e = 8 ulp(|camera| + w), the four products'
// rounding twice over (the read measured 1e-5 to 1e-4 at Firelink, |camera| about 650: e = 4.9e-4 there). A hidden row's pixel goes back to the clear (zero velocity, alpha 0: the camera's motion).
cbuffer Occlusion : register(b0) { float4 occlusion; };   // |camera|, near, far / (far - near), 0
Texture2D<float> scene_depth : register(t0);
Texture2D<float> rows_depth : register(t1);
float4 VS(uint id : SV_VertexID) : SV_Position { const float2 t=float2((id<<1)&2,id&2); return float4(t*float2(2,-2)+float2(-1,1),0,1); }
float4 PS(float4 position : SV_Position) : SV_Target {
    const int3 pixel=int3(position.xy,0);
    const float rows=rows_depth.Load(pixel);
    if(rows>=1.0 || occlusion.z<=1.0) discard;
    const float scene=scene_depth.Load(pixel);
    const float w=occlusion.y/max(1.0-scene/occlusion.z,1e-7);
    const float e=8.0*exp2(floor(log2(max(occlusion.x+w,1.0)))-23.0);
    // The rows' clip x and y carry the same rounding: a shift of up to 2 e / w in NDC, e w' / w pixels for a target w' wide, at least half
    // a pixel; on a grazing surface it moves the depth by the surface's step a pixel times that shift (the first final review's L1, the
    // former slope bias's part). The step is the smaller one-sided difference a direction (the second final review's M1: across a
    // silhouette the near side's own step, not the jump to what lies behind).
    uint width,height;scene_depth.GetDimensions(width,height);
    const int2 last=int2(width,height)-1;
    const float left=scene_depth.Load(int3(max(pixel.x-1,0),pixel.y,0)),right=scene_depth.Load(int3(min(pixel.x+1,last.x),pixel.y,0));
    const float up=scene_depth.Load(int3(pixel.x,max(pixel.y-1,0),0)),down=scene_depth.Load(int3(pixel.x,min(pixel.y+1,last.y),0));
    // At the image's border the one difference that exists (its step review's L1); the shift is 2 e / w in NDC as the depth's
    // (x and w both round: its step review's L2), e / w x the larger extent in pixels.
    const float step_x=pixel.x==0?abs(right-scene):pixel.x==last.x?abs(scene-left):min(abs(right-scene),abs(scene-left));
    const float step_y=pixel.y==0?abs(down-scene):pixel.y==last.y?abs(scene-up):min(abs(down-scene),abs(scene-up));
    const float shift=max(0.5,e/w*float(max(width,height)));
    if(rows<=scene+2.0*e/w+(step_x+step_y)*shift) discard;
    return float4(0.5,0.5,1.0,0.0);
}
