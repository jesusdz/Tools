#include "globals.hlsl"
#include "interpolators.hlsl"

float Lambert(float3 L, float3 N)
{
	float res = saturate(dot(L, N));
	return res;
}

float BlinnPhong(float3 H, float3 N)
{
	float res = pow(max(0.0, dot(H, N)), 128.0);
	return res;
}

float Attenuate(float dist, float maxDist)
{
	float res = 1.0 - saturate(dist / maxDist);
	res = res * res;
	res = res * res;
	return res;
}

float Luminance(float3 color)
{
	float3 luminanceFactors = float3( 0.2126f, 0.7152f, 0.0722f );
	float res = dot(color, luminanceFactors);
	return res;
}

float4 main(PixelInput IN) : SV_Target
{
	// Directional light
	float3 L = normalize(float3(1.0, 3.0, 2.0));
	float attenuationFactor = 1.0;

#if 0
	// Point light
	float3 lightPositionWs = float3(1.0, 3.0, 2.0);
	float3 lightVector = lightPositionWs - IN.positionWs.xyz;
	float3 L = normalize(lightVector);
	float lightDist = length(lightVector);
	float maxDist = 14.0f;
	float attenuationFactor = Attenuate(length(lightVector), maxDist);
#endif

	float3 N = normalize(IN.normalWs);
	float3 V = normalize(globals.eyePosition.xyz - IN.positionWs.xyz);
	float3 H = normalize(V + L);

	float3 albedo = albedoTexture.Sample(linearSampler, IN.texCoord).rgb;
	//float3 albedo = float3(0.5, 0.5, 0.5);

	float ambient = 0.1;
	float diffuse = Lambert(L, N);
	float specular = BlinnPhong(H, N) * Luminance(albedo);

	float3 shadedColor = ((ambient + diffuse) * albedo + specular) * attenuationFactor;
	return float4(shadedColor, 1.0);
}

