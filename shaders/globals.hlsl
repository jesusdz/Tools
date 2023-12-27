#include "structs.hlsl"

// register( name<binding>, space<descriptor set> )

// Space 0: Globals
ConstantBuffer<Globals> globals    : register(b0, space0);
SamplerState linearSampler         : register(s1, space0);
ByteAddressBuffer entities         : register(t2, space0);

// Space 1: Materials
ConstantBuffer<SMaterial> material : register(b0, space1);
Texture2D<float4> albedoTexture    : register(t1, space1);

