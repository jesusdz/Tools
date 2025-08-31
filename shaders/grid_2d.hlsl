#include "globals.hlsl"

#define USE_VIEWPORT_ROTATION 1

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

struct PixelOutput
{
	float4 color : SV_Target;
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

float4x4 inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

PixelOutput PSMain(PixelInput IN)
{
	float3 frustumCornerA = globals.cameraFrustumTopLeft.xyz;
	float3 frustumCornerB = globals.cameraFrustumBottomRight.xyz;
	float3 lerpFactor = float3(IN.texCoord, 0.0); // Z is constant in the near plane
	float4 pixelPosView = float4(lerp(frustumCornerA, frustumCornerB, lerpFactor), 1.0);

#if 0
	float4x4 inv = globals.cameraView;
	inv[0][0] = 1;
	inv[1][0] = 0;
	inv[2][0] = 0;
	inv[3][0] = 0;

	inv[0][1] = 0;
	inv[1][1] = 1;
	inv[3][1] = 0;
	inv[3][1] = 0;

	inv[0][2] = 0;
	inv[1][2] = 0;
	inv[2][2] = 1;
	inv[3][2] = 0;

	inv[0][3] = -inv[0][3];
	inv[1][3] = -inv[1][3];
	inv[2][3] = -inv[2][3];
	inv[3][3] = 1;
	float3 pixelPosWorld = normalize(mul(inv, pixelPosView).xyz);
#else
	float3 pixelPosWorld = pixelPosView.xyz;
	pixelPosWorld.x -= globals.cameraView[0][3];
	pixelPosWorld.y -= globals.cameraView[1][3];
#endif

	float2 pattern = frac(pixelPosWorld.xy);
	float2 pattern10 = frac(pixelPosWorld.xy / 10.0f);
	float dX = ddx(pixelPosWorld.x);
	float dX10 = ddx(pixelPosWorld.x / 10.0f);

	float3 red = float3(1.0, 0.5, 0.5);
	float3 green = float3(0.5, 1.0, 0.5);
	float greenFactor = step( abs(pixelPosWorld.x), abs(ddx(pixelPosWorld.x)));
	float redFactor = step( abs(pixelPosWorld.y), abs(ddy(pixelPosWorld.y)));
	float3 lineColor = 0.3.xxx;
	lineColor = lerp(lineColor, red, redFactor);
	lineColor = lerp(lineColor, green, greenFactor);

	float3 bgColor = 0.05.xxx;

	float2 lineFactors = step(pattern, dX.xx);
	float lineFactor = max(lineFactors.x, lineFactors.y);
	lineFactor *= min(1.0, 0.01 / dX);

	float2 lineFactors10 = step(pattern10, dX10.xx);
	float lineFactor10 = max(lineFactors10.x, lineFactors10.y);
	lineFactor10 *= min(1.0, 0.03 / dX10);

	lineFactor = max(lineFactor, lineFactor10);

	float3 finalColor = lerp(bgColor, lineColor, lineFactor);

	PixelOutput OUT;
	OUT.color = float4(finalColor, 1.0);
	return OUT;
}

