
static void AddEditorCommand(Editor &editor, const EditorCommand &command)
{
	ASSERT(editor.commandCount < ARRAY_COUNT(editor.commands));
	editor.commands[editor.commandCount++] = command;
}

static void EditorUpdateUI(Engine &engine)
{
	UI &ui = GetUI(engine);
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	char text[MAX_PATH_LENGTH];
	SPrintf(text, "Debug UI");

	UI_BeginWindow(ui, text);

	if ( UI_Section(ui, "Audio") )
	{
		if ( UI_Button(ui, "Play sound") )
		{
			const u32 index = 0;//GetAudioClipIndex(engine, "aclip_bell");
			PlayAudioClip(engine, index);
		}

		UI_Separator(ui);

		static u32 audioSourceMusic = INVALID_AUDIO_SOURCE;
		if ( !IsActiveAudioSource(engine, audioSourceMusic) ) {
			audioSourceMusic = INVALID_AUDIO_SOURCE;
		}

		if ( audioSourceMusic == INVALID_AUDIO_SOURCE ) {
			if ( UI_Button(ui, "Play music") ) {
				const u32 audioClipIndex = 1; //GetAudioClipIndex(engine, "aclip_music");
				if ( audioClipIndex != INVALID_AUDIO_CLIP ) {
					audioSourceMusic = PlayAudioClip(engine, audioClipIndex);
				}
			}
		} else {
			if ( IsPausedAudioSource(engine, audioSourceMusic) ) {
				if ( UI_Button(ui, "Resume music") ) {
					ResumeAudioSource(engine, audioSourceMusic);
				}
			} else {
				if ( UI_Button(ui, "Pause music") ) {
					PauseAudioSource(engine, audioSourceMusic);
				}
			} if ( UI_Button(ui, "Stop music") ) {
				StopAudioSource(engine, audioSourceMusic);
			}
		}
	}

	if ( UI_Section(ui, "Profiling" ) )
	{
		constexpr f32 maxExpectedMillis = 1000.0f / 60.0f; // Like expecting to reach 60 fps

		const u32 sceneWidth = gfx.device.swapchain.extent.width;
		const u32 sceneHeight = gfx.device.swapchain.extent.height;
		SPrintf(text, "Resolution: %ux%u px", sceneWidth, sceneHeight);
		UI_Label(ui, text);

		const TimeSamples &gpuTimes = gfx.gpuFrameTimes;
		SPrintf(text, "GPU %.02f ms / %.00f fps ", gpuTimes.average, 1000.0f / gpuTimes.average);
		UI_Label(ui, text);
		UI_Histogram(ui, gpuTimes.samples, ARRAY_COUNT(gpuTimes.samples), maxExpectedMillis);

		const TimeSamples &cpuTimes = gfx.cpuFrameTimes;
		SPrintf(text, "CPU %.02f ms / %.00f fps ", cpuTimes.average, 1000.0f / cpuTimes.average);
		UI_Label(ui, text);
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

		const Platform &platform = engine.platform;
		const char *unitsStr = unitsStrArray[units];
		const u32 unitsSize = unitsSizeArray[units];
		SPrintf(text, "- Global Arena: %u / %u %s", platform.globalArena.used / unitsSize, platform.globalArena.size / unitsSize, unitsStr);
		UI_Label(ui, text);
		SPrintf(text, "- Frame Arena: %u / %u %s", platform.frameArena.used / unitsSize, platform.frameArena.size / unitsSize, unitsStr);
		UI_Label(ui, text);
		SPrintf(text, "- String Arena: %u / %u %s", platform.stringArena.used / unitsSize, platform.stringArena.size / unitsSize, unitsStr);
		UI_Label(ui, text);
		SPrintf(text, "- Data Arena: %u / %u %s", platform.dataArena.used / unitsSize, platform.dataArena.size / unitsSize, unitsStr);
		UI_Label(ui, text);
	}

	if ( UI_Section(ui, "Scene") )
	{
		for (u32 i = 0; i < scene.entityHandles.handleCount; ++i)
		{
			Entity &entity = GetEntityAt(scene, i);
			if ( UI_Radio(ui, entity.name, editor.selectedEntity == i) )
			{
				editor.selectedEntity = i;
			}
		}
		if (UI_Button(ui, "Unselect"))
		{
			editor.selectedEntity = U32_MAX;
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
		const char *radioOptions[] = { "Perspective", "Orthographic" };
		CT_ASSERT(ARRAY_COUNT(radioOptions) == ProjectionTypeCount);

		for (u32 i = 0; i < ProjectionTypeCount; ++i)
		{
			if ( UI_Radio(ui, radioOptions[i], editor.cameraType == i) )
			{
				editor.cameraType = (ProjectionType)i;
			}
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
		UI_BeginLayout(ui, UiLayoutHorizontal);
		for (u32 i = 0; i < gfx.textureHandles.handleCount; ++i)
		{
			const TextureH handle = GetHandleAt(gfx.textureHandles, i);
			const Texture &texture = GetTexture(gfx, handle);

			const UIWidgetFlags flags = selectedHandle == handle ? UIWidgetFlag_Outline : UIWidgetFlag_None;
			if (UI_Image(ui, texture.image, flags))
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

		if ( UI_Button(ui, "Recompile") )
		{
			engine.game.shouldRecompile = true;
		}
	}

#if 0
	if ( UI_Section(ui, "Buttons") )
	{
		static u32 labelIndex = 0;
		const char *labels[] = {"Label 1", "Label 2"};
		const char *label = labels[labelIndex];

		UI_Label(ui, label);

		UI_Separator(ui);

		UI_BeginLayout(ui, UiLayoutHorizontal);

		if ( UI_Button(ui, "Hola") )
		{
			labelIndex = (labelIndex + 1) % ARRAY_COUNT(labels);
		}

		UI_Button(ui, "Adios");

		UI_Button(ui, "Memory");

		UI_EndLayout(ui);

	}

	if ( UI_Section(ui, "Radio/Check") )
	{
		const char *radioOptions[] = { "Option 1", "Option 2", "Option 3" };
		static int selectedOption = 0;

		for (u32 i = 0; i < ARRAY_COUNT(radioOptions); ++i)
		{
			if ( UI_Radio(ui, radioOptions[i], selectedOption == i) )
			{
				selectedOption = i;
			}
		}

		UI_Separator(ui);

		static const char *checkOptions[] = { "Check 1", "Check 2", "Check 3" };
		static bool checkSelections[ARRAY_COUNT(checkOptions)] = {};

		for (u32 i = 0; i < ARRAY_COUNT(checkOptions); ++i)
		{
			UI_Checkbox(ui, checkOptions[i], &checkSelections[i]);
		}

		UI_Separator(ui);

		static const char *comboOptions[] = { "Combo 1", "Combo 2", "Combo 3" };
		static u32 comboIndex = 0;

		UI_Combo(ui, "Select type", comboOptions, ARRAY_COUNT(comboOptions), &comboIndex);
	}

	if ( UI_Section(ui, "Images") )
	{
		UI_BeginLayout(ui, UiLayoutHorizontal);
		for (u32 i = 0; i < gfx.textureCount; ++i)
		{
			UI_Image(ui, gfx.textures[i].image);
		}
		UI_EndLayout(ui);
	}

	if ( UI_Section(ui, "Inputs") )
	{
		static char text[128] = "name";
		static i32 intNumber = 0;
		static f32 floatNumber = 0.0f;

		UI_InputText(ui, "Input text", text, ARRAY_COUNT(text));
		UI_InputInt(ui, "Input int", &intNumber);
		UI_InputFloat(ui, "Input float", &floatNumber);
	}
#endif

	UI_EndWindow(ui);
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

static void EditorUpdateCamera2D(const Window &window, const Input &input, Camera &camera, float deltaSeconds, bool handleInput)
{
	const Gamepad &gamepad = input.gamepad;

	float3 dir = { 0, 0, 0 };

	if ( handleInput )
	{
		// Camera rotation
		f32 deltaYaw = 0.0f;
		f32 deltaPitch = 0.0f;
#if 0
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
		if ( KeyPressed(window.keyboard, K_W) ) { dir = Add(dir, UpDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, K_S) ) { dir = Add(dir, Negate( UpDirectionFromAngles(angles) )); }
		if ( KeyPressed(window.keyboard, K_D) ) { dir = Add(dir, RightDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, K_A) ) { dir = Add(dir, Negate( RightDirectionFromAngles(angles) )); }

		dir = dir + gamepad.leftAxis.x * RightDirectionFromAngles(angles);
		dir = dir + gamepad.leftAxis.y * UpDirectionFromAngles(angles);
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

static void EditorBeginEntitySelection(Engine &engine, const Mouse &mouse, bool handleInput)
{
	Editor &editor = engine.editor;

	static bool mouseMoved = false;
	if ( handleInput )
	{
		if (MouseButtonPress(mouse, MOUSE_BUTTON_LEFT)) {
			mouseMoved = false;
		}
		if (mouse.dx != 0 || mouse.dx != 0) {
			mouseMoved = true;
		}
		if (!mouseMoved && MouseButtonRelease(mouse, MOUSE_BUTTON_LEFT)) {
			editor.selectEntity = true;
		}
	}
}

static void EditorEndEntitySelection(Engine &engine)
{
	Editor &editor = engine.editor;

	if ( editor.selectEntity )
	{
		WaitDeviceIdle(engine.gfx.device);
		editor.selectedEntity = *(u32*)GetBufferPtr(engine.gfx.device, engine.gfx.selectionBufferH);
		editor.selectEntity = false;
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
					LoadSceneFromTxt(engine);
					break;
				}
				case EditorCommandSaveTxt:
				{
					SaveSceneToTxt(engine);
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

void EditorInitialize(Engine &engine)
{
	Editor &editor = engine.editor;

	engine.editor.camera[ProjectionPerspective].projectionType = ProjectionPerspective;
	engine.editor.camera[ProjectionPerspective].position = {0, 1, 2};
	engine.editor.camera[ProjectionPerspective].orientation = {0, -0.45f};

	engine.editor.camera[ProjectionOrthographic].projectionType = ProjectionOrthographic;
	engine.editor.camera[ProjectionOrthographic].position = {0, 0, -5};
	engine.editor.camera[ProjectionOrthographic].orientation = {};

	engine.editor.cameraType = ProjectionPerspective;

	engine.editor.selectedEntity = U32_MAX;
}

void EditorUpdate(Engine &engine)
{
	Platform &platform = engine.platform;

	EditorUpdateUI(engine);

	const bool handleInput = !engine.ui.wantsInput;

	if (engine.editor.cameraType == ProjectionPerspective)
	{
		EditorUpdateCamera3D(platform.window, engine.editor.camera[ProjectionPerspective], platform.deltaSeconds, handleInput);
	}
	else if (engine.editor.cameraType == ProjectionOrthographic)
	{
		EditorUpdateCamera2D(platform.window, platform.input, engine.editor.camera[ProjectionOrthographic], platform.deltaSeconds, handleInput);
	}

	EditorBeginEntitySelection(engine, platform.window.mouse, handleInput);

	EditorProcessCommands(engine, engine.platform.frameArena);
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
		BeginDebugGroup(commandList, "Entity selection");

		{ // Draw entity IDs
			SetClearColor(commandList, 0, U32_MAX);

			BeginRenderPass(commandList, gfx.renderTargets.idFramebuffer );

			const uint2 framebufferSize = GetFramebufferSize(gfx.renderTargets.idFramebuffer);
			SetViewportAndScissor(commandList, framebufferSize);

			SetPipeline(commandList, gfx.idPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 entityIndex = 0; entityIndex < scene.entityHandles.handleCount; ++entityIndex)
			{
				const Entity &entity = GetEntityAt(scene, entityIndex);

				if ( !entity.visible || entity.culled ) continue;

				// Draw!!!
				const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
				const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
				const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);
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
			const BindGroup dynamicBindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

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
	EditorEndEntitySelection(engine);
}

