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
		const char *filepath;
	};
};

enum EditorInspectedType
{
	EditorInspectedType_None,
	EditorInspectedType_Entity,
	EditorInspectedType_Material,
	EditorInspectedType_Texture,
	EditorInspectedType_Image,
	EditorInspectedType_Audio,
	EditorInspectedType_Music,
	EditorInspectedType_Count,
	EditorInspectedType_AssetFileBegin = EditorInspectedType_Image,
	EditorInspectedType_AssetFileEnd = EditorInspectedType_Music,
};

static const char *EditorInspectedTypeName[] = {
	"None",
	"Entity",
	"Material",
	"Texture",
	"Image",
	"Audio",
	"Music",
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
	Handle selectedItem;
	EditorInspectedType inspectedType;

	Handle nextSelectedItem;
	EditorInspectedType nextInspectedType;

	FileNode *selectedAssetFile;
	FileNode *nextSelectedAssetFile;
};

struct Editor
{
	bool showLoadScene;
	bool showSaveScene;
	bool showDebugUI;
	bool showOutliner;
	bool showAssets;
	bool showInspector;
	bool showTilesets;
	bool showGrid;
	bool showAbout;

	bool selectEntity;

	bool isTranslating;

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

inline Handle EditorGetSelectedEntity(const Editor &editor)
{
	Handle handle = InvalidHandle;
	if ( editor.inspector.inspectedType == EditorInspectedType_Entity ) {
		handle = editor.inspector.selectedItem;
	}
	return handle;
}

#endif // #ifndef EDITOR_H
