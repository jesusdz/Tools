#include "structs.hlsl"
#include "defines.hlsl"

// register( name<binding>, space<descriptor set> )
#define REGISTER(reg, set, binding) register(reg##binding, space##set)

// Types of registers:
// t – for shader resource views (SRV)
// s – for samplers
// u – for unordered access views (UAV)
// b – for constant buffer views (CBV)
#define REGISTER_B(set, binding) REGISTER(b, set, binding)
#define REGISTER_S(set, binding) REGISTER(s, set, binding)
#define REGISTER_T(set, binding) REGISTER(t, set, binding)
#define REGISTER_U(set, binding) REGISTER(u, set, binding)

// Space 0: Globals
ConstantBuffer<Globals> globals         : REGISTER_B(DESCRIPTOR_SET_GLOBAL, BINDING_GLOBALS);
SamplerState linearSampler              : REGISTER_S(DESCRIPTOR_SET_GLOBAL, BINDING_SAMPLER);
ByteAddressBuffer entities              : REGISTER_T(DESCRIPTOR_SET_GLOBAL, BINDING_ENTITIES);
Texture2D<float4> shadowmap             : REGISTER_T(DESCRIPTOR_SET_GLOBAL, BINDING_SHADOWMAP);
SamplerComparisonState shadowmapSampler : REGISTER_S(DESCRIPTOR_SET_GLOBAL, BINDING_SHADOWMAP_SAMPLER);

// Space 1: Materials
ConstantBuffer<SMaterial> material      : REGISTER_B(DESCRIPTOR_SET_MATERIAL, BINDING_MATERIAL);
Texture2D<float4> albedoTexture         : REGISTER_T(DESCRIPTOR_SET_MATERIAL, BINDING_ALBEDO);

