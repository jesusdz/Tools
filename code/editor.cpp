
static const char * InternString(const char *str)
{
	const char *intern = MakeStringIntern(sPlatform->stringInterning, str);
	return intern;
}

static const char * NameFromFilename(const char *name)
{
	FilePath path = {};
	StrCopy(path.str, name);
	char *ptr = path.str;
	while (*ptr) {
		if (*ptr == '.') {
			*ptr = '_';
		}
		ptr++;
	}
	const char *outName = InternString(path.str);
	return outName;
}

static const char *MakeName(const char *format, ...)
{
	FilePath path = {};
	va_list vaList;
	va_start(vaList, format);
	VSPrintf(path.str, format, vaList);
	va_end(vaList);
	const char *outName = InternString(path.str);
	return outName;
}

static bool EditorMode3D(Editor &editor)
{
	return editor.cameraType == ProjectionPerspective;
}

static bool EditorMode2D(Editor &editor)
{
	return !EditorMode3D(editor);
}

static void EditorSetCamera(Editor &editor)
{
	SetCamera(editor.camera[editor.cameraType]);
}

static void EditorSetMode3D(Editor &editor)
{
	editor.cameraType = ProjectionPerspective;
	EditorSetCamera(editor);
}

static void EditorSetMode2D(Editor &editor)
{
	editor.cameraType = ProjectionOrthographic;
	EditorSetCamera(editor);
}

static void AddEditorCommand(Editor &editor, const EditorCommand &command)
{
	ASSERT(editor.commandCount < ARRAY_COUNT(editor.commands));
	editor.commands[editor.commandCount++] = command;
}

static ImageH EditorLoadIcon(Engine &engine, const char *filename, const char *name)
{
	const FilePath path = MakePath(ProjectDir, filename);
	ImagePixels imagePixels;
	Arena scratch = MakeSubArena(FrameArena, "Scratch - EditorLoadIcon");
	ReadImagePixels(scratch, path.str, imagePixels);
	const ImageH handle = EngineCreateImage(engine.gfx, imagePixels, name, false);
	return handle;
}

static ImageH EditorLoadSnapshot(Engine &engine, const char *filepath, const char *name)
{
	ImagePixels imagePixels;
	Arena scratch = MakeSubArena(FrameArena, "Scratch - EditorLoadSnapshot");
	ReadImagePixels(scratch, filepath, imagePixels);
	imagePixels = ResizeImagePixels(scratch, imagePixels, 32, 32);
	const ImageH handle = EngineCreateImage(engine.gfx, imagePixels, name, false);
	return handle;
}

static SnapshotNode *EditorGetOrCreateSnapshotNode(Engine &engine, const char *filepath)
{
	// First we try to find an existing snapshot for this path
	SnapshotNode *snapshot = engine.editor.snapshots;
	while (snapshot)
	{
		if ( StrEq( snapshot->filepath, filepath ) )
		{
			break;
		}
		snapshot = snapshot->next;
	}

	// If it didn't exist, create a new one
	if ( !snapshot )
	{
		snapshot = PushZeroStruct( GlobalArena, SnapshotNode );
		if (engine.editor.snapshots) {
			snapshot->next = engine.editor.snapshots;
		}
		engine.editor.snapshots = snapshot;
		snapshot->filepath = InternString(filepath);
		snapshot->imageH = EditorLoadSnapshot(engine, filepath, "editor_snapshot");
	}

	return snapshot;
}

static void EditorUpdateUI_MenuBar(Engine &engine)
{
	UI &ui = GetUI(engine);
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	if ( UI_BeginMenuBar(ui) )
	{
		if (UI_BeginMenu(ui, "File"))
		{
			if ( UI_MenuItem(ui, "Load scene") )
			{
				editor.showLoadScene = true;
			}
			if ( UI_MenuItem(ui, "Save scene") )
			{
				editor.showSaveScene = true;
			}

			if ( UI_MenuItem(ui, "Clear scene") )
			{
				const EditorCommand command = { .type = EditorCommandClean };
				AddEditorCommand(editor, command);
			}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Load scene (BIN)") )
			{
				const EditorCommand command = { .type = EditorCommandLoadBin };
				AddEditorCommand(editor, command);
			}

			if ( UI_MenuItem(ui, "Build scene (BIN)") )
			{
				const EditorCommand command = { .type = EditorCommandBuildBin };
				AddEditorCommand(editor, command);
			}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Quit") )
			{
				PlatformQuit();
			}
			UI_EndMenu(ui);
		}
		if (UI_BeginMenu(ui, "View"))
		{
			if ( UI_MenuItem(ui, "Outliner", editor.showOutliner) )
			{
				editor.showOutliner = !editor.showOutliner;
			}
			if ( UI_MenuItem(ui, "Assets", editor.showAssets) )
			{
				editor.showAssets = !editor.showAssets;
			}
			if ( UI_MenuItem(ui, "Inspector", editor.showInspector) )
			{
				editor.showInspector = !editor.showInspector;
			}
			if ( UI_MenuItem(ui, "Tilesets", editor.showTilesets) )
			{
				editor.showTilesets = !editor.showTilesets;
			}
			if ( UI_MenuItem(ui, "Debug UI", editor.showDebugUI) )
			{
				editor.showDebugUI = !editor.showDebugUI;
			}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Grid", editor.showGrid) )
			{
				editor.showGrid = !editor.showGrid;
			}

			UI_EndMenu(ui);
		}
		if (UI_BeginMenu(ui, "Help"))
		{
			if ( UI_MenuItem(ui, "About") )
			{
				editor.showAbout = true;
			}
			UI_EndMenu(ui);
		}

		UI_EndMenuBar(ui);
	}
}

static void EditorUpdateUI_ToolBar(Engine &engine)
{
	UI &ui = GetUI(engine);

	if ( UI_BeginToolBar(ui) )
	{
		if ( UI_ButtonIcon(ui, 0) )
		{
			engine.game.state = GameStateStarting;
		}

		UI_Separator(ui);

		if ( UI_Radio(ui, "2D", EditorMode2D(engine.editor)) ) {
			EditorSetMode2D(engine.editor);
		}
		if ( UI_Radio(ui, "3D", EditorMode3D(engine.editor)) ) {
			EditorSetMode3D(engine.editor);
		}

		UI_EndToolBar(ui);
	}
}

static void EditorSelectEntity(Editor &editor, Handle handle)
{
	editor.inspector.nextSelectedHandle = handle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_Entity;
}

static void EditorSelectMaterial(Editor &editor, Handle handle)
{
	editor.inspector.nextSelectedHandle = handle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_Material;
}

static void EditorSelectTexture(Editor &editor, Handle handle)
{
	editor.inspector.nextSelectedHandle = handle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_Texture;
}

static void EditorSelectAudioClip(Editor &editor, Handle handle)
{
	editor.inspector.nextSelectedHandle = handle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_Audio;
}

static void EditorSelectMusic(Editor &editor, Handle handle)
{
	editor.inspector.nextSelectedHandle = handle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_Music;
}

static void EditorSelectSprite(Editor &editor, Handle handle)
{
	editor.inspector.nextSelectedHandle = handle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_Sprite;
}

static void EditorSelectFileImage(Editor &editor, FileNode *node)
{
	editor.inspector.nextSelectedFile = node;
	editor.inspector.nextSelectedHandle = InvalidHandle;
	editor.inspector.nextInspectedType = EditorInspectedType_FileImage;
}

static void EditorSelectFileAudio(Editor &editor, FileNode *node)
{
	editor.inspector.nextSelectedFile = node;
	editor.inspector.nextSelectedHandle = InvalidHandle;
	editor.inspector.nextInspectedType = EditorInspectedType_FileAudio;
}

