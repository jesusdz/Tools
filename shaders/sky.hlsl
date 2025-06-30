#include "globals.hlsl"

#define USE_VIEWPORT_ROTATION 1

SamplerState skySampler : REGISTER_S(3, 0);
Texture2D<float4> skyTexture : REGISTER_T(3, 1);

struct VertexInput
{
	float3 position : POSITION;
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
	float2 texCoord = IN.texCoord.xy;
#if USE_VIEWPORT_ROTATION
	float4 centeredTexCoord = float4(texCoord - 0.5.xx, 0.0, 1.0);
	float4 rotatedTexCoord = mul(globals.viewportRotationMatrix, centeredTexCoord);
	texCoord = rotatedTexCoord.xy + 0.5.xx;
#endif

	VertexOutput OUT;
	OUT.position = float4(IN.position, 1.0);
	OUT.texCoord = texCoord;
	return OUT;
}

#define PI 3.14159265359

float4 PSMain(PixelInput IN) : SV_Target
{
	float3 frustumCornerA = globals.cameraFrustumTopLeft.xyz;
	float3 frustumCornerB = globals.cameraFrustumBottomRight.xyz;
	float3 lerpFactor = float3(IN.texCoord, 0.0); // Z is constant in the near plane
	float4 pixelDirView = float4(lerp(frustumCornerA, frustumCornerB, lerpFactor), 0.0);
	float3 pixelDirWorld = normalize(mul(globals.cameraViewInv, pixelDirView).xyz);

	float yaw = atan2(pixelDirWorld.z, pixelDirWorld.x); // [-PI, PI]
	float pitch = asin(pixelDirWorld.y); // [-PI/2, PI/2]

	float yawNormalized = (yaw / PI) * 0.5 + 0.5;
	float pitchNormalized = 1.0 - pitch * 2.0 / PI;

	float2 skyTexCoord = float2(yawNormalized, pitchNormalized);
	float3 skyColor = skyTexture.SampleLevel(skySampler, skyTexCoord, 0.0).rgb;

	float3 groundColor = 0.05.xxx;
	float groundFactor = smoothstep(1.0, 1.02, skyTexCoord.y);
	float4 finalColor = float4(lerp(skyColor, groundColor, groundFactor), 1.0);

	if (globals.flipRB) {
		finalColor.rb = finalColor.br;
	}

	return finalColor;
}

