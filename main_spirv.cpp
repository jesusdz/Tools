#include "tools_spirv.h"

int main(int argc, char **argv)
{
	// Get shader filename
	if ( argc != 2 ) {
		LOG(Error, "Usage: %s <spirv_file.spv>\n", argv[0]);
		return -1;
	}
	const char *shaderFile = argv[1];

	// Create a memory arena
	u32 memorySize = MB(1);
	byte *memory = (byte*)AllocateVirtualMemory(memorySize);
	Arena arena = MakeArena(memory, memorySize);

	// Get shader file memory blob
	DataChunk *chunk = PushFile(arena, shaderFile);
	if (!chunk) {
		LOG(Error, "Error reading %s\n", shaderFile);
		return -1;
	}

	// Prepare a SpvParser
	u32 *words = (u32*)chunk->data;
	const u32 wordCount = chunk->size / 4;
	ASSERT( chunk->size % 4 == 0 );
	SpvParser spirvParser = SpvParserInit( words, wordCount );

#define PARSE_INSTRUCTIONS 0
#if PARSE_INSTRUCTIONS
	// Parse spirv bytecode
	SpvModule spirv = {};
	const bool ok = SpvParse(&spirvParser, &spirv);

	// Log spirv module header information
	LOG(Info, "\nHeader info\n");
	LOG(Info, "- magic: %x\n", spirv.header->magic);
	LOG(Info, "- major: %d\n", SpvVersionMajor(spirv.header->version));
	LOG(Info, "- minor: %d\n", SpvVersionMinor(spirv.header->version));
	LOG(Info, "- generator: %d\n", spirv.header->generator);
	LOG(Info, "- bound: %d\n", spirv.header->bound);
	LOG(Info, "- schema: %d\n", spirv.header->schema);

	LOG(Info, "\nParsed information\n");
	LOG(Info, "- Result: %s\n", ok ? "Success" : "Fail");
#else
	// Parse descriptor set layouts
	SpvDescriptorSetLayout descriptorSetLayout = {};
	const bool ok = SpvParse(&spirvParser, &descriptorSetLayout);
#endif

	return 0;
}