static void EditorSelectFileMusic(Editor &editor, FileNode *node)
{
	editor.inspector.nextSelectedFile = node;
	editor.inspector.nextSelectedHandle = InvalidHandle;
	editor.inspector.nextInspectedType = EditorInspectedType_FileMusic;
}

static void EditorUnselectAll(Editor &editor)
{
	editor.inspector.nextSelectedHandle = InvalidHandle;
	editor.inspector.nextSelectedFile = nullptr;
	editor.inspector.nextInspectedType = EditorInspectedType_None;
}

static void EditorUpdateUI_DebugUI(Engine &engine)
{
	UI &ui = GetUI(engine);
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	UI_BeginWindow(ui, "Debug UI", &editor.showDebugUI);

	if ( UI_Section(ui, "General") )
	{
		if (EditorMode3D(editor))
		{
			const char *radioOptions[] = { "3D mode", "2D mode" };
			const ProjectionType mode[] = { ProjectionPerspective, ProjectionOrthographic };

			for (u32 i = 0; i < ARRAY_COUNT(radioOptions); ++i)
			{
				if ( UI_Radio(ui, radioOptions[i], editor.cameraType == mode[i]) )
				{
					editor.cameraType = mode[i];
				}
			}
		}
	}

	if ( UI_Section(ui, "Profiling" ) )
	{
		constexpr f32 maxExpectedMillis = 1000.0f / 60.0f; // Like expecting to reach 60 fps

		const u32 sceneWidth = gfx.device.swapchain.extent.width;
		const u32 sceneHeight = gfx.device.swapchain.extent.height;
		UI_Label(ui, "Resolution: %ux%u px", sceneWidth, sceneHeight);

		const TimeSamples &gpuTimes = gfx.gpuFrameTimes;
		UI_Label(ui, "GPU %.02f ms / %.00f fps ", gpuTimes.average, 1000.0f / gpuTimes.average);
		UI_Histogram(ui, gpuTimes.samples, ARRAY_COUNT(gpuTimes.samples), maxExpectedMillis);

		const TimeSamples &cpuTimes = gfx.cpuFrameTimes;
		UI_Label(ui, "CPU %.02f ms / %.00f fps ", cpuTimes.average, 1000.0f / cpuTimes.average);
		UI_Histogram(ui, cpuTimes.samples, ARRAY_COUNT(cpuTimes.samples), maxExpectedMillis + 1.0f);
	}

	if ( UI_Section(ui, "Memory") )
	{
		UI_BeginLayout(ui, UiLayoutHorizontal);
		const char *unitsStrArray[] = { "B", "KB", "MB" };
		const u32 unitsSizeArray[] = { 1, KB(1), MB(1) };
		static u32 units = 0;
		for (u32 i = 0; i < ARRAY_COUNT(unitsStrArray); ++i) {
			if ( UI_Radio(ui, unitsStrArray[i], units == i) ) {
				units = i;
			}
		}
		UI_EndLayout(ui);

		const char *unitsStr = unitsStrArray[units];
		const u32 unitsSize = unitsSizeArray[units];
		UI_Label(ui, "- Global Arena: %u / %u %s", GlobalArena.used / unitsSize, GlobalArena.size / unitsSize, unitsStr);
		UI_Label(ui, "- Frame Arena: %u / %u %s", FrameArena.used / unitsSize, FrameArena.size / unitsSize, unitsStr);
		UI_Label(ui, "- String Arena: %u / %u %s", StringArena.used / unitsSize, StringArena.size / unitsSize, unitsStr);
		UI_Label(ui, "- Data Arena: %u / %u %s", DataArena.used / unitsSize, DataArena.size / unitsSize, unitsStr);
	}

	if ( UI_Section(ui, "Scene") )
	{
		for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
		{
			Handle handle = *it;
			Entity &entity = GetEntity(scene, handle);
			if ( UI_Radio(ui, entity.name, EditorGetSelectedEntity(editor) == handle) )
			{
				EditorSelectEntity(editor, handle);
			}
		}
		if (UI_Button(ui, "Unselect"))
		{
			EditorUnselectAll(editor);
		}
		UI_BeginLayout(ui, UiLayoutHorizontal);
		if ( UI_Button(ui, "Load TXT") )
		{
			const EditorCommand command = { .type = EditorCommandLoadTxt };
			AddEditorCommand(editor, command);
		}
		if (UI_Button(ui, "Save TXT"))
		{
			const EditorCommand command = { .type = EditorCommandSaveTxt };
			AddEditorCommand(editor, command);
		}
		UI_EndLayout(ui);
		UI_BeginLayout(ui, UiLayoutHorizontal);
		if ( UI_Button(ui, "Load BIN") )
		{
			const EditorCommand command = { .type = EditorCommandLoadBin };
			AddEditorCommand(editor, command);
		}
		if ( UI_Button(ui, "Build BIN") )
		{
			const EditorCommand command = { .type = EditorCommandBuildBin };
			AddEditorCommand(editor, command);
		}
		UI_EndLayout(ui);
		if ( UI_Button(ui, "Clean") )
		{
			const EditorCommand command = { .type = EditorCommandClean };
			AddEditorCommand(editor, command);
		}
	}

	if ( UI_Section(ui, "Camera") )
	{
		if ( editor.cameraType == ProjectionPerspective )
		{
			UI_SeparatorLabel(ui, "Perspective options");
			UI_Checkbox(ui, "Orbit", &editor.cameraOrbit);
		}
	}

	if ( UI_Section(ui, "Pipelines") )
	{
		if ( UI_Button(ui, "Recompile shaders") )
		{
			CompileShaders();
		}

		for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
		{
			const char *pipelineName = pipelineDescs[i].desc.name;

			UI_BeginLayout(ui, UiLayoutHorizontal);
			if ( UI_Button(ui, "Reload") )
			{
				const EditorCommand command = {
					.type = EditorCommandReloadGraphicsPipeline,
					.pipelineIndex = i,
				};
				AddEditorCommand(editor, command);
			}
			UI_Label(ui, pipelineName);
			UI_EndLayout(ui);
		}
	}

	if ( UI_Section( ui, "Textures" ) )
	{
		static TextureH selectedHandle = {};
		constexpr u32 imagesPerRow = 3;
		UI_BeginLayout(ui, UiLayoutItemBrowser);
		for (u32 i = 0; i < gfx.textureHandles.handleCount; ++i)
		{
			const TextureH handle = GetHandleAt(gfx.textureHandles, i);
			const Texture &texture = GetTexture(gfx, handle);

			const UIWidgetFlags flags = selectedHandle == handle ? UIWidgetFlag_Outline : UIWidgetFlag_None;

			if (UI_Image(ui, texture.image, float2{32, 32}, flags))
			{
				selectedHandle = handle;
			}
		}
		UI_EndLayout(ui);

		if (IsValidHandle(gfx.textureHandles, selectedHandle))
		{
			if (UI_Button(ui, "Remove"))
			{
				const EditorCommand command = {
					.type = EditorCommandRemoveTexture,
					.textureH = selectedHandle,
				};
				AddEditorCommand(editor, command);
			}
		}
	}

	if ( UI_Section(ui, "Game") )
	{
		UI_BeginLayout(ui, UiLayoutHorizontal);

		// Start/Stop button
		if ( engine.game.state == GameStateStopped )
		{
			if ( UI_Button(ui, "Start") )
			{
				engine.game.state = GameStateStarting;
			}
		}
		else
		{
			if ( UI_Button(ui, "Stop") )
			{
				engine.game.state = GameStateStopping;
			}
		}

		// State label
		const char *labels[] = {"Stopped", "Starting", "Running", "Stopping"};
		CT_ASSERT(ARRAY_COUNT(labels) == GameStateCount);
		const char *label = labels[engine.game.state];
		UI_Label(ui, label);

		UI_EndLayout(ui);
	}

	UI_EndWindow(ui);
}


