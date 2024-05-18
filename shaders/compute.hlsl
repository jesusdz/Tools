
RWBuffer<float> outBuffer : register(u1);  // RW for read-write, register(u1) for slot assignment

[numthreads(1, 1, 1)]
void main_clear( uint3 dtid : SV_DispatchThreadID )
{
	outBuffer[0] = 0;
}

[numthreads(1, 1, 1)]
void main_update( uint3 dtid : SV_DispatchThreadID )
{
	float value = outBuffer[0];
	value = frac(value + 0.01);
	outBuffer[0] = value;
}

