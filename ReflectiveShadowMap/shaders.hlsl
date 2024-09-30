
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
  float3 worldPos : POS;
  float4 pos : SV_Position;
};

VOutLight VSLight(Vin vin) {
  VOutLight vout;

  float4x4 mv = mul(g_lightView, g_model);
  float4x4 mvp = mul(g_lightOrtho, mv);

  vout.pos = mul(mvp, float4(vin.pos, 1.f));

  vout.worldNormal = normalize(mul(transpose(g_invModel), float4(normalize(vin.normal), 0.f)).xyz);


  float lightDepth = mul(mv, float4(vin.pos, 1.f)).z;
  vout.normalizedLinearDepth = (lightDepth - g_lightZNear) / (g_lightZFar - g_lightZNear);

  vout.worldPos = mul(g_model, float4(vin.pos, 1.f)).xyz;

  return vout;
}

struct PinLight {
  float3 worldNormal : NORMAL;
  float linearDepth : DEPTH;
  float3 worldPos : POS;
};

// clang-format off
void PSLight(PinLight pin, out float4 depth : SV_Target0, 
	                   out float4 normal : SV_Target1,
	                   out float4 flux : SV_Target2,
                           out float4 worldPos : SV_Target3) {
  // clang-format on

  // Depth texture
  float3 l = normalize(g_lightPos - pin.worldPos);
  float slopeFactor = 1.f - dot(pin.worldNormal, l);
  float d = pin.linearDepth + 0.05f * slopeFactor;
  depth = float4(d, d, d, 1.f);

  // Normal texture
  normal = float4(pin.worldNormal, 1.f);

  // Flux texture
  flux = float4(g_albedo * g_lightFlux, 1.f);

  worldPos = float4(pin.worldPos, 1.f);
}

// ===========
// Second pass
// ===========

struct VOut {
  float2 rsmUV : TEXCOORD;              // UV coordinates when projected to shadow map
  float3 worldNormal : NORMAL;          // World normal
  float normalizedLinearDepth : DEPTH;  // Depth to light
  float3 shadingPoint : POSITION;       // World position of shading point
  float4 pos : SV_Position;
};

VOut VS(Vin vin) {
  VOut vout;

  float4x4 mvp = mul(g_proj, mul(g_view, g_model));
  float4 pWorld = float4(vin.pos, 1.f);
  float4 pNdc = mul(mvp, pWorld);

  // No need for perspective division. Hardware will do that for us later.
  // If using perspective projection:
  // Use depth in view space (before perspective projection thus linear).
  // Depth values after perspective projection are pushed to the far plane.
  // And smaller zNear results in more quickly growth of projected z to 1.0f.
  // That means majority of z values before projection will become really close to 1.0f after
  // projection.
  // See [Luna 2016] Introduction to 3D Game Programming with DirectX 12. (5.6.3.5 Normalized
  // Depth Value)
  vout.pos = pNdc;

  // Normal transformation: transpose(inverse(T))
  vout.worldNormal = normalize(mul(transpose(g_invModel), float4(vin.normal, 0.f)).xyz);

  float4x4 mvLight = mul(g_lightView, g_model);
  float4x4 mvpLight = mul(g_lightOrtho, mvLight);

  float lightDepth = mul(mvLight, pWorld).z;
  vout.normalizedLinearDepth = (lightDepth - g_lightZNear) / (g_lightZFar - g_lightZNear);

  float4 worldPos = mul(g_model, float4(vin.pos, 1.f)).xyzw;
  vout.shadingPoint = worldPos.xyz;

  float4 pRsm = mul(mvpLight, pWorld);

  float u = (pRsm.x / pRsm.w + 1.f) * 0.5f;
  float v = 1.f - (pRsm.y / pRsm.w + 1.f) * 0.5f;
  vout.rsmUV = float2(u, v);

  return vout;
}

struct PSOut {
  float4 finalColor : SV_Target;
};

// clang-format off
PSOut PS(float2 rsmUV : TEXCOORD,               //
          float3 worldNormal : NORMAL,          //
          float normalizedLinearDepth : DEPTH,  //
          float3 shadingPoint : POSITION) {
  // clang-format on

  float3 l = normalize(-g_lightDirection);
  float3 n = normalize(worldNormal);

  // Direct lighting

  float3 direct = g_albedo * g_lightFlux * saturate(dot(l, n));

  float rsmPixelSize = 1.f / g_rsmSize;
  float2 duv0 = {0.f, 0.f};
  float2 duv1 = {rsmPixelSize, 0.f};
  float2 duv2 = {0.f, rsmPixelSize};
  float2 duv3 = {rsmPixelSize, rsmPixelSize};

  float depthInView = normalizedLinearDepth * (g_lightZFar - g_lightZNear);
  float depthInRSM0 = g_depthMap.Sample(g_samp, rsmUV + duv0) * (g_lightZFar - g_lightZNear);
  float depthInRSM1 = g_depthMap.Sample(g_samp, rsmUV + duv1) * (g_lightZFar - g_lightZNear);
  float depthInRSM2 = g_depthMap.Sample(g_samp, rsmUV + duv2) * (g_lightZFar - g_lightZNear);
  float depthInRSM3 = g_depthMap.Sample(g_samp, rsmUV + duv3) * (g_lightZFar - g_lightZNear);
  float comp0 = depthInRSM0 < depthInView;
  float comp1 = depthInRSM1 < depthInView;
  float comp2 = depthInRSM2 < depthInView;
  float comp3 = depthInRSM3 < depthInView;
  float shadowFactor = (comp0 + comp1 + comp2 + comp3) / 4.f;

  direct *= 1.f - shadowFactor;

  // Indirect lighting

  float3 indirect = {0.f, 0.f, 0.f};

  int px = floor(rsmUV.x * g_rsmSize);
  int py = floor(rsmUV.y * g_rsmSize);

  int neighborCount = 30;
  for (int di = -neighborCount; di < neighborCount; ++di) {
    for (int dj = -neighborCount; dj < neighborCount; ++dj) {
      int qx = px + di;
      int qy = py + dj;

      float qu = (qx + 0.5f) / g_rsmSize;
      float qv = (qy + 0.5f) / g_rsmSize;

      float3 lightNormal = g_normalMap.Sample(g_samp, float2(qu, qv));
      float3 indirectLightWorldPos = g_posMap.Sample(g_samp, float2(qu, qv)).xyz;

      float dist = max(distance(shadingPoint, indirectLightWorldPos), 0.1f);
      float3 dirOut = shadingPoint - indirectLightWorldPos;
      float3 dirIn = -dirOut;
      float cosLight = max(0.f, dot(lightNormal, dirOut));
      float cosShadingPoint = max(0.f, dot(n, dirIn));
      float3 lightFlux = g_fluxMap.Sample(g_samp, float2(qu, qv));

      indirect += lightFlux * ((cosLight * cosShadingPoint) / pow(dist, 4.f));
    }
  }

  int sampleCount = (2 * neighborCount + 1) * (2 * neighborCount + 1);

  float3 finalColor = float3(0.f, 0.f, 0.f);
  finalColor += direct;
  finalColor += indirect / sampleCount;

  PSOut res;
  res.finalColor = float4(finalColor, 1.f);

  return res;
}

float4 PSTest(PinLight pin) : SV_Target {
  return float4(pin.worldNormal, 1.f);
  // return float4(pin.screenUV, 0.f, 1.f);
}