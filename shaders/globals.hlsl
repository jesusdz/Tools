#include "structs.hlsl"
#include "defines.hlsl"

// register( name<binding>, space<descriptor set> )
#define REGISTER(reg, binding, set) register(reg##binding, space##set)

// Types of registers:
// t – for shader resource views (SRV)
// s – for samplers
// u – for unordered access views (UAV)
// b – for constant buffer views (CBV)
#define REGISTER_B(binding, set) REGISTER(b, binding, set)
#define REGISTER_S(binding, set) REGISTER(s, binding, set)
#define REGISTER_T(binding, set) REGISTER(t, binding, set)
#define REGISTER_U(binding, set) REGISTER(u, binding, set)

// Space 0: Globals
ConstantBuffer<Globals> globals    : REGISTER_B(BINDING_GLOBALS,           DESCRIPTOR_SET_GLOBAL);
SamplerState linearSampler         : REGISTER_S(BINDING_SAMPLER,           DESCRIPTOR_SET_GLOBAL);
ByteAddressBuffer entities         : REGISTER_T(BINDING_ENTITIES,          DESCRIPTOR_SET_GLOBAL);
Texture2D<float4> shadowmap        : REGISTER_T(BINDING_SHADOWMAP,         DESCRIPTOR_SET_GLOBAL);
SamplerState shadowmapSampler      : REGISTER_S(BINDING_SHADOWMAP_SAMPLER, DESCRIPTOR_SET_GLOBAL);

// Space 1: Materials
ConstantBuffer<SMaterial> material : REGISTER_B(BINDING_MATERIAL,  DESCRIPTOR_SET_MATERIAL);
Texture2D<float4> albedoTexture    : REGISTER_T(BINDING_ALBEDO,    DESCRIPTOR_SET_MATERIAL);

