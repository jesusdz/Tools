#ifndef EDITOR_H
#define EDITOR_H

enum EditorCommandType
{
	EditorCommandReloadGraphicsPipeline,
	EditorCommandRemoveTexture,
	EditorCommandLoadTxt,
	EditorCommandSaveTxt,
	EditorCommandLoadBin,
	EditorCommandBuildBin,
	EditorCommandClean,
	EditorCommandCount,
};

struct EditorCommand
{
	EditorCommandType type;
	union
	{
		u32 pipelineIndex;
		TextureH textureH;
	};
};

enum EditorInspectedType
{
	EditorInspectedType_None,
	EditorInspectedType_Image,
	EditorInspectedType_Audio,
	EditorInspectedType_Count,
};

static const char *EditorInspectedTypeName[] = {
	"None",
	"Image",
	"Audio",
};
CT_ASSERT(ARRAY_COUNT(EditorInspectedTypeName) == EditorInspectedType_Count);

struct EditorInspector
{
	EditorInspectedType inspectedType;
	FilePath path;

	ImageH image;
};

struct Editor
{
	bool showDebugUI;
	bool showAssets;
	bool showInspector;
	bool showGrid;

	bool selectEntity;
	u32 selectedEntity;

	Camera camera[ProjectionTypeCount];
	bool cameraOrbit;

	EditorCommand commands[128];
	u32 commandCount;

	ImageH iconAsset;
	ImageH iconWav;
	ImageH iconImg;

	EditorInspector inspector;
};

struct Engine;

void EditorInitialize(Engine &engine);
void EditorUpdate(Engine &engine);
void EditorRender(Engine &engine, CommandList &commandList);
void EditorPostRender(Engine &engine);

#endif // #ifndef EDITOR_H
