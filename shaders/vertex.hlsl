#include "globals.hlsl"

#define USE_INTERPOLATOR_POSITION_WS 1
#define USE_INTERPOLATOR_NORMAL_WS 1
#define USE_INTERPOLATOR_TEXCOORD 1
#define USE_INTERPOLATOR_SHADOWMAP_COORD 1
#include "interpolators.hlsl"

struct VertexInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

//struct Constants
//{
//	float4x4 model;
//};

VertexOutput main(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	uint entityId = instanceID;
	float4x4 worldMatrix = entities.Load<SEntity>(entityId * sizeof(SEntity)).world;
	OUT.positionWs = mul(worldMatrix, float4(IN.position, 1.0f));
	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, OUT.positionWs));
	OUT.normalWs = mul( worldMatrix, float4( IN.normal, 0.0 ) ).xyz;
	OUT.texCoord = IN.texCoord * material.uvScale;
	OUT.shadowmapCoord = mul(globals.sunProj, mul(globals.sunView, OUT.positionWs));
	return OUT;
}

