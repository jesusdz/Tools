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
	EditorInspectedType_Music,
	EditorInspectedType_Entity,
	EditorInspectedType_Count,
	EditorInspectedType_AssetFileBegin = EditorInspectedType_Image,
	EditorInspectedType_AssetFileEnd = EditorInspectedType_Music,
};

static const char *EditorInspectedTypeName[] = {
	"None",
	"Image",
	"Audio",
	"Music",
	"Entity",
};
CT_ASSERT(ARRAY_COUNT(EditorInspectedTypeName) == EditorInspectedType_Count);

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

enum FileNodeType
{
	FileNodeType_Image,
	FileNodeType_Music,
	FileNodeType_Sound,
	FileNodeType_COUNT,
};

struct FileNode
{
	FileNodeType type;
	const char *filename;

	FileNode *next; // Next file in same directory
	FileNode *prev; // Prev file in same directory
	FileNode *child; // For directy contents
};

struct SnapshotNode
{
	const char *filepath;
	ImageH imageH;
	SnapshotNode *next;
};

struct EditorInspector
{
	EditorInspectedType inspectedType;
	FileNode *assetFile;

	TextureH textureH;
	AudioClipH audioClipH;
	Handle musicH;

	bool refresh;
};

struct Editor
{
	bool showDebugUI;
	bool showAssets;
	bool showInspector;
	bool showTilesets;
	bool showGrid;
	bool showAbout;

	bool selectEntity;
	Handle selectedEntity;

	bool setGridTile;
	int2 setGridTileCoord;

	Camera camera[ProjectionTypeCount];
	bool cameraOrbit;

	EditorCommand commands[128];
	u32 commandCount;

	ImageH iconAsset;
	ImageH iconWav;
	ImageH iconMod;
	ImageH iconImg;
	ImageH iluLogo;

	SnapshotNode *snapshots;

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
