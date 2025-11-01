
static const float PI = 3.14159265f;

float TanThetaSqr(float3 w)
{
	float wSqr = w.z * w.z;
	float tmp = 1.0 - wSqr;
	tmp = tmp <= 0.0f ? 0.0f : tmp / wSqr;
	return tmp;
}

// Smith masking function GGX.
// Amount of visible surface given a certain direction.
// dir is the direction to compute masking for (either incoming or outgoing).
// The returned value is a number in the range [0, 1].
float G1GGX(float3 dir, float3 microfacetNormal, float alpha)
{
	// dir is normalized
	// microfacetNormal is normalized
	// alpha is not negative

	if (microfacetNormal.z <= 0.0f)
		return 0.0f;

	const float cosThetaVM = dot(dir, microfacetNormal);

	if (dir.z * cosThetaVM  < 0.0f)
		return 0.0f; // up direction is below microfacet or vice versa

	const float alphaSqr = alpha * alpha;
	const float tanThetaSqr = TanThetaSqr(dir);
	const float root = sqrt(1.0f + alphaSqr * tanThetaSqr);

	const float result = 2.0f / (1.0f + root);

	// result range is [0, 1]
	return result;
}

// Geometry GGX function.
// Amount of lit surface visible from the eye point of view.
// That's why its the combination of the Smith GGX masking function for the view (V) and light (L) vectors.
// The returned value is a number in the range [0, 1].
float GeometryGGX(float3 V, float3 L, float3 N, float3 H, float alpha)
{
	return G1GGX(V, H, alpha) * G1GGX(L, H, alpha);
}

float GeometryUE4(float3 N, float3 V, float roughness)
{
	const float NoV = saturate(dot(N, V));
	return NoV / (NoV * (1.0f - roughness) + roughness);
}

// Amount of surface with geometric normal (N) whose microfacets match the halfway vector (H).
// Alpha is the squared geometric roughness of the material (roughness * roughness).
// The returned value is a number in the range [0, 1].
float DistributionGGX(float3 N, float3 H, float alpha)
{
	const float alpha2 = alpha * alpha;
	const float NoH = saturate(dot(N, H));
	const float NoH2 = NoH * NoH;
	float denom = NoH2 * (alpha2 - 1.0f) + 1.0f;
	denom = PI * denom * denom;
	return alpha2 / denom;
}

// Material reflectivity at a given orientation.
// cosTheta is the dot product between the view direction and the target microfacet normal: dot(V,H).
// R0 is the base reflectivity (when cosTheta == 0) of the material.
float3 Fresnel(float cosTheta, float3 R0)
{
    float3 fresnel = R0 + (float3(1.0f, 1.0f, 1.0f) - R0) * pow(1 - cosTheta, 5.0f);
	return fresnel;
}

