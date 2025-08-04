
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

void SaveAssetDescriptors(const char *path, const AssetDescriptors &assets)
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
		WriteLine(ctx, ".mipmap = %d,", desc.mipmap);
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

	WriteSectionLine(ctx, "Audio clips");

	for (u32 i = 0; i < assets.audioClipDescCount; ++i)
	{
		const AudioClipDesc &desc = assets.audioClipDescs[i];

		WriteLine(ctx, "AudioClip %s {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		PopIndent(ctx);

		WriteLine(ctx, "}");
		NewLine(ctx);
	}

	if ( !WriteEntireFile(path, ctx.arena.base, ctx.arena.used) )
	{
		LOG(Warning, "Could not write data file: %s\n", path);
	}
}














#if USE_DATA_BUILD

void BuildAssets(Arena tempArena)
{
	LOG(Info, "Building data\n");

	FilePath dir;
	CreateDirectory( MakePath(ProjectDir, "build").str );
	CreateDirectory( MakePath(ProjectDir, "build/shaders").str );

	CompileShaders();

	FilePath filepath =  MakePath(DataDir, "assets.dat");
	FILE *file = fopen(filepath.str, "wb");
	if ( file )
	{
		const u32 shaderCount = ARRAY_COUNT(shaderSources);
		const u32 shadersOffset = sizeof( BinAssetsHeader );
		const u32 shadersSize = shaderCount * sizeof(BinShaderDesc);
		const u32 imageCount = ARRAY_COUNT(textures);
		const u32 imagesOffset = shadersOffset + shadersSize;
		const u32 imagesSize = imageCount * sizeof(BinImageDesc);
		const u32 audioClipCount = ARRAY_COUNT(audioClipDescs);
		const u32 audioClipsOffset = imagesOffset + imagesSize;
		const u32 audioClipsSize = audioClipCount * sizeof(BinAudioClipDesc);
		const u32 materialCount = ARRAY_COUNT(materialDescs);
		const u32 materialsOffset = audioClipsOffset + audioClipsSize;
		const u32 materialsSize = materialCount * sizeof(BinMaterialDesc);
		const u32 entityCount = ARRAY_COUNT(entityDescs);
		const u32 entitiesOffset = materialsOffset + materialsSize;
		const u32 entitiesSize = entityCount * sizeof(BinEntityDesc);
		const u32 basePayloadOffset = entitiesOffset + entitiesSize;

		// Write file header
		const BinAssetsHeader fileHeader = {
			.magicNumber = U32FromChars('I', 'R', 'I', 'S'),
			.shadersOffset = shadersOffset,
			.shaderCount = shaderCount,
			.imagesOffset = imagesOffset,
			.imageCount = imageCount,
			.audioClipsOffset = audioClipsOffset,
			.audioClipCount = audioClipCount,
			.materialsOffset = materialsOffset,
			.materialCount = materialCount,
			.entitiesOffset = entitiesOffset,
			.entityCount = entityCount,
		};
		fwrite(&fileHeader, sizeof(fileHeader), 1, file);

		u32 payloadOffset = basePayloadOffset;

		// Reserve space for asset descs
		BinShaderDesc *binShaderDescs = PushArray(tempArena, BinShaderDesc, shaderCount);
		BinImageDesc *binImageDescs = PushArray(tempArena, BinImageDesc, imageCount);
		BinAudioClipDesc *binAudioClipDescs = PushArray(tempArena, BinAudioClipDesc, audioClipCount);
		BinMaterialDesc *binMaterialDescs = PushArray(tempArena, BinMaterialDesc, materialCount);
		BinEntityDesc *binEntityDescs = PushArray(tempArena, BinEntityDesc, entityCount);

		// Prepare asset descs and write asset payloads

		fseek(file, payloadOffset, SEEK_SET);

		// Shaders
		for (u32 i = 0; i < shaderCount; ++i)
		{
			const ShaderSourceDesc &shaderSourceDesc = shaderSources[i];

			char filepath[MAX_PATH_LENGTH];
			SPrintf(filepath, "%s/shaders/%s.spv", DataDir, shaderSourceDesc.name);

			u64 payloadSize = 0;
			GetFileSize(filepath, payloadSize);

			// Save asset desc info
			BinShaderDesc &shaderHeader = binShaderDescs[i];
			StrCopy(shaderHeader.name, shaderSourceDesc.name);
			StrCopy(shaderHeader.entryPoint, shaderSourceDesc.entryPoint);
			shaderHeader.type = shaderSourceDesc.type;
			shaderHeader.dataOffset = payloadOffset;
			shaderHeader.dataSize = U64ToU32(payloadSize);

			Arena scratch = MakeSubArena(tempArena);
			void *shaderPayload = PushSize(scratch, payloadSize);
			ReadEntireFile(filepath, shaderPayload, payloadSize);

			fwrite(shaderPayload, payloadSize, 1, file);

			payloadOffset += payloadSize;
		}

		// Images
		for (u32 i = 0; i < imageCount; ++i)
		{
			const TextureDesc &textureDesc = textures[i];

			const FilePath imagePath = MakePath(ProjectDir, textureDesc.filename);

			int texWidth, texHeight, texChannels;
			stbi_uc* originalPixels = stbi_load(imagePath.str, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			texChannels = 4; // Because we use STBI_rgb_alpha
			stbi_uc* pixels = originalPixels;
			if ( !pixels )
			{
				LOG(Error, "stbi_load failed to load %s\n", imagePath.str);
				static stbi_uc constPixels[] = {255, 0, 255, 255};
				pixels = constPixels;
				texWidth = texHeight = 1;
				texChannels = 4;
			}

			const u64 payloadSize = texWidth * texHeight * texChannels;

			// Save asset desc info
			BinImageDesc &imageHeader = binImageDescs[i];
			StrCopy(imageHeader.name, textureDesc.name);
			imageHeader.width = I32ToU16(texWidth),
			imageHeader.height = I32ToU16(texHeight),
			imageHeader.channels = I32ToU8(texChannels),
			imageHeader.mipmap = textureDesc.mipmap,
			imageHeader.dataOffset = payloadOffset,
			imageHeader.dataSize = U64ToU32(payloadSize),

			fwrite(pixels, payloadSize, 1, file);

			if ( originalPixels ) {
				stbi_image_free(originalPixels);
			}

			payloadOffset += payloadSize;
		}

		// AudioClips
		for (u32 i = 0; i < audioClipCount; ++i)
		{
			const AudioClipDesc &audioClipDesc = audioClipDescs[i];

			const FilePath path = MakePath(ProjectDir, audioClipDesc.filename);

			AudioClip audioClip;
			Arena scratch = MakeSubArena(tempArena);
			const bool ok = LoadAudioClipFromWAVFile(path.str, scratch, audioClip);

			const u64 payloadSize = audioClip.sampleCount * audioClip.sampleSize;

			const BinAudioClipDesc audioClipHeader = {
				.sampleCount = audioClip.sampleCount,
				.samplingRate = audioClip.samplingRate,
				.sampleSize = audioClip.sampleSize,
				.channelCount = audioClip.channelCount,
				.dataOffset = payloadOffset,
				.dataSize = U64ToU32(payloadSize),
			};
			binAudioClipDescs[i] = audioClipHeader;

			if ( ok ) {
				fwrite(audioClip.samples, payloadSize, 1, file);
			} else {
				fseek(file, payloadSize, SEEK_CUR);
			}

			payloadOffset += payloadSize;
		}

		// Materials
		for (u32 i = 0; i < materialCount; ++i)
		{
			const MaterialDesc &materialDesc = materialDescs[i];

			BinMaterialDesc &binMaterialDesc = binMaterialDescs[i];
			StrCopy(binMaterialDesc.name, materialDesc.name);
			StrCopy(binMaterialDesc.textureName, materialDesc.textureName);
			StrCopy(binMaterialDesc.pipelineName, materialDesc.pipelineName);
			binMaterialDesc.uvScale = materialDesc.uvScale;
		}

		// Entities
		for (u32 i = 0; i < entityCount; ++i)
		{
			const EntityDesc &entityDesc = entityDescs[i];

			BinEntityDesc &binEntityDesc = binEntityDescs[i];
			StrCopy(binEntityDesc.name, entityDesc.name);
			StrCopy(binEntityDesc.materialName, entityDesc.materialName);
			binEntityDesc.pos = entityDesc.pos;
			binEntityDesc.scale = entityDesc.scale;
			binEntityDesc.geometryType = entityDesc.geometryType;
		}

		// Write asset descs

		fseek(file, sizeof(fileHeader), SEEK_SET);
		fwrite(binShaderDescs, sizeof(binShaderDescs[0]), shaderCount, file);
		fwrite(binImageDescs, sizeof(binImageDescs[0]), imageCount, file);
		fwrite(binAudioClipDescs, sizeof(binAudioClipDescs[0]), audioClipCount, file);
		fwrite(binMaterialDescs, sizeof(binMaterialDescs[0]), materialCount, file);
		fwrite(binEntityDescs, sizeof(binEntityDescs[0]), entityCount, file);

		fclose(file);
	}
}

#endif // USE_DATA_BUILD

BinAssets LoadAssets(Arena &dataArena)
{
	BinAssets assets = {};

	ResetArena(dataArena);

	const FilePath filepath = MakePath(DataDir, "assets.dat");

	DataChunk *chunk = PushFile( dataArena, filepath.str );
	if ( !chunk ) {
		LOG( Error, "Could not open file %s\n", filepath.str );
		QUIT_ABNORMALLY();
	}

	byte *bytes =  chunk->bytes;
	BinAssetsHeader fileHeader = *(BinAssetsHeader*)bytes;

	if (fileHeader.magicNumber != U32FromChars('I', 'R', 'I', 'S'))
	{
		LOG( Error, "Wrong magic number in file %s\n", filepath.str );
		QUIT_ABNORMALLY();
	}

	assets.header = fileHeader;
	assets.shaders = PushArray(dataArena, BinShader, assets.header.shaderCount);
	assets.images = PushArray(dataArena, BinImage, assets.header.imageCount);
	assets.audioClips = PushArray(dataArena, BinAudioClip, assets.header.audioClipCount);
	assets.materialDescs = PushArray(dataArena, BinMaterialDesc, assets.header.materialCount);

	BinShaderDesc *binShaderDescs = (BinShaderDesc*)(bytes + fileHeader.shadersOffset);

	for (u32 i = 0; i < fileHeader.shaderCount; ++i)
	{
		BinShader &shader = assets.shaders[i];
		shader.desc = binShaderDescs + i;
		shader.spirv = bytes + shader.desc->dataOffset;
	}

	BinImageDesc *binImageDescs = (BinImageDesc*)(bytes + fileHeader.imagesOffset);

	for (u32 i = 0; i < fileHeader.imageCount; ++i)
	{
		BinImage &image = assets.images[i];
		image.desc = binImageDescs + i;
		image.pixels = bytes + image.desc->dataOffset;
	}

	BinAudioClipDesc *binAudioClipDescs = (BinAudioClipDesc*)(bytes + fileHeader.audioClipsOffset);

	for (u32 i = 0; i < fileHeader.audioClipCount; ++i)
	{
		BinAudioClip &audioClip = assets.audioClips[i];
		audioClip.desc = binAudioClipDescs + i;
		audioClip.samples = bytes + audioClip.desc->dataOffset;
	}

	assets.materialDescs = (BinMaterialDesc*)(bytes + fileHeader.materialsOffset);
	assets.entityDescs = (BinEntityDesc*)(bytes + fileHeader.entitiesOffset);

	return assets;
}

