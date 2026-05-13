
void GameStart(Game &game)
{
	LOG(Info, "- GameStart!\n");

	game.character = {
		.id = 0,
		.pos = { 0, 0 },
		.size = { 1, 1 },
	};

	game.speed = {2.0f, 10.0f};
}

void GameUpdate(Game &game)
{
	LOG(Debug, "- GameUpdate!\n");

	constexpr float deltaSeconds = 1.0f / 60.0f;
	constexpr float gravity = -9.8f;

	float2 &pos = game.character.pos;

	pos.x += game.speed.x * deltaSeconds;
	if (pos.x > 10) pos.x = -10;

	pos.y = pos.y + game.speed.y * deltaSeconds + 0.5 * gravity * deltaSeconds * deltaSeconds;
	game.speed.y = game.speed.y + gravity * deltaSeconds;

	if (pos.y < 0.0f ) {
		pos.y = 0.0f;
		game.speed.y = 10.0f;
	}

	DrawBox(pos, game.character.size, {0.0, 0.5, 1.0, 1.0});
}

void GameStop(Game &game)
{
	LOG(Info, "- GameStop!\n");
}

