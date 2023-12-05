#include "structs.hlsl"

// register( name<binding>, space<descriptor set> )
ConstantBuffer<Globals> globals : register(b0, space0);
SamplerState texSampler         : register(s1, space0);