static void EditorUpdateUI_Outliner(Engine &engine)
{
	UI &ui = engine.ui;
	Editor &editor = engine.editor;
	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;
	Audio &audio = engine.audio;

	constexpr uint2 size = {200, 500};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {0,0});
	UI_SetNextWindowDefaultDisplacement(ui, {10, 50});

	UI_BeginWindow(ui, "Outliner", &editor.showOutliner);

	if ( UI_Section(ui, "Scene") )
	{
		for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
		{
			Handle handle = *it;
			const Entity &entity = GetEntity(scene, handle);

			if ( UI_Button(ui, entity.name) ) {
				EditorSelectEntity(editor, handle);
			}
		}
	}

	if ( UI_Section(ui, "Sprites") )
	{
		for (HandleIter it = BeginIter(scene.spriteHandles); it; it++)
		{
			Handle handle = *it;
			const Sprite &sprite = GetSprite(scene, handle);

			if ( UI_Button(ui, sprite.name) ) {
				EditorSelectSprite(editor, handle);
			}
		}
	}

	if ( UI_Section(ui, "Materials") )
	{
		for (HandleIter it = BeginIter(gfx.materialHandles); it; it++)
		{
			Handle handle = *it;
			const Material &material = GetMaterial(gfx, handle);

			if ( UI_Button(ui, material.name) ) {
				EditorSelectMaterial(editor, handle);
			}
		}
	}

	if ( UI_Section(ui, "Textures") )
	{
		for (HandleIter it = BeginIter(gfx.textureHandles); it; it++)
		{
			Handle handle = *it;
			const Texture &texture = GetTexture(gfx, handle);

			if ( UI_Button(ui, texture.name) ) {
				EditorSelectTexture(editor, handle);
			}
		}
	}

	if ( UI_Section(ui, "AudioClips") )
	{
		for (HandleIter it = BeginIter(audio.clipHandles); it; it++)
		{
			Handle handle = *it;
			const AudioClipDesc &desc = GetAudioClipDesc(audio, handle);

			if ( UI_Button(ui, desc.name) ) {
				EditorSelectAudioClip(editor, handle);
			}
		}
	}

	if ( UI_Section(ui, "MusicFiles") )
	{
		for (HandleIter it = BeginIter(audio.musicHandles); it; it++)
		{
			Handle handle = *it;
			const MusicFileDesc &desc = GetMusicFileDesc(audio, handle);

			if ( UI_Button(ui, desc.name) ) {
				EditorSelectMusic(editor, handle);
			}
		}
	}

	UI_EndWindow(ui);
}


static bool IsWavFile(const char *filename)
{
	const bool isWav = HasFileExtension(filename, "wav");
	return isWav;
}

static bool IsMusicFile(const char *filename)
{
	const bool isMusic =
		HasFileExtension(filename, "mod") ||
		HasFileExtension(filename, "s3m") ||
		HasFileExtension(filename, "xm");
	return isMusic;
}

static bool IsImgFile(const char *filename)
{
	const bool isImg =
		HasFileExtension(filename, "png") ||
		HasFileExtension(filename, "bmp") ||
		HasFileExtension(filename, "jpg");
	return isImg;
}

static void EditorUpdateUI_Assets(Engine &engine)
{
	UI &ui = GetUI(engine);
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;
	EditorInspector &inspector = editor.inspector;

	constexpr uint2 size = {500, 200};
	constexpr float2 displacement = {10, -10};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {0, 1});
	UI_SetNextWindowDefaultDisplacement(ui, displacement);

	UI_BeginWindow(ui, "Assets", &editor.showAssets);

	UI_BeginLayout(ui, UiLayoutItemBrowser);

	static char selectedName[MAX_PATH_LENGTH] = {};
	FilePath path;

	FileNode *node = editor.root;
	while (node)
	{
		const bool isImg = node->type == FileNodeType_Image;
		const bool isMusic = node->type == FileNodeType_Music;
		const bool isWav = node->type == FileNodeType_Sound;

		path = MakePath(AssetDir, node->filename);

		// For images, we get a snapshot
		ImageH iconImg = editor.iconImg;
		if ( isImg ) {
			SnapshotNode *snapshot = EditorGetOrCreateSnapshotNode(engine, path.str);
			if ( snapshot ) {
				iconImg = snapshot->imageH;
			}
		}

		const ImageH icon =
			isWav ? editor.iconWav :
			isMusic ? editor.iconMod :
			isImg ? iconImg :
			editor.iconAsset;

		const bool isSelected = inspector.selectedFile ? StrEq(path.str, inspector.selectedFile->filename) : false;
		const UIWidgetFlags flags = isSelected ? UIWidgetFlag_Outline : UIWidgetFlag_None;

		if ( UI_Image(ui, icon, float2{32,32}, flags) )
		{
			if ( isWav )
			{
				EditorSelectFileAudio(editor, node);
			}
			else if ( isMusic )
			{
				EditorSelectFileMusic(editor, node);
			}
			else if ( isImg )
			{
				EditorSelectFileImage(editor, node);
			}
			else
			{
				EditorUnselectAll(editor);
			}
		}

		UI_DragAndDropSource(ui, "FileNode", node, icon );

		node = node->next;
	}

	UI_EndLayout(ui);

	UI_EndWindow(ui);
}

