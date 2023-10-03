struct VertexInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float3 normalWs : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct Globals
{
	float4x4 view;
	float4x4 proj;
};

struct Constants
{
	float4x4 model;
};

// register( name<binding>, space<descriptor set> )
ConstantBuffer<Globals> globals : register(b0, space0);

[[vk::push_constant]] Constants constants;

VertexOutput main(VertexInput IN)
{
	VertexOutput OUT;
	OUT.position = mul(globals.proj, mul(globals.view, mul(constants.model, float4(IN.position, 1.0f))));
	OUT.normalWs = mul( constants.model, float4( IN.normal, 0.0 ) ).xyz;
	OUT.texCoord = IN.texCoord;
	return OUT;
}

