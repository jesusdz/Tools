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
	float4 positionWs : POSITION0;
	nointerpolation uint entityId : COLOR0;
};

typedef Interpolators VertexOutput;
typedef Interpolators PixelInput;

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	uint entityId = instanceID;
	float4x4 worldMatrix = entities.Load<SEntity>(entityId * sizeof(SEntity)).world;
	OUT.positionWs = mul(worldMatrix, float4(IN.position, 1.0f));
	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, OUT.positionWs));
	OUT.entityId = entityId;
	return OUT;
}

uint PSMain(PixelInput IN) : SV_Target
{
	return IN.entityId;
}

