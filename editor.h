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
	FilePath inspectedFilename;

	TextureH textureH;
};

enum EditorDrawTool
{
	EditorDrawTool_Draw,
	EditorDrawTool_Erase,
};

struct EditorTilesets
{
	Tile tile;
	EditorDrawTool selectedTool;
};

struct FileNode
{
	bool isDirectory;
	const char *filename;
	FileNode *next; // Next file in same directory
	FileNode *prev; // Prev file in same directory
	FileNode *child; // For directy contents
};

struct Editor
{
	bool showDebugUI;
	bool showAssets;
	bool showInspector;
	bool showTilesets;
	bool showGrid;

	bool selectEntity;
	u32 selectedEntity;

	bool setGridTile;
	int2 setGridTileCoord;

	Camera camera[ProjectionTypeCount];
	bool cameraOrbit;

	EditorCommand commands[128];
	u32 commandCount;

	ImageH iconAsset;
	ImageH iconWav;
	ImageH iconImg;

	EditorInspector inspector;
	EditorTilesets tilesets;

	FileNode *root;
	FileNode *freeNodes;
};

struct Engine;

void EditorInitialize(Engine &engine);
void EditorUpdate(Engine &engine);
void EditorRender(Engine &engine, CommandList &commandList);
void EditorPostRender(Engine &engine);

#endif // #ifndef EDITOR_H