static void EditorUpdateUI_Inspector(Engine &engine)
{
	UI &ui = engine.ui;
	Editor &editor = engine.editor;
	EditorInspector &inspector = editor.inspector;

	constexpr uint2 size = {200, 500};
	constexpr float2 displacement = {-10, 50};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowDefaultDisplacement(ui, displacement);
	UI_SetNextWindowAnchor(ui, {1, 0});

	static u32 audioSourceIndex = U32_MAX;

	const bool refresh =
		inspector.inspectedType != inspector.nextInspectedType ||
		inspector.selectedHandle != inspector.nextSelectedHandle ||
		inspector.selectedFile != inspector.nextSelectedFile;

	if ( refresh )
	{
		AudioStopAll(engine);

		if (inspector.inspectedType == EditorInspectedType_FileImage) {
			WaitDeviceIdle(engine.gfx.device);
			RemoveTexture(engine.gfx, inspector.selectedHandle);
		}
		if (inspector.inspectedType == EditorInspectedType_FileAudio) {
			audioSourceIndex = U32_MAX;
			RemoveAudioClip(engine, inspector.selectedHandle);
		}
		if (inspector.inspectedType == EditorInspectedType_FileMusic) {
			DestroyMusicFile(engine, inspector.selectedHandle);
		}
	}

	inspector.inspectedType = inspector.nextInspectedType;
	inspector.selectedHandle = inspector.nextSelectedHandle;
	inspector.selectedFile = inspector.nextSelectedFile;

	UI_BeginWindow(ui, "Inspector", &editor.showInspector);

	UI_SeparatorLabel(ui, "%s", EditorInspectedTypeName[inspector.inspectedType]);

	if (inspector.selectedFile &&
		inspector.inspectedType >= EditorInspectedType_FileBegin &&
		inspector.inspectedType <= EditorInspectedType_FileEnd)
	{
		const bool createFromFile = refresh;

		UI_Label(ui, "%s", inspector.selectedFile->filename);

		if (inspector.inspectedType == EditorInspectedType_FileImage)
		{
			if (createFromFile)
			{
				const TextureDesc desc = {
					.name = "inspected_image",
					.filename = inspector.selectedFile->filename,
					.mipmap = true,
					.flags = AssetFlag_Builtin,
				};
				inspector.selectedHandle = CreateTexture(engine.gfx, desc);
			}

			const ImageH imageH = GetTextureImage(engine.gfx, inspector.selectedHandle, engine.gfx.grayImageH);
			UI_Image(ui, imageH, float2{0,0}, UIWidgetFlag_Expand);

			if (UI_Section(ui, "Import settings"))
			{
				static char name[64] = {};
				UI_InputText(ui, "Name", name, ARRAY_COUNT(name));

				if (UI_Button(ui, "Import as Tileset"))
				{
					const TileAtlasDesc desc = {
						.imagePath = inspector.selectedFile->filename,
						.name = InternString(name),
						.tileSize = 32.0f,
					};
					CreateTileAtlas(engine, desc);
				}
				if ( UI_Button(ui, "Import as animation") )
				{
					const char *filename = inspector.selectedFile->filename;
					const char *basename = NameFromFilename(filename);
					const char *textureName = MakeName("tex_%s", basename);
					const char *spriteName = MakeName("spr_%s", basename);

					const TextureDesc textureDesc = {
						.name = textureName,
						.filename = filename,
						.mipmap = true,
					};
					GetOrCreateTexture(engine.gfx, textureDesc);

					const SpriteDesc desc = {
						.name = spriteName,
						.textureName = textureName,
						.framePos = {0, 0},
						.frameSize = {32, 32},
						.frameCount = 4,
						.fps = 4,
						.loop = true,
					};
					GetOrCreateSprite(engine, desc);
				}
			}
		}
		else if (inspector.inspectedType == EditorInspectedType_FileAudio)
		{
			if (createFromFile)
			{
				const AudioClipDesc desc = {
					.name = "inspected_audio_clip",
					.filename = inspector.selectedFile->filename,
					.flags = AssetFlag_Builtin,
				};
				inspector.selectedHandle = CreateAudioClip(engine, desc);
			}

			if (inspector.selectedHandle != InvalidHandle)
			{
				if (UI_Button(ui, "Play")) {
					audioSourceIndex = PlayAudioClip(engine, inspector.selectedHandle);
				}
				if (UI_Button(ui, "Stop")) {
					StopAudioSource(engine, audioSourceIndex);
					audioSourceIndex = U32_MAX;
				}
			}
		}
		else if (inspector.inspectedType == EditorInspectedType_FileMusic)
		{
			if (createFromFile)
			{
				const MusicFileDesc desc = {
					.name = "inspected_music_file",
					.filename = inspector.selectedFile->filename,
					//.flags = AssetFlag_Builtin,
				};
				inspector.selectedHandle = CreateMusicFile(engine, desc);
			}

			if (inspector.selectedHandle != InvalidHandle)
			{
				if (UI_Button(ui, "Play")) {
					MusicPlay(engine, inspector.selectedHandle);
				}
				if (UI_Button(ui, "Stop")) {
					MusicStop(engine);
				}
			}
		}
	}
	else
	{
		if(inspector.inspectedType == EditorInspectedType_Entity)
		{
			Entity &entity = GetEntity(engine.scene, engine.editor.inspector.selectedHandle);

			static char name[64];
			StrCopy(name, entity.name);
			UI_InputText(ui, "Name", name, ARRAY_COUNT(name));
			entity.name = InternString(name);

			UI_InputFloat3(ui, "Position", &entity.position);
			UI_InputFloat(ui, "X", &entity.position.x);
			UI_InputFloat(ui, "Scale", &entity.scale);
			UI_Checkbox(ui, "Visible", &entity.visible);
		}
		else if (inspector.inspectedType == EditorInspectedType_Material)
		{
		}
		else if (inspector.inspectedType == EditorInspectedType_Texture)
		{
			if (inspector.selectedHandle != InvalidHandle)
			{
				const ImageH imageH = GetTextureImage(engine.gfx, inspector.selectedHandle, engine.gfx.grayImageH);
				UI_Image(ui, imageH, float2{0,0}, UIWidgetFlag_Expand);
			}
		}
		else if (inspector.inspectedType == EditorInspectedType_Audio)
		{
			if (inspector.selectedHandle != InvalidHandle)
			{
				if (UI_Button(ui, "Play")) {
					audioSourceIndex = PlayAudioClip(engine, inspector.selectedHandle);
				}
				if (UI_Button(ui, "Stop")) {
					StopAudioSource(engine, audioSourceIndex);
					audioSourceIndex = U32_MAX;
				}
			}
		}
		else if (inspector.inspectedType == EditorInspectedType_Music)
		{
			if (inspector.selectedHandle != InvalidHandle)
			{
				if (UI_Button(ui, "Play")) {
					MusicPlay(engine, inspector.selectedHandle);
				}
				if (UI_Button(ui, "Stop")) {
					MusicStop(engine);
				}
			}
		}
		else if (inspector.inspectedType == EditorInspectedType_Sprite)
		{
			if (inspector.selectedHandle != InvalidHandle)
			{
				Sprite &sprite = GetSprite(engine.scene, inspector.selectedHandle);

				static char name[64];
				StrCopy(name, sprite.name);
				UI_InputText(ui, "Name", name, ARRAY_COUNT(name));
				sprite.name = InternString(name);

				if (IsValidHandle(engine.gfx.textureHandles, sprite.textureH))
				{
					const Texture &texture = GetTexture(engine.gfx, sprite.textureH);
					UI_Label(ui, "Texture: %s", texture.name);
					UI_Image(ui, texture.image, float2{0, 0}, UIWidgetFlag_Expand);
				}

				i32 framePosX  = (i32)sprite.framePixelPos.x;
				i32 framePosY  = (i32)sprite.framePixelPos.y;
				i32 frameSizeX = (i32)sprite.framePixelSize.x;
				i32 frameSizeY = (i32)sprite.framePixelSize.y;
				i32 frameCount = (i32)sprite.frameCount;
				i32 fps        = (i32)sprite.fps;

				const uint2 oldFramePixelPos  = sprite.framePixelPos;
				const uint2 oldFramePixelSize = sprite.framePixelSize;
				const u32   oldFrameCount     = sprite.frameCount;
				const u32   oldFps            = sprite.fps;

				UI_InputInt(ui, "Frame Pos X",  &framePosX);
				UI_InputInt(ui, "Frame Pos Y",  &framePosY);
				UI_InputInt(ui, "Frame Size X", &frameSizeX);
				UI_InputInt(ui, "Frame Size Y", &frameSizeY);
				UI_InputInt(ui, "Frame Count",  &frameCount);
				UI_InputInt(ui, "FPS",          &fps);
				UI_Checkbox(ui, "Loop",         &sprite.loop);

				sprite.framePixelPos  = { (u32)Max(0, framePosX),  (u32)Max(0, framePosY)  };
				sprite.framePixelSize = { (u32)Max(0, frameSizeX), (u32)Max(0, frameSizeY) };
				sprite.frameCount     = (u32)Max(0, frameCount);
				sprite.fps            = (u32)Max(0, fps);
			}
		}
	}

	UI_EndWindow(ui);

	inspector.nextInspectedType = inspector.inspectedType;
	inspector.nextSelectedHandle = inspector.selectedHandle;
	inspector.nextSelectedFile = inspector.selectedFile;
}

