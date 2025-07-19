
struct WriteContext
{
	Arena arena;
	char line[MAX_PATH_LENGTH];
	u32 indent;
};

static void WriteLine(WriteContext &ctx, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);

	// Push indentation
	for (u32 i = 0; i < ctx.indent; ++i) {
		PushChar(ctx.arena, '\t');
	}

	// Push text
	const i32 len = VSPrintf(ctx.line, format, vaList);
	char *chars = (char*)PushSize(ctx.arena, len);
	MemCopy(chars, ctx.line, len);
	
	// Push new line
	PushChar(ctx.arena, '\n');

	va_end(vaList);
}

static void PushIndent(WriteContext &ctx)
{
	ctx.indent++;
}

static void PopIndent(WriteContext &ctx)
{
	ASSERT(ctx.indent > 0);
	ctx.indent--;
}

void DataSaveToText(const char *path, const AssetData &assets)
{
	Scratch scratch;
	WriteContext ctx = {
		.arena = scratch.arena,
	};

	LOG(Debug, "DataSaveToText\n");

	char line[MAX_PATH_LENGTH];

	for (u32 i = 0; i < assets.textureDescCount; ++i)
	{
		const TextureDesc &desc = assets.textureDescs[i];

		WriteLine(ctx, "Texture %s {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		WriteLine(ctx, ".mipmap = \"%d\",", desc.mipmap);
		PopIndent(ctx);

		WriteLine(ctx, "}");
	}

	if ( !WriteEntireFile(path, ctx.arena.base, ctx.arena.used) )
	{
		LOG(Warning, "Could not write data file: %s\n", path);
	}
}

