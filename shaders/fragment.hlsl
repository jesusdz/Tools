struct PixelInput
{
	float4 position : SV_Position;
	float3 normalWs : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

// Create a combined image/sampler at binding=1, descriptorSet=1
// Register for texture and sampler are t0, s0 in DX.
// Reference: https://github.com/microsoft/DirectXShaderCompiler/wiki/Vulkan-combined-image-sampler-type
[[vk::combinedImageSampler]]
Texture2D<float4> tex : register(t0, space1);
[[vk::combinedImageSampler]]
SamplerState texSampler : register(s0, space1);

float4 main(PixelInput IN) : SV_Target
{
	float3 albedo = tex.Sample(texSampler, IN.texCoord).rgb;
	float3 L = normalize(float3(1.0, 3.0, 2.0));
	float3 N = normalize(IN.normalWs);
	float3 lightColor = float3(1.0, 1.0, 1.0);
	float3 diffuse = 0.8 * clamp(dot(N, L), 0.0, 1.0) * lightColor;
	float3 ambient = 0.2 * lightColor;
	return float4((diffuse + ambient) * albedo, 1.0);
}

