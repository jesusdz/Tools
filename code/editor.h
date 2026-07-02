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

enum EditorSelectedType
{
	EditorSelectedType_None,
	EditorSelectedType_Scene,
	EditorSelectedType_Room,
	EditorSelectedType_Layer,
	EditorSelectedType_Entity,
	EditorSelectedType_Material,
	EditorSelectedType_Texture,
	EditorSelectedType_Audio,
	EditorSelectedType_Music,
	EditorSelectedType_Sprite,
	EditorSelectedType_FileImage,
	EditorSelectedType_FileAudio,
	EditorSelectedType_FileMusic,
	EditorSelectedType_Count,
	EditorSelectedType_FileBegin = EditorSelectedType_FileImage,
	EditorSelectedType_FileEnd = EditorSelectedType_FileMusic,
};

static const char *EditorSelectedTypeName[] = {
	"None",
	"Scene",
	"Room",
	"Layer",
	"Entity",
	"Material",
	"Texture",
	"Audio",
	"Music",
	"Sprite",
	"Image file",
	"Audio file",
	"Music file",
};
CT_ASSERT(ARRAY_COUNT(EditorSelectedTypeName) == EditorSelectedType_Count);

enum EditorDrawTool
{
	EditorDrawTool_Draw,
	EditorDrawTool_Erase,
};

struct EditorTilesets
{
	SpriteH selectedSprite;
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

struct EditorSelection
{
	EditorSelectedType type;
	union
	{
		Scene *scene;
		Room *room;
		Layer *layer;
		Handle handle;
		FileNode *file;
		u64 value;
	};
};

struct EditorInspector
{
	EditorSelection selected;
	EditorSelection nextSelected;
	Handle tmpHandle;
};

struct EditorContext
{
	FileNode *selectedFile;
	Room *room;
	Layer *layer;
	// Layer
	// Brush
	// Tool
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
	ProjectionType cameraType;
	bool cameraOrbit;

	EditorCommand commands[128];
	u32 commandCount;

	ImageH iconAsset;
	ImageH iconWav;
	ImageH iconMod;
	ImageH iconImg;
	ImageH iluLogo;

	SnapshotNode *snapshots;

	EditorContext context;
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
	if ( editor.inspector.selected.type == EditorSelectedType_Entity ) {
		handle = editor.inspector.selected.handle;
	}
	return handle;
}

#endif // #ifndef EDITOR_H