static void EditorUpdateUI_Tilesets(Engine &engine)
{
	UI &ui = GetUI(engine);
	Graphics &gfx = engine.gfx;
	Editor &editor = engine.editor;

	UI_BeginWindow(ui, "Tilesets", &editor.showTilesets);

	UI_SeparatorLabel(ui, "TileAtlas");

	if ( IsTileAtlasValid(engine) )
	{
		const TileAtlas &tileAtlas = GetTileAtlas(engine);
		const i32 tileCountPerSide = Floor(tileAtlas.size / tileAtlas.tileSize);
		const f32 normalizedTileSize = tileAtlas.tileSize / tileAtlas.size;
		const Texture &texture = GetTexture(gfx, tileAtlas.textureH);
		if (UI_Image(ui, texture.image, float2{0, 0}, UIWidgetFlag_Expand))
		{
			const float2 normalizedCoord = UI_LastMouseClickPosInWidgetNormalized(ui);
			const int2 tileCoord = {
				.x = Clamp((i32) (normalizedCoord.x / normalizedTileSize), 0, tileCountPerSide - 1),
				.y = Clamp((i32) (normalizedCoord.y / normalizedTileSize), 0, tileCountPerSide - 1),
			};

			editor.tilesets.tile.used = editor.tilesets.selectedTool == EditorDrawTool_Draw ? 1 : 0;
			editor.tilesets.tile.atlasId = 0;
			editor.tilesets.tile.tileId = tileCoord.y * tileCountPerSide + tileCoord.x;
			//LOG(Debug, "Pos %u %u %u\n", tileCoord.x, tileCoord.y, editor.tilesets.tile.tileId);
		}
	}
	else
	{
		UI_Label(ui, "No tile atlas available");
	}

	UI_SeparatorLabel(ui, "Action");

	EditorTilesets &tilesets = editor.tilesets;
	const char *radioOptionStrs[] { "Draw", "Erase" };
	const EditorDrawTool radioOptions[] = { EditorDrawTool_Draw, EditorDrawTool_Erase };
	for (u32 i = 0; i < ARRAY_COUNT(radioOptionStrs); ++i) {
		if ( UI_Radio(ui, radioOptionStrs[i], tilesets.selectedTool == radioOptions[i]) ) {
			tilesets.selectedTool = radioOptions[i];
		}
	}

	UI_EndWindow(ui);
}

static void EditorUpdateUI_About(Engine &engine)
{
	UI &ui = engine.ui;
	Editor &editor = engine.editor;

	UI_SetNextWindowModal(ui);
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});
	UI_SetNextWindowSize(ui, uint2{ 512, 350 });
	UI_SetNextWindowDefaultDisplacement(ui, {0, 0});

	UI_BeginWindow(ui, "About", nullptr, UIWindowFlag_Border | UIWindowFlag_Background);

	UI_RaiseWindow(ui);
	UI_FocusWindow(ui);

	UI_Label(ui, "");
	UI_Image(ui, editor.iluLogo, float2{256, 256}, UIWidgetFlag_Centered);
	UI_Label(ui, "                          ILU engine");
	UI_Label(ui, "                     (by Jesus Diaz Garcia)");

	UI_EndWindow(ui);

	static bool wasShown = false;
	if ( UI_IsMouseClickWithAnyButton(ui) && wasShown ) {
		engine.editor.showAbout = false;
	}
	wasShown = engine.editor.showAbout;
}

static void EditorUpdateUI_DragAndDropLost(Engine &engine)
{
	UI &ui = GetUI(engine);
	if ( UI_DragAndDropTargetLost(ui, "FileNode") )
	{
		FileNode *node = (FileNode*) UI_DragAndDropPayload(ui);
		if (node->type == FileNodeType_Image)
		{
			LOG(Info, "Image asset dropped: %s\n", node->filename);

			const Mouse &mouse = sPlatform->window->mouse;
			const int2 mousePos = { mouse.x, mouse.y };

			if (EditorMode2D(engine.editor))
			{
				const Camera &camera = engine.editor.camera[ProjectionOrthographic];
				const float2 worldPos = Floor(GetWorld2DCoord(engine, camera, mousePos));
				LOG(Info, "World x: %f, y: %f\n", worldPos.x, worldPos.y);

				const char *basename = NameFromFilename(node->filename);
				const char *texname = MakeName("tex_%s", basename);
				const char *matname = MakeName("mat_%s", basename);
				const char *spriteName = MakeName("spr_%s", basename);

				const TextureDesc textureDesc = {
					.name = texname,
					.filename = node->filename,
					.mipmap = true,
				};
				TextureH textureH = GetOrCreateTexture(engine.gfx, textureDesc);

				const SpriteDesc spriteDesc = {
					.name = spriteName,
					.textureName = texname,
				};
				SpriteH spriteH = GetOrCreateSprite(engine, spriteDesc);

				const Texture &texture = GetTexture(engine.gfx, textureH);
				constexpr float pixelsPerGridTile = 32;

				const EntityDesc entityDesc = {
					.name = "entity",
					.spriteName = spriteName,
					.pos = Float3(worldPos, 0.0),
					.scale = 1.0f,
				};

				CreateEntity(engine, entityDesc);
			}
			else
			{
				LOG(Debug, "Drag and Drop not implemented in 3D mode.\n");
			}
		}
		else if (node->type == FileNodeType_Sound)
		{
			LOG(Info, "AudioClip asset dropped: %s\n", node->filename);

			const char *basename = NameFromFilename(node->filename);
			const char *clipname = MakeName("snd_%s", basename);

			const AudioClipDesc desc = {
				.name = clipname,
				.filename = node->filename,
				//.flags = AssetFlag_Builtin,
			};
			Handle clipHandle = GetOrCreateAudioClip(engine, desc);
		}
		else if (node->type == FileNodeType_Music)
		{
			LOG(Info, "MusicFile asset dropped: %s\n", node->filename);

			const char *basename = NameFromFilename(node->filename);
			const char *modname = MakeName("mod_%s", basename);

			const MusicFileDesc desc = {
				.name = modname,
				.filename = node->filename,
				//.flags = AssetFlag_Builtin,
			};
			Handle clipHandle = GetOrCreateMusicFile(engine, desc);
		}
	}
}

enum EditorFileDialogMode
{
	EditorFileDialog_LoadFile,
	EditorFileDialog_SaveFile,
	EditorFileDialogModeCount,
};
struct EditorFileDialogStrings
{
	const char *caption;
	const char *button;
};
static const EditorFileDialogStrings EditorFileDialogStringsArray[] = {
	{ .caption = "Load file", .button = "Load"},
	{ .caption = "Save file", .button = "Save"},
};
CT_ASSERT( ARRAY_COUNT(EditorFileDialogStringsArray) == EditorFileDialogModeCount );

static bool EditorFileDialog(Engine &engine, EditorFileDialogMode mode, const char *extension, bool *isOpen, FilePath *filePath)
{
	bool ret = false;

	UI &ui = GetUI(engine);
	Editor &editor = engine.editor;

	static bool wasOpen = false;
	static char filename[MAX_PATH_LENGTH] = {};

	const bool justOpened = !wasOpen;
	if ( justOpened ) {
		StrCopy(filename, "");
	}

	const char *caption = EditorFileDialogStringsArray[mode].caption;
	const char *button = EditorFileDialogStringsArray[mode].button;

	UI_SetNextWindowModal(ui);
	UI_SetNextWindowDefaultSize(ui, {400, 300});
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});
	UI_BeginWindow(ui, caption, isOpen);

	Dir dir;
	if ( OpenDir(dir, AssetDir) )
	{
		DirEntry entry;
		while ( ReadDir(dir, entry) )
		{
			if ( HasFileExtension(entry.name, extension) )
			{
				if ( UI_Radio(ui, entry.name, StrEq(entry.name, filename)) )
				{
					StrCopy(filename, entry.name);
				}
			}
		}

		CloseDir(dir);
	}

	if ( mode == EditorFileDialog_SaveFile )
	{
		UI_InputText(ui, "File name", filename, ARRAY_COUNT(filename));
	}

	if ( UI_Button(ui, button) && !StrEq(filename, "") ) {
		*filePath = MakePath(AssetDir, filename);
		*isOpen = false;
		ret = true;
	}

	UI_EndWindow(ui);

	wasOpen = *isOpen;

	return ret;
}

