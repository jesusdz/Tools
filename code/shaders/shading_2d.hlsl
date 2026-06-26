#include "defines.hlsl"
#include "globals.hlsl"

Texture2D<float4> spriteTexture : REGISTER_T(2, 0);

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
#if USE_ENTITY_SELECTION
	nointerpolation bool isSelected : POSITION2;
#endif
};

typedef Interpolators VertexOutput;
typedef Interpolators PixelInput;

uint EntityId(uint entityHandle)
{
	return entityHandle>>16;
}

VertexOutput VSMain(VertexInput IN, uint entityHandle : SV_InstanceID)
{
	VertexOutput OUT;
	uint entityIndex = EntityId(entityHandle);
	SEntity entityData = entities.Load<SEntity>(entityIndex * sizeof(SEntity));
	SSpriteData sprite = spriteData.Load<SSpriteData>(entityData.spriteIndex * sizeof(SSpriteData));
	float3 posOs = float3(IN.position.xy * sprite.worldSize, IN.position.z);
	float4 posWs = mul(entityData.world, float4(posOs, 1.0));
	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, posWs));
	OUT.texCoord = sprite.uvOffset + IN.texCoord * sprite.uvSize;
#if USE_ENTITY_SELECTION
	OUT.isSelected = entityHandle == globals.selectedEntity;
#endif
	return OUT;
}

float4 PSMain(PixelInput IN) : SV_Target
{
	float4 albedo = spriteTexture.Sample(pointSampler, IN.texCoord);
	if (albedo.a == 0.0)
		discard;

#if USE_ENTITY_SELECTION
	if (IN.isSelected)
	{
		float3 orange = float3(1.0, 0.7, 0.0);
		albedo.rgb = lerp(albedo.rgb, orange, sin(globals.time * 2.0) * 0.1f + 0.2f);
	}
#endif

	return albedo;
}
