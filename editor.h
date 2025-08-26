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

struct Editor
{
	bool showDebugUI;
	bool showAssets;
	bool showGrid;

	bool selectEntity;
	u32 selectedEntity;

	ProjectionType cameraType;
	Camera camera[ProjectionTypeCount];
	bool cameraOrbit;

	EditorCommand commands[128];
	u32 commandCount;

	ImageH iconAsset;
	ImageH iconWav;
	ImageH iconImg;
};

struct Engine;

void EditorInitialize(Engine &engine);
void EditorUpdate(Engine &engine);
void EditorRender(Engine &engine, CommandList &commandList);
void EditorPostRender(Engine &engine);

#endif // #ifndef EDITOR_H
