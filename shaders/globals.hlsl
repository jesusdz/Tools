
struct Globals
{
	float4x4 view;
	float4x4 proj;
};

// register( name<binding>, space<descriptor set> )
ConstantBuffer<Globals> globals : register(b0, space0);
SamplerState texSampler         : register(s1, space0);

