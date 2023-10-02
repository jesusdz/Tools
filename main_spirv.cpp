#include "tools.h"

struct SpirvHeader
{
	u32 magic;
	u8 padding0;
	u8 major;
	u8 minor;
	u8 padding1;
	u32 generator;
	u32 bound;
	u32 schema;
};

struct SpirvModule
{
	SpirvHeader *header;
	bool bigEndian;
};

int main(int argc, char **argv)
{
	u32 memorySize = MB(1);
	byte *memory = (byte*)AllocateVirtualMemory(memorySize);
	Arena arena = MakeArena(memory, memorySize);

	const char *shaderFile = "spirv/vertex.spv";
	DataChunk *chunk = PushFile(arena, shaderFile);
	if (!chunk)
	{
		LOG(Error, "Error reading %s\n", shaderFile);
	}

	LOG(Info, "File size: %llu\n\n", chunk->size);

	SpirvModule spirv = {};
	spirv.header = (SpirvHeader*)chunk->data;

	// TODO: Check endiannes based on the magic number
	// TODO: Make read functions based on endiannes

	LOG(Info, "magicNumber: %d\n", spirv.header->magic);
	LOG(Info, "versionNumberMajor: %d\n", spirv.header->major);
	LOG(Info, "versionNumberMinor: %d\n", spirv.header->minor);
	LOG(Info, "generatorMagicNumber: %d\n", spirv.header->generator);
	LOG(Info, "bound: %d\n", spirv.header->bound);
	LOG(Info, "schema: %d\n", spirv.header->schema);

#if 0
	u32 *dataU32 = (u32*)chunk->data;
	u16 *dataU16 = (u16*)chunk->data;
	char *dataChar = (char*)chunk->data;
	for (u32 i = 0; i < 32; ++i)
	{
		LOG(Info, "%d ", dataU32[i]);
	}
	LOG(Info,"\n");
	for (u32 i = 0; i < 32; ++i)
	{
		LOG(Info, "%d ", dataU16[i]);
	}
	LOG(Info,"\n");
	for (u32 i = 0; i < 32; ++i)
	{
		LOG(Info, "%d ", dataChar[i]);
	}
	LOG(Info,"\n");
#endif
	return 0;
}


