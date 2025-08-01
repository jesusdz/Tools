#ifndef EDITOR_H
#define EDITOR_H

enum EditorCommandType
{
	EditorCommandReloadGraphicsPipeline,
	EditorCommandRemoveTexture,
	EditorCommandSave,
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
	bool selectEntity;
	u32 selectedEntity;

	ProjectionType cameraType;
	Camera camera[ProjectionTypeCount];

	EditorCommand commands[128];
	u32 commandCount;
};

struct Engine;

void EditorInitialize(Engine &engine);
void EditorUpdate(Engine &engine);
void EditorRender(Engine &engine, CommandList &commandList);
void EditorUpdatePostRender(Engine &engine);

#endif // #ifndef EDITOR_H
