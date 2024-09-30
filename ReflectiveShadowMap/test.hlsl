
cbuffer PassConstant : register(b0) {
  float4x4 g_view;
  float4x4 g_invView;
  float4x4 g_proj;
  float4x4 g_invProj;
  float4x4 g_lightView;
  float4x4 g_invLightView;
  float4x4 g_lightOrtho;
  float4x4 g_invLightOrtho;
  float3 g_lightFlux;
  float g_lightZNear;
  float3 g_lightDirection;
  float g_lightZFar;
  float3 g_lightPos;
  float g_width;
  float g_height;
  float g_rsmSize;
  float g_timeElapsed;
};

cbuffer ModelConstant : register(b1) {
  float4x4 g_model;
  float4x4 g_invModel;
  float3 g_albedo;
};

SamplerState g_samp : register(s0);


Texture2D g_depthMap : register(t0);
Texture2D g_normalMap : register(t1);
Texture2D g_fluxMap : register(t2);
Texture2D g_posMap : register(t3);

struct Vin {
  float3 pos : POSITION;
  float3 normal : NORMAL;
};

// ==========
// First pass
// ==========

struct VOutLight {
  float3 worldNormal : NORMAL;
  float normalizedLinearDepth : DEPTH;
  float3 worldPos : POSITION;
  float4 pos : SV_Position;
};

struct PinLight {
  float3 worldNormal : NORMAL;
  float linearDepth : DEPTH;
  float3 worldPos : POSITION;
};

// clang-format off
void main(PinLight pin, out float4 depth : SV_Target0, 
	                   out float4 normal : SV_Target1,
	                   out float4 flux : SV_Target2,
                           out float4 worldPos : SV_Target3) {
  // clang-format on

  // Depth texture
  float d = pin.linearDepth;
  depth = float4(d, d, d, 1.f);

  // Normal texture
  normal = float4(pin.worldNormal, 1.f);

  // Flux texture
  flux = float4(g_albedo * g_lightFlux, 1.f);

  worldPos = float4(pin.worldPos, 1.f);
}