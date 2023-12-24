#include "globals.hlsl"
#include "interpolators.hlsl"

struct VertexInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct Constants
{
	float4x4 model;
};

[[vk::push_constant]] Constants constants;

VertexOutput main(VertexInput IN)
{
	VertexOutput OUT;
	OUT.positionWs = mul(constants.model, float4(IN.position, 1.0f));
	OUT.position = mul(globals.proj, mul(globals.view, OUT.positionWs));
	OUT.normalWs = mul( constants.model, float4( IN.normal, 0.0 ) ).xyz;
	OUT.texCoord = IN.texCoord;// * material.uvScale;
	return OUT;
}

