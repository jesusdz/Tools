#include "globals.hlsl"

#define USE_INTERPOLATOR_POSITION_WS 1
#define USE_INTERPOLATOR_NORMAL_WS 1
#define USE_INTERPOLATOR_TEXCOORD 1
#define USE_INTERPOLATOR_SHADOWMAP_COORD 1
#include "interpolators.hlsl"

#include "brdf.hlsl"

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
	//float3 L = normalize(float3(1.0, 3.0, 2.0));
	float3 L = globals.sunDir.xyz; //normalize(float3(1.0, 3.0, 2.0));
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

	//IN.shadowmapCoord.xyz /= IN.shadowmapCoord.w;
	float fragmentShadowmapDepth = IN.shadowmapCoord.z;
	float2 fragmentShadowmapTexCoord = IN.shadowmapCoord.xy * 0.5 + 0.5;
	fragmentShadowmapTexCoord.y = 1.0 - fragmentShadowmapTexCoord.y;
	float sunVisibility = shadowmap.SampleCmp(shadowmapSampler, fragmentShadowmapTexCoord, fragmentShadowmapDepth - 0.005).r;

#if 1
	float ambient = 0.1;
	float diffuse = Lambert(L, N) * sunVisibility;
	float specular = BlinnPhong(H, N) * Luminance(albedo) * sunVisibility;

	float3 shadedColor = ((ambient + diffuse) * albedo + specular) * attenuationFactor;
#else
	// Material properties
	float3 diffColor = albedo;
	float3 specColor = float3(1.0f, 1.0f, 1.0f);// albedo;
	float3 R0 = float3(0.04f, 0.04f, 0.04f);
	float roughness = 0.9f;
	float alpha = roughness * roughness;

	float NoL = saturate(dot(N, L));
	float NoV = saturate(dot(N, V));
	float LoH = saturate(dot(L, H));
	float HoV = saturate(dot(H, V));

	float3 Ra = (diffColor);

	float3 Rd = (diffColor / PI);

	float3 F = Fresnel(HoV, R0);
	float  G = GeometryUE4(N, V, roughness);
	float  D = DistributionGGX(N, H, alpha);
	float3 Rs = (specColor / PI) * F * G * D / (4.0f * NoL * NoV);

	const float lightPower = 50.0f;
	const float3 lightColor = float3(1.0, 0.9, 0.9) * lightPower;
	float3 shadedColor = Ra + lightColor * NoL * lerp(Rs, Rd, F);
#endif

	return float4(shadedColor, 1.0);
}

