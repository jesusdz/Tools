#include "globals.hlsl"

RWBuffer<uint> outBuffer : REGISTER_U(3, 0);
Texture2D<uint> inImage : REGISTER_T(3, 1);

[numthreads(1, 1, 1)]
void CSMain( uint3 dtid : SV_DispatchThreadID )
{
	int2 mousePos = globals.mousePosition;
	const uint entityId = inImage.Load(uint3(mousePos.x, mousePos.y, 0));
	outBuffer[0] = entityId;
}

