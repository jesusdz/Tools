#include "tools.h"


// NOTE: Check for reference: https://github.com/KhronosGroup/SPIRV-Tools/tree/main/source
// NOTE: Check for reference: https://github.com/KhronosGroup/SPIRV-Tools/blob/main/source/spirv_endian.cpp

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
	u32 version;
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

	SpirvHeader *header = (SpirvHeader*)data;

	// Check module endianness based on the magic number
	SpirvEndianness endianness;
	if ( header->magic == SPIRV_MAGIC_LITTLE_ENDIAN.value ) {
		endianness = SPIRV_ENDIANNESS_LITTLE_ENDIAN;
	} else {
		endianness = SPIRV_ENDIANNESS_BIG_ENDIAN;
	}

	// Fix values for host endianness
	header->magic = SpirvFixWord( header->magic, endianness );
	header->version = SpirvFixWord( header->version, endianness );
	header->generator = SpirvFixWord( header->generator, endianness );
	header->bound = SpirvFixWord( header->bound, endianness );
	header->schema = SpirvFixWord( header->schema, endianness );

	// Set output values
	spirv->header = (SpirvHeader*)data;
	spirv->endianness = endianness;

	return true;
}



// Getters ///////////////////////////////////////////////////////////////////////////////

u8 SpirvVersionMajor(u32 version)
{
	const u8 major = ( version & 0x00ff0000 ) >> 16;
	return major;
}

u8 SpirvVersionMinor(u32 version)
{
	const u8 minor = ( version & 0x0000ff00 ) >> 8;
	return minor;
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
	LOG(Info, "magic: %x\n", spirv.header->magic);
	LOG(Info, "major: %d\n", SpirvVersionMajor(spirv.header->version));
	LOG(Info, "minor: %d\n", SpirvVersionMinor(spirv.header->version));
	LOG(Info, "generator: %d\n", spirv.header->generator);
	LOG(Info, "bound: %d\n", spirv.header->bound);
	LOG(Info, "schema: %d\n", spirv.header->schema);

	return 0;
}

