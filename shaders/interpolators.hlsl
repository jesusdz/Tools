
struct Interpolators
{
	float4 position : SV_Position;
#if USE_INTERPOLATOR_POSITION_WS
	float4 positionWs : POSITION0;
#endif
#if USE_INTERPOLATOR_NORMAL_WS
	float3 normalWs : NORMAL0;
#endif
#if USE_INTERPOLATOR_TEXCOORD
	float2 texCoord : TEXCOORD0;
#endif
#if USE_INTERPOLATOR_SHADOWMAP_COORD
	float4 shadowmapCoord : POSITION1;
#endif
};

typedef Interpolators VertexOutput;
typedef Interpolators PixelInput;

