#include "globals.hlsl"

struct VertexInput
{
	float2 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
};

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	OUT.position = mul(globals.camera2dProj, float4(IN.position, 0.0, 1.0));
	OUT.texCoord = IN.texCoord;
	OUT.color = IN.color;
	return OUT;
}

struct PixelInput
{
	float2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct PixelOutput
{
	float4 color;
};

SamplerState widgetSampler : REGISTER_S(3, 0);
Texture2D<float4> widgetTexture : REGISTER_T(3, 1);

//float3 SrgbToLinear(float3 srgb)
//{
//	return pow(max(srgb, 0.0), 2.2);
//}

PixelOutput PSMain(PixelInput IN) : SV_Target
{
	PixelOutput OUT = (PixelOutput)0;
	const float4 color = IN.color; //float4(SrgbToLinear(IN.color.rgb), IN.color.a);
	OUT.color = color * widgetTexture.Sample(widgetSampler, IN.texCoord);
	return OUT;
}

