#include "tools.h"


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
static const SpirvEndianness SPIRV_MAGIC_LITTLE_ENDIAN = { 0x03, 0x02, 0x23, 0x07 }; // Magic number: 0x07230203
static const unsigned int SPIRV_HOST_ENDIANNESS = 0x01020304;

unsigned int SpirvFixWord(unsigned int word, SpirvEndianness endianness)
{
	const unsigned int swappedWord = (word & 0x000000ff) << 24 | (word & 0x0000ff00) << 8 | (word & 0x00ff0000) >> 8 | (word & 0xff000000) >> 24;
	const unsigned int fixedWord = ( endianness.value == SPIRV_HOST_ENDIANNESS ? word : swappedWord );
	return fixedWord;
}


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
	SpirvEndianness endianness;
};


// Parsing ///////////////////////////////////////////////////////////////////////////////

bool SpirvParse(byte *data, u32 size, SpirvModule *spirv)
{
	// TODO: Assert data size is multiple of 4-byte

	spirv->header = (SpirvHeader*)data;

	// Check module endianness based on the magic number
	// NOTE: Check for reference https://github.com/KhronosGroup/SPIRV-Tools/blob/main/source/spirv_endian.cpp
	if ( spirv->header->magic == SPIRV_MAGIC_LITTLE_ENDIAN.value )
	{
		spirv->endianness = SPIRV_ENDIANNESS_LITTLE_ENDIAN;
	}
	else
	{
		spirv->endianness = SPIRV_ENDIANNESS_BIG_ENDIAN;
	}

	// TODO: Make read functions based on endianness
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

	// Parse spirv bytecode
	SpirvModule spirv = {};
	const bool ok = SpirvParse(chunk->data, chunk->size, &spirv);

	// Print platform endianness
	LOG(Info, "Host endianness: %s endian\n", SPIRV_HOST_ENDIANNESS == SPIRV_ENDIANNESS_LITTLE_ENDIAN.value ? "little" : "big" );
	LOG(Info, "Spirv endianness: %s endian\n", spirv.endianness.value == SPIRV_ENDIANNESS_LITTLE_ENDIAN.value ? "little" : "big");

	// Log some spirv information
	LOG(Info, "magicNumber: %x\n", spirv.header->magic);
	LOG(Info, "versionNumberMajor: %d\n", spirv.header->major);
	LOG(Info, "versionNumberMinor: %d\n", spirv.header->minor);
	LOG(Info, "generatorMagicNumber: %d\n", spirv.header->generator);
	LOG(Info, "bound: %d\n", spirv.header->bound);
	LOG(Info, "schema: %d\n", spirv.header->schema);

	return 0;
}

