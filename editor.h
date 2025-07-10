#ifndef EDITOR_H
#define EDITOR_H

struct Editor
{
	bool selectEntity;
	u32 selectedEntity;

	ProjectionType cameraType;
	Camera camera[ProjectionTypeCount];
};

struct Engine;

void EditorUpdate(Engine &engine);
void EditorUpdatePostRender(Engine &engine);

#endif // #ifndef EDITOR_H
