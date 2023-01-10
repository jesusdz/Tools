struct PixelInput
{
	float4 Color : COLOR;
};

float4 main(PixelInput IN) : SV_Target
{
	return IN.Color;
}

