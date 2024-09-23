
cbuffer PassConstant : register(b0) {
  float4x4 g_view;
  float4x4 g_proj;
  float g_timeElapsed;
};

cbuffer ModelConstant : register(b1) {
  float4x4 g_model;
  float3 g_albedo;
};

struct Vin {
  float3 pos : POSITION;
  float3 normal : NORMAL;
  float4 color : COLOR;
};

struct Vout {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
};

Vout VS(Vin vin) {
  Vout vout;
  float4x4 mvp = mul(mul(g_proj, g_view), g_model);
  vout.pos = mul(mvp, float4(vin.pos, 1.0f));
  vout.color = vin.color;
  vout.normal = vin.normal;
  return vout;
}

float4 PS(Vout pin) : SV_TARGET {
  float3 l = normalize(float3(1.f, 1.f, -1.f));
  float3 n = normalize(pin.normal);
  float3 c = dot(l, n) * g_albedo;
  return float4(c, 1.f);
  // return float4(1.f, 1.f, 1.f, 1.f);
}
