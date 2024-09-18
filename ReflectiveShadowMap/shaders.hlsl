

cbuffer ConstantBufferObject : register(b0) {
  float4x4 g_view;
  float4x4 g_proj;
  float4x4 g_translate;
  float4 g_red;
  float g_timeElapsed;
};

struct Vout {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
};

Vout VS(float3 pos : POSITION, float4 color : COLOR) {
  Vout vout;
  float4x4 viewProj = transpose(mul(g_proj, g_view));
  vout.pos = mul(float4(pos, 1.0f), viewProj);
  vout.color = color;
  return vout;
}

float4 PS(Vout pin) : SV_TARGET {
  float r = sin(g_timeElapsed);
  float g = sin(2 * g_timeElapsed);
  float b = sin(4 * g_timeElapsed);
  float3 v = 0.5 * (float3(r, g, b) + 1);
  return pin.color * float4(v, 1.0f);
}