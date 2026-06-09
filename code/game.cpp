
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
}

void GameUpdate(Game &game)
{
	LOG(Debug, "- GameUpdate!\n");

	if (!game.playingMusic)
	{
		PlayMusic(game.modEquinox);
	}


	constexpr float deltaSeconds = 1.0f / 60.0f;
	constexpr float gravity = -15.8f;

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
		float2 &pos = game.box2.pos;
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

		pos.x += game.speed2.x * deltaSeconds;

		// Y ///////////////////////////////////////////////////////////

		if (KeyPress(*input.keyboard, K_SPACE)) {
			if (game.speed2.y == 0) {
				game.speed2.y = 10;
				PlayAudioClip(game.sndJump);
			}
		}
		game.speed2.y = game.speed2.y + gravity * deltaSeconds;
		pos.y += game.speed2.y * deltaSeconds + 0.5 * gravity * deltaSeconds * deltaSeconds;

		if (pos.y < 0) {
			pos.y = 0;
			game.speed2.y = 0;
		}

		if (pos.y < 0.0f ) {
			pos.y = 0.0f;
		}

		game.ent->position = Float3(game.box2.pos, 0.0);
		//DrawBox(game.box2.pos, game.box2.size, game.box2.color);
	}
}

void GameStop(Game &game)
{
	LOG(Info, "- GameStop!\n");
}

