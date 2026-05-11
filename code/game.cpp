
static float2 speed = {5.0f, 10.0f};

void GameStart(Game &game)
{
	LOG(Info, "- GameStart!\n");

	game.character = {
		.id = 0,
		.pos = { 0, 0 },
		.size = { 1, 1 },
	};

	speed = {2.0f, 10.0f};
}

void GameUpdate(Game &game)
{
	LOG(Debug, "- GameUpdate!\n");

	constexpr float deltaSeconds = 1.0f / 60.0f;
	constexpr float gravity = -9.8f;

	float2 &pos = game.character.pos;

	pos.x += speed.x * deltaSeconds;
	if (pos.x > 10) pos.x = -10;

	pos.y = pos.y + speed.y * deltaSeconds + 0.5 * gravity * deltaSeconds * deltaSeconds;
	speed.y = speed.y + gravity * deltaSeconds;

	if (pos.y < 0.0f ) {
		pos.y = 0.0f;
		speed.y = 10.0f;
	}

	DrawBox(pos, game.character.size, {0.4, 1.0, 0.3, 1.0});
}

void GameStop(Game &game)
{
	LOG(Info, "- GameStop!\n");
}