static void EditorUpdateUI(Engine &engine)
{
	UI &ui = GetUI(engine);
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	EditorUpdateUI_MenuBar(engine);
	EditorUpdateUI_ToolBar(engine);

	if ( editor.showLoadScene )
	{
		static FilePath filePath = {};
		if ( EditorFileDialog(engine, EditorFileDialog_LoadFile, "txt", &editor.showLoadScene, &filePath) )
		{
			const EditorCommand command = { .type = EditorCommandLoadTxt, .filepath = filePath.str };
			AddEditorCommand(editor, command);
		}
	}

	static FilePath saveSceneFilepath = {};
	static bool saveScene = false;
	if ( editor.showSaveScene )
	{
		if ( EditorFileDialog(engine, EditorFileDialog_SaveFile, "txt", &editor.showSaveScene, &saveSceneFilepath) )
		{
			saveScene = true;
		}
	}
	if ( saveScene )
	{
		const char *buttons[] = { "Yes", "No", nullptr };
		u32 result = 0;
		if ( ExistsFile(saveSceneFilepath.str) )
		{
			if ( UI_MessageBox(ui, "Save scene", "File already exists. Overwrite contents?", buttons, &result) )
			{
				if ( result == 0 ) {
					EditorCommand command = { .type = EditorCommandSaveTxt, .filepath = saveSceneFilepath.str };
					AddEditorCommand(editor, command);
				}
				saveScene = false;
			}
		}
		else
		{
			EditorCommand command = { .type = EditorCommandSaveTxt, .filepath = saveSceneFilepath.str };
			AddEditorCommand(editor, command);

			saveScene = false;
		}
	}

	if ( editor.showOutliner )
	{
		EditorUpdateUI_Outliner(engine);
	}

	if ( editor.showAssets )
	{
		EditorUpdateUI_Assets(engine);
	}

	if ( editor.showInspector )
	{
		EditorUpdateUI_Inspector(engine);
	}

	if ( editor.showTilesets )
	{
		EditorUpdateUI_Tilesets(engine);
	}

	if ( editor.showDebugUI )
	{
		EditorUpdateUI_DebugUI(engine);
	}

	if ( editor.showAbout )
	{
		EditorUpdateUI_About(engine);
	}

	EditorUpdateUI_DragAndDropLost(engine);
}

#if PLATFORM_ANDROID
static bool GetOrientationTouchId(const Window &window, u32 *touchId)
{
	ASSERT( touchId != 0 );
	for (u32 i = 0; i < ARRAY_COUNT(window.touches); ++i)
	{
		if (window.touches[i].state == TOUCH_STATE_PRESSED &&
			window.touches[i].x0 > window.width/2 )
		{
			*touchId = i;
			return true;
		}
	}
	return false;
}

static bool GetMovementTouchId(const Window &window, u32 *touchId)
{
	ASSERT( touchId != 0 );
	for (u32 i = 0; i < ARRAY_COUNT(window.touches); ++i)
	{
		if (window.touches[i].state == TOUCH_STATE_PRESSED &&
			window.touches[i].x0 <= window.width/2 )
		{
			*touchId = i;
			return true;
		}
	}
	return false;
}
#endif

static void EditorUpdateCamera3DOrbit(Camera &camera, float deltaSeconds)
{
	static f32 yaw = 0;
	yaw += 0.25 * Pi * deltaSeconds;

	const f32 pitch = -0.45f;
	const float2 angles = {yaw, pitch};
	const float3 forward = ForwardDirectionFromAngles(angles);
	const float3 position = -3.0f * forward;

	camera.position = position;
	camera.orientation = angles;
}

static void EditorUpdateCamera3D(const Window &window, Camera &camera, float deltaSeconds, bool handleInput)
{
	float3 dir = { 0, 0, 0 };

	if ( handleInput )
	{
		// Camera rotation
		f32 deltaYaw = 0.0f;
		f32 deltaPitch = 0.0f;
#if PLATFORM_ANDROID
		u32 touchId;
		if ( GetOrientationTouchId(window, &touchId) )
		{
			deltaYaw = - window.touches[touchId].dx * ToRadians * 0.2f;
			deltaPitch = - window.touches[touchId].dy * ToRadians * 0.2f;
		}
#else
		if (MouseButtonPressed(window.mouse, MOUSE_BUTTON_LEFT)) {
			deltaYaw = - window.mouse.dx * ToRadians * 0.2f;
			deltaPitch = - window.mouse.dy * ToRadians * 0.2f;
		}
#endif
		float2 angles = camera.orientation;
		angles.x = angles.x + deltaYaw;
		angles.y = Clamp(angles.y + deltaPitch, -Pi * 0.49, Pi * 0.49);

		camera.orientation = angles;

		// Movement direction
#if PLATFORM_ANDROID
		if ( GetMovementTouchId(window, &touchId) )
		{
			const float3 forward = ForwardDirectionFromAngles(angles);
			const float3 right = RightDirectionFromAngles(angles);
			const float scaleForward = -window.touches[touchId].dy;
			const float scaleRight = window.touches[touchId].dx;
			dir = Add(Mul(forward, scaleForward), Mul(right, scaleRight));
		}
#else
		if ( KeyPressed(window.keyboard, K_W) ) { dir = Add(dir, ForwardDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, K_S) ) { dir = Add(dir, Negate( ForwardDirectionFromAngles(angles) )); }
		if ( KeyPressed(window.keyboard, K_D) ) { dir = Add(dir, RightDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, K_A) ) { dir = Add(dir, Negate( RightDirectionFromAngles(angles) )); }
#endif
		dir = NormalizeIfNotZero(dir);
	}

	// Accelerated translation
	static constexpr f32 MAX_SPEED = 100.0f;
	static constexpr f32 ACCELERATION = 50.0f;
	static float3 speed = { 0, 0, 0 };
	const float3 speed0 = speed;

	// Apply acceleration, then limit speed
	speed = Add(speed, Mul(dir, ACCELERATION * deltaSeconds));
	speed = Length(speed) > MAX_SPEED ?  Mul( Normalize(speed), MAX_SPEED) : speed;

	// Based on speed, translate camera position
	const float3 translation = Add( Mul(speed0, deltaSeconds), Mul(speed, 0.5f * deltaSeconds) );
	camera.position = Add(camera.position, translation);

	// Apply deceleration
	speed = Mul(speed, 0.9);
}

