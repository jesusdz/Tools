#include "../tools.h"

#define SPV_ASSERT ASSERT
#define SPV_PRINTF(...) LOG(Info, ##__VA_ARGS__)
#define SPV_IMPLEMENTATION
#define SPV_PRINT_FUNCTIONS
#include "../tools_spirv.h"

#define PRINT_DISASSEMBLY 0

int main(int argc, char **argv)
{
	// Check input arguments
	if ( argc < 2 ) {
		LOG(Error, "Usage: %s <file1.spv> <file2.spv> ...\n", argv[0]);
		return -1;
	}

	// Get shader filenames
	const char *shaderFileNames[2];
	const u32 shaderFileCount = Min(argc - 1, ARRAY_COUNT(shaderFileNames));
	for (u32 i = 0; i < shaderFileCount; ++i) {
		shaderFileNames[i] = argv[i + 1];
	}

	// Create a memory arena
	u32 memorySize = MB(1);
	byte *memory = (byte*)AllocateVirtualMemory(memorySize);
	Arena arena = MakeArena(memory, memorySize, "Main arena");
	
	const u32 spvIdCount = 4096;
	SpvId *spvIds = PushArray(arena, SpvId, spvIdCount);

#if PRINT_DISASSEMBLY
	for (u32 i = 0; i < shaderFileCount; ++i)
	{
		// Get shader file memory blob
		const char *shaderFile = shaderFileNames[i];
		DataChunk *chunk = PushFile(arena, shaderFile);
		if (!chunk) {
			LOG(Error, "Error reading %s\n", shaderFile);
			return -1;
		}

		// Prepare a SpvParser
		SpvParser spirvParser = SpvParserInit( chunk->bytes, chunk->size );

		// Parse spirv bytecode
		SpvPrintDisassembly(&spirvParser);
	}
#else
	// Parse descriptor set layouts
	SpvDescriptorSetList descriptorSetList = {};

	for (u32 i = 0; i < shaderFileCount; ++i)
	{
		// Get shader file memory blob
		const char *shaderFile = shaderFileNames[i];
		DataChunk *chunk = PushFile(arena, shaderFile);
		if (!chunk) {
			LOG(Error, "Error reading %s\n", shaderFile);
			return -1;
		}

		// Prepare a SpvParser
		SpvParser spirvParser = SpvParserInit( chunk->bytes, chunk->size );

		const bool ok = SpvParseDescriptors(&spirvParser, &descriptorSetList, spvIds, sizeof(SpvId) * spvIdCount );
	}

	SpvPrintDescriptorSetList(&descriptorSetList);
#endif

	return 0;
}

