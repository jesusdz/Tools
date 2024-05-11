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

	const Assets *assets = NULL;

	// Parse file
	Clon clon = {};
	if (ClonParse(&arena, chunk->chars, chunk->size, &clon))
	{
		const char *type_name = "Assets";
		const char *global_name = "gAssets";
		const ClonGlobal *global = ClonGetGlobal(&clon, type_name, global_name);
		assets = (const Assets*)global->data;
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

	const TextureDesc *textures = assets->textures;
	for (u32 i = 0; i < assets->texturesCount; ++i)
	{
		LOG(Info, "texture[%u]\n", i);
		LOG(Info, "- name: %s\n", textures[i].name);
		LOG(Info, "- filename: %s\n", textures[i].filename);
	}

	const EntityDesc *entities = assets->entities;
	for (u32 i = 0; i < assets->entitiesCount; ++i)
	{
		LOG(Info, "entity[%u]\n", i);
		LOG(Info, "- name: %s\n", entities[i].name);
		LOG(Info, "- materialName: %s\n", entities[i].materialName);
		LOG(Info, "- scale: %f\n", entities[i].scale);
	}

	return 0;
}

