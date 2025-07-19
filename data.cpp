
struct WriteContext
{
	Arena arena;
	char line[MAX_PATH_LENGTH];
	u32 indent;
};

static void WriteIndentation(WriteContext &ctx)
{
	for (u32 i = 0; i < ctx.indent; ++i) {
		PushChar(ctx.arena, ' ');
	}
}

static void NewLine(WriteContext &ctx)
{
	PushChar(ctx.arena, '\n');
}

static void WriteSectionLine(WriteContext &ctx, const char *name)
{
	constexpr u32 maxLineLength = 72;
	u32 lineLength = 0;

	NewLine(ctx);

	WriteIndentation(ctx);
	lineLength += ctx.indent;

	PushChar(ctx.arena, '/'); lineLength++;
	PushChar(ctx.arena, '/'); lineLength++;
	PushChar(ctx.arena, ' '); lineLength++;

	while (*name) {
		PushChar(ctx.arena, *name++);
		lineLength++;
	}

	PushChar(ctx.arena, ' '); lineLength++;

	while (lineLength < maxLineLength) {
		PushChar(ctx.arena, '/'); lineLength++;
	}

	NewLine(ctx);
	NewLine(ctx);
}

static void WriteLine(WriteContext &ctx, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);

	// Push indentation
	WriteIndentation(ctx);

	// Push text
	const i32 len = VSPrintf(ctx.line, format, vaList);
	char *chars = (char*)PushSize(ctx.arena, len);
	MemCopy(chars, ctx.line, len);
	
	// Push new line
	NewLine(ctx);

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

static const char *GeometryTypeToString(GeometryType type)
{
	switch (type)
	{
		case GeometryTypeCube: return "GeometryTypeCube";
		case GeometryTypePlane: return "GeometryTypePlane";
		case GeometryTypeScreen: return "GeometryTypeScreen";
		default:;
	}
	return "<unknown>";
}

void DataSaveToTextFile(const char *path, const AssetData &assets)
{
	Scratch scratch;
	WriteContext ctx = {
		.arena = scratch.arena,
	};

	LOG(Info, "Saving data to text file: %s\n", path);

	char line[MAX_PATH_LENGTH];

	WriteSectionLine(ctx, "Textures");

	for (u32 i = 0; i < assets.textureDescCount; ++i)
	{
		const TextureDesc &desc = assets.textureDescs[i];

		WriteLine(ctx, "Texture %s {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		WriteLine(ctx, ".mipmap = \"%d\",", desc.mipmap);
		PopIndent(ctx);

		WriteLine(ctx, "}");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Materials");

	for (u32 i = 0; i < assets.materialDescCount; ++i)
	{
		const MaterialDesc &desc = assets.materialDescs[i];

		WriteLine(ctx, "Material %s {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".textureName = \"%s\",", desc.textureName);
		WriteLine(ctx, ".pipelineName = \"%s\",", desc.pipelineName);
		WriteLine(ctx, ".uvScale = %f,", desc.uvScale);
		PopIndent(ctx);

		WriteLine(ctx, "}");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Entities");

	for (u32 i = 0; i < assets.entityDescCount; ++i)
	{
		const EntityDesc &desc = assets.entityDescs[i];

		WriteLine(ctx, "Entity %s {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".materialName = \"%s\",", desc.materialName);
		WriteLine(ctx, ".pos = {%f, %f, %f},", desc.pos.x, desc.pos.y, desc.pos.z );
		WriteLine(ctx, ".scale = %f,", desc.scale);
		WriteLine(ctx, ".geometryType = %s,", GeometryTypeToString(desc.geometryType));
		PopIndent(ctx);

		WriteLine(ctx, "}");
		NewLine(ctx);
	}

	if ( !WriteEntireFile(path, ctx.arena.base, ctx.arena.used) )
	{
		LOG(Warning, "Could not write data file: %s\n", path);
	}
}

