
static Input input = {};

void GameSetInput(Input pInput)
{
	input = pInput;
}

void GameStart(Game &game)
{
	LOG(Info, "- GameStart!\n");

	game.box1 = {
		.pos = { 0, 0 },
		.size = { 1, 1 },
		.color = { 0.0, 0.5, 1.0, 1.0 },
	};
	game.box2 = {
		.pos = { 0, 0 },
		.size = { 1, 1 },
		.color = { 1.0, 0.5, 0.0, 1.0 },
	};

	game.speed = {2.0f, 10.0f};
	game.speed2 = {};
	game.accel2 = 50;

	game.ent = GetEntity("player");
	game.sndJump = GetAudioClip("snd_bell_wav");
	game.modEquinox = GetMusic("mod_equinox_mod");
	game.playingMusic = false;

	game.camera = {
		.projectionType = ProjectionOrthographic,
		.position = {0, 0, -1},
		//.height = 180.0f / PIXELS_PER_METER,
		.height = 90.0f / PIXELS_PER_METER,
	};

	game.screen = {
		.boundingBox = { 0, 0, 10 * SCENE_WIDTH/PIXELS_PER_METER, 2 * SCENE_HEIGHT/PIXELS_PER_METER},
	};
}

void GameUpdate(Game &game)
{
	LOG(Debug, "- GameUpdate!\n");

	SetCamera(game.camera);

	if (!game.playingMusic)
	{
		PlayMusic(game.modEquinox);
	}


	constexpr float deltaSeconds = 1.0f / 60.0f;
	constexpr float gravity = -15.8f;

	DrawBoxOutline(Float2(game.screen.boundingBox.pos), Float2(game.screen.boundingBox.size), ColorOrange);

	{
		float2 &pos = game.box1.pos;

		pos.x += game.speed.x * deltaSeconds;
		if (pos.x > 10) pos.x = -10;

		pos.y = pos.y + game.speed.y * deltaSeconds + 0.5 * gravity * deltaSeconds * deltaSeconds;
		game.speed.y = game.speed.y + gravity * deltaSeconds;

		if (pos.y < 0.0f ) {
			pos.y = 0.0f;
			game.speed.y = 10.0f;
		}
		DrawBox(game.box1.pos, game.box1.size, game.box1.color);
	}

	{
		float2 &playerPos = game.ent->position.xy;

		i32 direction = 0;

		// X ///////////////////////////////////////////////////////////

		if (KeyPressed(*input.keyboard, K_D)) {
			direction++;
		}
		if (KeyPressed(*input.keyboard, K_A)) {
			direction--;
		}

		if (direction < 0 && game.speed2.x > 0 ||
				direction > 0 && game.speed2.x < 0 ||
				!direction )
		{
			game.speed2.x *= 0.8;
		}

		if (direction != 0) {
			game.speed2.x = game.speed2.x + direction * game.accel2 * deltaSeconds;
		}

		game.speed2.x = Clamp(game.speed2.x, -15.0f, 15.0f);

		playerPos.x += game.speed2.x * deltaSeconds;

		// Y ///////////////////////////////////////////////////////////

		if (KeyPress(*input.keyboard, K_SPACE)) {
			if (game.speed2.y == 0) {
				game.speed2.y = 10;
				PlayAudioClip(game.sndJump);
			}
		}
		game.speed2.y = game.speed2.y + gravity * deltaSeconds;
		playerPos.y += game.speed2.y * deltaSeconds + 0.5 * gravity * deltaSeconds * deltaSeconds;

		if (playerPos.y < 0) {
			playerPos.y = 0;
			game.speed2.y = 0;
		}

		if (playerPos.y < 0.0f ) {
			playerPos.y = 0.0f;
		}

		// Player bounds
		const f32 screenLeft = game.screen.boundingBox.pos.x;
		const f32 screenRight = game.screen.boundingBox.pos.x + game.screen.boundingBox.size.x;
		const f32 screenBottom = game.screen.boundingBox.pos.y;
		const f32 screenTop = game.screen.boundingBox.pos.y + game.screen.boundingBox.size.y;
		playerPos.x = Clamp(playerPos.x, screenLeft, screenRight);
		playerPos.y = Clamp(playerPos.y, screenBottom, screenTop);

		// Camera bounds
		const float2 halfSceneSize = 0.5f * float2{SCENE_WIDTH, SCENE_HEIGHT} / PIXELS_PER_METER;
		const f32 cameraLeft = screenLeft + halfSceneSize.x;
		const f32 cameraRight = screenRight - halfSceneSize.x;
		const f32 cameraBottom = screenBottom + halfSceneSize.y;
		const f32 cameraTop = screenTop - halfSceneSize.y;
		const f32 cameraX = Lerp(game.camera.position.x, playerPos.x, 0.2f);
		const f32 cameraY = Lerp(game.camera.position.y, playerPos.y, 0.2f);
		game.camera.position.x = Clamp(cameraX, cameraLeft, cameraRight);
		game.camera.position.y = Clamp(cameraY, cameraBottom, cameraTop);
	}
}

void GameStop(Game &game)
{
	LOG(Info, "- GameStop!\n");
}

