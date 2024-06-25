#include "globals.hlsl"

#define USE_INTERPOLATOR_POSITION 1
#include "interpolators.hlsl"

struct VertexInput
{
	float3 position : POSITION;
};

VertexOutput main(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	uint entityId = instanceID;
	float4x4 worldMatrix = entities.Load<SEntity>(entityId * sizeof(SEntity)).world;
	float4 positionWs = mul(worldMatrix, float4(IN.position, 1.0f));
	OUT.position = mul(globals.sunProj, mul(globals.sunView, positionWs));
	return OUT;
}