static void EditorUpdateInteraction2D(Engine &engine, const Window &window, const Gamepad &gamepad, Camera &camera, float deltaSeconds, bool handleInput)
{
	Editor &editor = engine.editor;
	UI &ui = engine.ui;
	const Mouse &mouse = window.mouse;

	Handle selectedEntity = EditorGetSelectedEntity(editor);

	// Object transformations
	if ( handleInput && selectedEntity != InvalidHandle )
	{
		Entity &entity = GetEntity(engine.scene, selectedEntity);

		const int2 mousePos = {mouse.x, mouse.y};
		const float2 mouseWorldPos = GetWorld2DCoord(engine, camera, mousePos);

		static float2 initialWorldOffset = {};
		static float2 initialWorldPos = {};

		static bool wasTranslating = false;

		if ( editor.isTranslating )
		{
			if (!wasTranslating) {
				initialWorldPos = float2{entity.position.x, entity.position.y};
				initialWorldOffset = Floor(initialWorldPos) - Floor(mouseWorldPos);
			} else if (MouseButtonPress(window.mouse, MOUSE_BUTTON_RIGHT)) {
				entity.position = Float3(initialWorldPos, entity.position.z);
				editor.isTranslating = false;
			} else if (MouseButtonRelease(window.mouse, MOUSE_BUTTON_LEFT)) {
				editor.isTranslating = false;
			} else {
				const float2 finalPos = Floor(mouseWorldPos) + initialWorldOffset;
				entity.position = Float3(finalPos, entity.position.z);
			}
		}

		wasTranslating = editor.isTranslating;

		static bool isScaling = false;
		static f32 initialScale = 0.0;
		if (KeyPress(window.keyboard, K_E)) {
			isScaling = true;
			initialWorldOffset = float2{entity.position.x, entity.position.y} - mouseWorldPos;
			initialScale = entity.scale;
		} else if (isScaling && MouseButtonPress(window.mouse, MOUSE_BUTTON_RIGHT)) {
			entity.scale = initialScale;
			isScaling = false;
		} else if (isScaling && MouseButtonPress(window.mouse, MOUSE_BUTTON_LEFT)) {
			isScaling = false;
		} else if (isScaling) {
			const float2 worldOffset = float2{entity.position.x, entity.position.y} - mouseWorldPos;
			const f32 initialOffsetLen = Length(initialWorldOffset);
			const f32 offsetLen = Length(worldOffset);
			entity.scale = initialScale * offsetLen / initialOffsetLen;
		}

		if (KeyPress(window.keyboard, K_DELETE))
		{
			RemoveEntity(engine, selectedEntity);
			EditorUnselectAll(engine.editor);
		}

		if (KeyPressed(window.keyboard, K_SHIFT))
		{
			handleInput = false; // K_SHIFT is for commands, so abort camera translation

			if (KeyPress(window.keyboard, K_D))
			{
				DuplicateEntity(engine, selectedEntity);
			}
		}
	}

	// Camera navigation
	if ( handleInput )
	{
		static int2 clickPos = {};
		static float3 cameraPositionOnClick = camera.position;
		if (MouseButtonPress(mouse, MOUSE_BUTTON_MIDDLE))
		{
			clickPos = {mouse.x, mouse.y};
			cameraPositionOnClick = camera.position;
		}
		if (MouseButtonPressed(mouse, MOUSE_BUTTON_MIDDLE))
		{
			const f32 windowHeight = window.height;
			const int2 mousePos = {mouse.x, mouse.y};
			const int2 dragPixels = mousePos - clickPos;
			const float2 dragNorm = {dragPixels.x/windowHeight, dragPixels.y/windowHeight};
			const float2 dragScaled = 2.0f * camera.height * dragNorm;
			camera.position.x = cameraPositionOnClick.x - dragScaled.x;
			camera.position.y = cameraPositionOnClick.y + dragScaled.y;
		}
	}

	if ( ui.hoveredWindow == nullptr )
	{
		if (mouse.wy != 0.0)
		{
			const float ar = (f32) window.width / window.height;
			const float heightPrev = camera.height;
			camera.height = Max(2.0f, camera.height + mouse.wy);
			const float heightDiff = heightPrev - camera.height;
			const float widthDiff = ar * heightDiff;
			const float xScale = 2.0f * ( (f32) mouse.x / window.width ) - 1.0f;
			const float yScale = 1.0 - 2.0f * ( (f32) mouse.y / window.height );
			camera.position.x += widthDiff * xScale;
			camera.position.y += heightDiff * yScale;
		}
	}

	float3 dir = { 0, 0, 0 };

	// Interaction with keyboard / gamepad
	if ( handleInput )
	{
		f32 dx = 0.0f;
		f32 dy = 0.0f;

		// Keyboard
		dy += KeyPressed(window.keyboard, K_W) ? 1.0f : 0.0f;
		dx += KeyPressed(window.keyboard, K_A) ? -1.0f : 0.0f;
		dy += KeyPressed(window.keyboard, K_S) ? -1.0f : 0.0f;
		dx += KeyPressed(window.keyboard, K_D) ? 1.0f : 0.0f;

		// Gamepad
		dx += gamepad.leftAxis.x;
		dy += gamepad.leftAxis.y;

		const float3 upVector = {0, 1, 0};
		const float3 rightVector = {1, 0, 0};
		dir = NormalizeIfNotZero(dx * rightVector + dy * upVector);
	}

	// Accelerated translation
	static constexpr f32 MAX_SPEED = 25.0f;
	static constexpr f32 ACCELERATION = 50.0f;
	static float3 speed = { 0, 0, 0 };
	const float3 speed0 = speed;

	// Apply acceleration, then limit speed
	speed = Add(speed, Mul(dir, ACCELERATION * deltaSeconds));
	speed = Length(speed) > MAX_SPEED ?  Mul( Normalize(speed), MAX_SPEED) : speed;

	// Based on speed, translate camera position
	const float3 translation = Add( Mul(speed0, deltaSeconds), Mul(speed, 0.5f * deltaSeconds) );
	camera.position = Add(camera.position, translation);

	// Apply deceleration
	if ( Length2(dir) == 0.0f )
	{
		speed = Mul(speed, 0.9);
	}
}

static void EditorBeginSceneEditing(Engine &engine, const Mouse &mouse, bool handleInput)
{
	Editor &editor = engine.editor;

	if ( handleInput )
	{
		if (MouseButtonPress(mouse, MOUSE_BUTTON_LEFT))
		{
			editor.selectEntity = true;
		}
		else if ( editor.selectEntity )
		{
			editor.selectEntity = false;

			WaitDeviceIdle(engine.gfx.device);
			Handle entityHandle = *(Handle*)GetBufferPtr(engine.gfx.device, engine.gfx.selectionBufferH);

			if (entityHandle != InvalidHandle)
			{
				editor.isTranslating = true;
				EditorSelectEntity(editor, entityHandle);
			}
			else
			{
				EditorUnselectAll(editor);
			}
		}

		if (EditorMode2D(engine.editor) && MouseButtonPressed(mouse, MOUSE_BUTTON_LEFT))
		{
			const Camera &camera = editor.camera[ProjectionOrthographic];
			const int2 mousePos = { mouse.x, mouse.y };
			const int2 setGridTileCoord = GetGridTileCoord(engine, camera, mousePos);
			const Tile tile = editor.tilesets.tile;
			SetGridTileAtCoord(engine, tile, setGridTileCoord);
		}
	}
}

static void EditorProcessCommands(Engine &engine, Arena scratch)
{
	Editor &editor = engine.editor;
	Graphics &gfx = engine.gfx;

	if ( editor.commandCount > 0 )
	{
		WaitDeviceIdle(gfx.device);

		for (u32 i = 0; i < editor.commandCount; ++i)
		{
			const EditorCommand &command = editor.commands[i];

			switch (command.type)
			{
				case EditorCommandReloadGraphicsPipeline:
				{
					const u32 pipelineIndex = command.pipelineIndex;
					const ShaderAndPipelineDesc &desc = pipelineDescs[pipelineIndex];
					const u32 vertexShaderIndex = FindShaderSourceDescIndex(desc.vsName);
					const u32 fragmentShaderIndex = FindShaderSourceDescIndex(desc.fsName);
					CompileShader(vertexShaderIndex);
					CompileShader(fragmentShaderIndex);
					CompileGraphicsPipeline(engine, scratch, pipelineIndex);
					break;
				}
				case EditorCommandRemoveTexture:
				{
					RemoveTexture(engine.gfx, command.textureH);
					break;
				}
				case EditorCommandLoadTxt:
				{
					LoadSceneFromTxt(engine, command.filepath);
					break;
				}
				case EditorCommandSaveTxt:
				{
					SaveSceneToTxt(engine, command.filepath);
					break;
				}
				case EditorCommandLoadBin:
				{
					LoadSceneFromBin(engine);
					break;
				}
				case EditorCommandBuildBin:
				{
					const FilePath assetsFilepath = MakePath(DataDir, "assets.dat");
					const FilePath descriptorsFilepath = MakePath(AssetDir, "assets.txt");
					BuildAssetsFromTxt(engine, descriptorsFilepath.str, assetsFilepath.str);
					break;
				}
				case EditorCommandClean:
				{
					EditorUnselectAll(engine.editor);
					CleanScene(engine);
					break;
				}

				default:;
			}
		}

		editor.commandCount = 0;

		LinkHandles(gfx);
	}
}

