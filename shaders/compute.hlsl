
RWBuffer<float> outBuffer : register(u1);  // RW for read-write, register(u1) for slot assignment

[numthreads(1, 1, 1)]
void main( uint3 dtid : SV_DispatchThreadID )
{
	float value = outBuffer[0];
	value = frac(value + 0.01);
	outBuffer[0] = value;
}

