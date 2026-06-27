#include "globals.hlsl"

struct VertexInput
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	OUT.position = float4(IN.position, 1.0);
	OUT.texCoord = IN.texCoord;
	return OUT;
}

struct PixelInput
{
	float2 texCoord : TEXCOORD0;
};

struct PixelOutput
{
	float4 color;
};

SamplerState imageSampler : REGISTER_S(3, 0);
Texture2D<float4> imageTexture : REGISTER_T(3, 1);

PixelOutput PSMain(PixelInput IN) : SV_Target
{
	PixelOutput OUT = (PixelOutput)0;
	OUT.color = imageTexture.Sample(imageSampler, IN.texCoord);
	return OUT;
}

