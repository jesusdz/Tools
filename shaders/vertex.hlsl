struct VertexInput
{
	//float3 Position : POSITION;
	float2 Position : POSITION;
	float2 Color    : COLOR;
};

struct VertexOutput
{
	float4 Color    : COLOR;
	float4 Position : SV_Position;
};

//struct ModelViewProjection
//{
//	float4x4 MVP;
//};

//ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

VertexOutput main(VertexInput IN)
{
	VertexOutput OUT;
	//OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 0.0f, 1.0f));
	OUT.Position = float4(IN.Position, 0.0f, 1.0f);
	OUT.Color = float4(IN.Color, 0.0f, 1.0f);
	return OUT;
}

