#include "defines.hlsl"
#include "globals.hlsl"

struct VertexInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct Interpolators
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

typedef Interpolators VertexOutput;
typedef Interpolators PixelInput;

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, float4(IN.position, 1.0)));
	OUT.texCoord = IN.texCoord;
	return OUT;
}

float4 PSMain(PixelInput IN) : SV_Target
{
	float4 albedo = albedoTexture.Sample(pointSampler, IN.texCoord);
	if (albedo.a == 0.0)
		discard;

	return albedo;
}

