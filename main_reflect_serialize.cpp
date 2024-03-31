#include "tools.h"
#include "assets.h"


////////////////////////////////////////////////////////////////////////
// Print utils

#define Printf( format, ... ) LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ ); IncreaseIndentation();
#define PrintEndScope( format, ... ) DecreaseIndentation(); LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ );

static u32 indentPos = 0;
static char indent[64] = {};

static void IncreaseIndentation()
{
	indent[indentPos++] = '\t';
}

static void DecreaseIndentation()
{
	ASSERT(indentPos > 0);
	indent[--indentPos] = '\0';
}


////////////////////////////////////////////////////////////////////////
// Automatic implementation (using reflection)

//#include "assets_refl.h"

// TODO


////////////////////////////////////////////////////////////////////////
// Manual implementation

void Print(const TextureDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"filename\" : \"%s\",", desc->filename);
	PrintEndScope("},");
}

void Print(const PipelineDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"vsFilename\" : \"%s\",", desc->vsFilename);
	Printf("\"fsFilename\" : \"%s\",", desc->fsFilename);
	PrintEndScope("},");
}

void Print(const MaterialDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"textureName\" : \"%s\",", desc->textureName);
	Printf("\"pipelineName\" : \"%s\",", desc->pipelineName);
	Printf("\"uvScale\" : \"%f\",", desc->uvScale);
	PrintEndScope("},");
}

void Print(const EntityDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"materialName\" : \"%s\",", desc->materialName);
	Printf("\"pos\" : [%f, %f, %f],", desc->pos.x, desc->pos.y, desc->pos.z);
	Printf("\"scale\" : %f,", desc->scale);
	Printf("\"geometryType\" : \"%d\",", desc->geometryType);
	PrintEndScope("},");
}

void Print(const Assets *assets)
{
	PrintBeginScope("{");

	PrintBeginScope("\"textures\" : [");
	for (int i = 0; i < assets->textureCount; ++i)
	{
		Print(&assets->textures[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"pipelines\" : [");
	for (int i = 0; i < assets->pipelineCount; ++i)
	{
		Print(&assets->pipelines[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"materials\" : [");
	for (int i = 0; i < assets->materialCount; ++i)
	{
		Print(&assets->materials[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"entities\" : [");
	for (int i = 0; i < assets->entityCount; ++i)
	{
		Print(&assets->entities[i]);
	}
	PrintEndScope("],");

	PrintEndScope("};");
}


////////////////////////////////////////////////////////////////////////
//  Main function

int main(int argc, char **argv)
{
	Print(&gAssets);

	return 0;
}

