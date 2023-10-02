#include "tools.h"


// Types /////////////////////////////////////////////////////////////////////////////////

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


// Endianness ////////////////////////////////////////////////////////////////////////////

union SpirvEndianness
{
	unsigned char values[4];
	struct {
		unsigned int value;
	};
};

static const SpirvEndianness SPIRV_ENDIANNESS_LITTLE_ENDIAN = { 4, 3, 2, 1 };
static const SpirvEndianness SPIRV_ENDIANNESS_BIG_ENDIAN = { 1, 2, 3, 4 };
static const unsigned int SPIRV_PLATFORM_ENDIANNESS = 0x01020304;

bool SpirvPlatformIsLittleEndian()
{
	return SPIRV_PLATFORM_ENDIANNESS == SPIRV_ENDIANNESS_LITTLE_ENDIAN.value;
}

bool SpirvPlatformIsBigEndian()
{
	return SPIRV_PLATFORM_ENDIANNESS == SPIRV_ENDIANNESS_BIG_ENDIAN.value;
}


// Parsing ///////////////////////////////////////////////////////////////////////////////

bool SpirvParse(byte *data, u32 size, SpirvModule *spirv)
{
	// TODO: Assert data size is multiple of 4-byte

	spirv->header = (SpirvHeader*)data;

	// TODO: Check endiannes based on the magic number
	//       Check https://github.com/KhronosGroup/SPIRV-Tools/blob/main/source/spirv_endian.cpp
	// TODO: Make read functions based on endiannes
	//       Check repo: https://github.com/KhronosGroup/SPIRV-Tools/tree/main/source

	return true;
}


// Main program //////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	// Create a memory arena
	u32 memorySize = MB(1);
	byte *memory = (byte*)AllocateVirtualMemory(memorySize);
	Arena arena = MakeArena(memory, memorySize);

	// Get shader file memory blob
	const char *shaderFile = "spirv/vertex.spv";
	DataChunk *chunk = PushFile(arena, shaderFile);
	if (!chunk) {
		LOG(Error, "Error reading %s\n", shaderFile);
		return -1;
	}

	// Print platform endianness
	LOG(Info, "Platform endianness: %s endian\n", SpirvPlatformIsLittleEndian() ? "little" : "big" );

	// Parse spirv bytecode
	SpirvModule spirv = {};
	const bool ok = SpirvParse(chunk->data, chunk->size, &spirv);

	// Log some spirv information
	LOG(Info, "magicNumber: %d\n", spirv.header->magic);
	LOG(Info, "versionNumberMajor: %d\n", spirv.header->major);
	LOG(Info, "versionNumberMinor: %d\n", spirv.header->minor);
	LOG(Info, "generatorMagicNumber: %d\n", spirv.header->generator);
	LOG(Info, "bound: %d\n", spirv.header->bound);
	LOG(Info, "schema: %d\n", spirv.header->schema);

	return 0;
}

