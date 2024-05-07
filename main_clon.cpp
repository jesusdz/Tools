#include "tools.h"
#include "clon.h"

#include "assets.h"

int main(int argc, char **argv)
{
	// File to parse
	const char *filename = "assets.h";

	// Create a memory arena
	u32 memorySize = MB(1);
	byte *memory = (byte*)AllocateVirtualMemory(memorySize);
	Arena arena = MakeArena(memory, memorySize);

	// Read file
	DataChunk *chunk = PushFile(arena, filename);
	if (!chunk)
	{
		LOG(Error, "Error reading file: %s\n", filename);
		return -1;
	}

	Assets *assets = NULL;

	// Parse file
	Clon clon = {};
	if (ClonParse(&arena, chunk->chars, chunk->size, &clon))
	{
		const char *type_name = "Assets";
		const char *global_name = "gAssets";
		assets = (Assets*)ClonGetGlobal(&clon, type_name, global_name);
		if (!assets)
		{
			LOG(Error, "Could not find object %s of type %s in %s\n", global_name, type_name, filename);
			return -1;
		}
	}
	else
	{
		LOG(Error, "Error in ClonParse: %s\n", filename);
		return -1;
	}

	LOG(Info, "gAssets:\n");
	LOG(Info, "- texturesCount: %u\n", assets->texturesCount);
	LOG(Info, "- pipelinesCount: %u\n", assets->pipelinesCount);
	LOG(Info, "- materialsCount: %u\n", assets->materialsCount);
	LOG(Info, "- entitiesCount: %u\n", assets->entitiesCount);

	return 0;
}