static FileNode *GetFreeFileNode(Editor &editor)
{
	ASSERT(editor.freeNodes != nullptr);
	FileNode *res = editor.freeNodes;
	editor.freeNodes = res->next;
	if ( editor.freeNodes) { editor.freeNodes->prev = nullptr; }
	res->type = FileNodeType_COUNT,
	res->filename = nullptr;
	res->next = nullptr;
	res->prev = nullptr;
	res->child = nullptr;
	return res;
}

static void FreeFileNode(Editor &editor, FileNode *node)
{
	FileNode *first = editor.freeNodes;
	if (first) { first->prev = node; }
	node->prev = nullptr;
	node->next = first;
	editor.freeNodes = node;
}

static FileNode * InsertFileNode(FileNode *node, FileNode *first)
{
	if ( first ) {
		first->prev = node;
	}
	node->next = first;
	node->prev = nullptr;
	return node;
}

void EditorInitialize(Engine &engine)
{
	Editor &editor = engine.editor;

	editor.showOutliner = true;
	editor.showAssets = true;
	editor.showInspector = true;
	editor.showTilesets = false;
	editor.showDebugUI = false;
	editor.showGrid = true;
	editor.showAbout = false;

	editor.camera[ProjectionPerspective].projectionType = ProjectionPerspective;
	editor.camera[ProjectionPerspective].position = {0, 1, 2};
	editor.camera[ProjectionPerspective].orientation = {0, -0.45f};

	editor.camera[ProjectionOrthographic].projectionType = ProjectionOrthographic;
	editor.camera[ProjectionOrthographic].position = {0, 0, -5};
	editor.camera[ProjectionOrthographic].orientation = {};
	editor.camera[ProjectionOrthographic].height = 8.0;

	editor.cameraType = ProjectionOrthographic;
	engine.gfx.activeCamera = &editor.camera[ProjectionOrthographic];

	editor.iconAsset = EditorLoadIcon(engine, "editor/file_32x32.png", "file_32x32");
	editor.iconWav = EditorLoadIcon(engine, "editor/wav_32x32.png", "wav_32x32");
	editor.iconMod = EditorLoadIcon(engine, "editor/mod_32x32.png", "mod_32x32");
	editor.iconImg = EditorLoadIcon(engine, "editor/img_32x32.png", "img_32x32");
	editor.iluLogo = EditorLoadIcon(engine, "editor/ilu_logo.png", "ilu_logo");

	EditorUnselectAll(editor);

	// Make a liked list of free file nodes
	constexpr u32 maxFileNodes = 4092;
	editor.freeNodes = PushZeroArray(GlobalArena, FileNode, maxFileNodes);
	for (u32 i = 0; i < maxFileNodes; ++i) {
		if ( i < maxFileNodes - 1) {
			editor.freeNodes[i].next = &editor.freeNodes[i+1];
		}
		if ( i > 0 ) {
			editor.freeNodes[i].prev = &editor.freeNodes[i-1];
		}
	}
	editor.root = nullptr;

	// Read the asset directory contents into file nodes
	Dir dir;
	if ( OpenDir(dir, AssetDir) )
	{
		DirEntry entry;
		while ( ReadDir(dir, entry) )
		{
			FileNode *node = GetFreeFileNode(editor);
			node->filename = InternString(entry.name);
			node->type = IsImgFile(entry.name) ? FileNodeType_Image :
						IsMusicFile(entry.name) ? FileNodeType_Music :
						IsWavFile(entry.name) ? FileNodeType_Sound:
						FileNodeType_COUNT;
			editor.root = InsertFileNode(node, editor.root);
		}

		CloseDir(dir);
	}
}

void EditorUpdate(Engine &engine)
{
	Plat &platform = *sPlatform;
	Graphics &gfx = engine.gfx;

	if ( KeyPress(platform.window->keyboard, K_F5) )
	{
		if ( engine.game.state == GameStateStopped ) {
			engine.game.state = GameStateStarting;
		}
	}
	else if ( KeyPress(platform.window->keyboard, K_ESCAPE) )
	{
		if ( engine.game.state == GameStateRunning ) {
			engine.game.state = GameStateStopping;
			EditorSetCamera(engine.editor);
		}
	}

	if ( engine.game.state != GameStateStopped )
	{
		return;
	}

	EditorUpdateUI(engine);

	const bool handleInput = !engine.ui.wantsInput;

	if (EditorMode3D(engine.editor))
	{
		if (engine.editor.cameraOrbit)
		{
			EditorUpdateCamera3DOrbit(engine.editor.camera[ProjectionPerspective], gfx.deltaSeconds);
		}
		else
		{
			EditorUpdateCamera3D(*platform.window, engine.editor.camera[ProjectionPerspective], gfx.deltaSeconds, handleInput);
		}
	}
	else
	{
		EditorUpdateInteraction2D(engine, *platform.window, *platform.gamepad, engine.editor.camera[ProjectionOrthographic], gfx.deltaSeconds, handleInput);
	}

	EditorBeginSceneEditing(engine, platform.window->mouse, handleInput);
}

void EditorRender(Engine &engine, CommandList &commandList)
{
	Editor &editor = engine.editor;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	const u32 frameIndex = gfx.device.frameIndex;
	const BufferH vertexBuffer = gfx.globalVertexArena.buffer;
	const BufferH indexBuffer = gfx.globalIndexArena.buffer;

	if ( editor.selectEntity )
	{
		BeginDebugGroup(commandList, "Entity selection", ColorBlack);

		{ // Draw entity IDs
			SetClearColorU32(commandList, 0, InvalidHandle.num);

			BeginRenderPass(commandList, gfx.renderTargets.idFramebuffer );

			const uint2 framebufferSize = GetFramebufferSize(gfx.renderTargets.idFramebuffer);
			SetViewportAndScissor(commandList, framebufferSize);

			SetPipeline(commandList, gfx.modelIdPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
			{
				Handle handle = *it;
				const Entity &entity = GetEntity(scene, handle);

				if ( !entity.visible || entity.culled ) continue;
				if ( entity.materialH == InvalidHandle ) continue;

				// Draw!!!
				const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
				const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
				const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, handle.num);
			}

			{ // Sprite entities
				const uint32_t spriteIndexCount = gfx.spriteIndices.size / sizeof(Index);
				const uint32_t spriteFirstIndex = gfx.spriteIndices.offset / sizeof(Index);
				const int32_t spriteFirstVertex = gfx.spriteVertices.offset / sizeof(Vertex);

				SetPipeline(commandList, gfx.spriteIdPipelineH);

				for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
				{
					Handle handle = *it;
					const Entity &entity = GetEntity(scene, handle);

					if ( !entity.visible || entity.culled ) continue;
					if ( !IsValidHandle(scene.spriteHandles, entity.spriteH) ) continue;

					DrawIndexed(commandList, spriteIndexCount, spriteFirstIndex, spriteFirstVertex, handle.num);
				}
			}

			EndRenderPass(commandList);
		}

		{ // Write entity ID under mouse cursor into selection buffer
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.computeSelectH);

			SetPipeline(commandList, gfx.computeSelectH);

			const BindGroupDesc bindGroupDesc = {
				.layout = pipeline.layout.bindGroupLayouts[3],
				.bindings = {
					{ .index = 0, .bufferView = gfx.selectionBufferViewH },
					{ .index = 1, .image = gfx.renderTargets.idImage },
				},
			};
			const BindGroup dynamicBindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

			SetBindGroup(commandList, 0, dynamicBindGroup);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 3, dynamicBindGroup);

			TransitionImageLayout(commandList, gfx.renderTargets.idImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

			Dispatch(commandList, 1, 1, 1);

			TransitionImageLayout(commandList, gfx.renderTargets.idImage, ImageStateShaderInput, ImageStateRenderTarget, 0, 1);
		}

		EndDebugGroup(commandList);
	}
}

void EditorPostRender(Engine &engine)
{
	EditorProcessCommands(engine, FrameArena);
}

