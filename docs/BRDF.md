# BRDF notes

```
// Smith masking function GGX.
// Amount of visible surface given a certain direction.
// aDir is the direction to compute masking for (either incoming or outgoing).
// The returned value is a number in the range [0, 1].
float G1GGX(float3 aDir, float3 aMicrofacetNormal, float aRoughnessAlpha)
{
    // aDir is normalized
    // aMicrofacetNormal is normalized
    // aRoughnessAlpha is not negative

    if (aMicrofacetNormal.z <= 0)
        return 0.0f;

    const float cosThetaVM = Dot(aDir, aMicrofacetNormal);
    if ((aDir.z * cosThetaVM) < 0.0f)
        return 0.0f; // up direction is below microfacet or vice versa

    const float roughnessAlphaSqr = aRoughnessAlpha * aRoughnessAlpha;
    const float tanThetaSqr = Geom::TanThetaSqr(aDir);
    const float root = sqrt(1.0f + roughnessAlphaSqr * tanThetaSqr);

    const float result = 2.0f / (1.0f + root);

    // result range is [0, 1]
    return result;
}

// Geometry GGX function.
// Amount of lit surface visible from the eye point of view.
// That's why its the combination of the Smith GGX masking function for the view (v) and light (l) vectors.
// The returned value is a number in the range [0, 1].
float :GeometryGGX(float3 v, float3 l, float3 n, float3 h, float a)
{
    return G1GGX(v, h, a) * G1GGX(l, h, a);
}

// Amount of surface with geometric normal (n) whose microfacets match the halfway vector (h).
// Alpha is the squared geometric roughness of the material (roughness * roughness).
// The returned value is a number in the range [0, 1].
float DistributionGGX(float3 n, float3 h, float alpha)
{
    float alpha2 = alpha * alpha;
    float NoH = saturate(dot(n, h));
    float denom = (NoH * NoH * (alpha2 - 1.0f)) + 1.0f;
    return alpha2 / max((float)PI * denom * denom, 1e-7f);
}
```

Explained in:
*[Correct form of the GGX geometry term](https://computergraphics.stackexchange.com/questions/2489/correct-form-of-the-ggx-geometry-term)
