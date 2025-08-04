
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

		WriteLine(ctx, "Texture %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		WriteLine(ctx, ".mipmap = %d,", desc.mipmap);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Materials");

	for (u32 i = 0; i < assets.materialDescCount; ++i)
	{
		const MaterialDesc &desc = assets.materialDescs[i];

		WriteLine(ctx, "Material %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".textureName = \"%s\",", desc.textureName);
		WriteLine(ctx, ".pipelineName = \"%s\",", desc.pipelineName);
		WriteLine(ctx, ".uvScale = %f,", desc.uvScale);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Entities");

	for (u32 i = 0; i < assets.entityDescCount; ++i)
	{
		const EntityDesc &desc = assets.entityDescs[i];

		WriteLine(ctx, "Entity %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".materialName = \"%s\",", desc.materialName);
		WriteLine(ctx, ".pos = {%f, %f, %f},", desc.pos.x, desc.pos.y, desc.pos.z );
		WriteLine(ctx, ".scale = %f,", desc.scale);
		WriteLine(ctx, ".geometryType = %s,", GeometryTypeToString(desc.geometryType));
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Audio clips");

	for (u32 i = 0; i < assets.audioClipDescCount; ++i)
	{
		const AudioClipDesc &desc = assets.audioClipDescs[i];

		WriteLine(ctx, "AudioClip %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	if ( !WriteEntireFile(path, ctx.arena.base, ctx.arena.used) )
	{
		LOG(Warning, "Could not write data file: %s\n", path);
	}
}














#if USE_DATA_BUILD

enum DTokenId
{
	// Single character tokens
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_MINUS,
	TOKEN_SEMICOLON,
	TOKEN_EQUAL,
	// Literals
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_CHARACTER,
	TOKEN_NUMBER,
	// End
	TOKEN_EOF,
	TOKEN_COUNT,
};

static const char *DTokenIdNames[] =
{
	// Single character tokens
	"TOKEN_LEFT_BRACE",
	"TOKEN_RIGHT_BRACE",
	"TOKEN_COMMA",
	"TOKEN_DOT",
	"TOKEN_MINUS",
	"TOKEN_SEMICOLON",
	"TOKEN_EQUAL",
	// Literals
	"TOKEN_IDENTIFIER",
	"TOKEN_STRING",
	"TOKEN_CHARACTER",
	"TOKEN_NUMBER",
	// End
	"TOKEN_EOF",
};

CT_ASSERT(ARRAY_COUNT(DTokenIdNames) == TOKEN_COUNT);

enum ValueType
{
	VALUE_TYPE_BOOL,
	VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_STRING,
	VALUE_TYPE_CHAR,
	VALUE_TYPE_NULL,
};

struct DValue
{
	ValueType type;
	union
	{
		bool b;
		char c;
		i32 i;
		f32 f;
		String s;
	};
};

typedef DValue DLiteral;

struct DToken
{
	DTokenId id;
	u32 line;
	String lexeme;
	DLiteral literal;
};

static DToken gNullToken = {
	.id = TOKEN_COUNT,
	.line = 0,
	.lexeme = { "", 0 },
	.literal = { .type = VALUE_TYPE_NULL, .i = 0 },
};

struct DTokenList
{
	Arena *arena;
	DToken *tokens;
	i32 count;
	bool valid;
};

struct DScanner
{
	i32 start;
	i32 current;
	i32 currentInLine;
	u32 line;
	bool hasErrors;

	const char *text;
	u32 textSize;
};

static bool Char_IsEOL(char character)
{
	return character == '\n';
}

static bool Char_IsAlpha(char character)
{
	return
		( character >= 'a' && character <= 'z' ) ||
		( character >= 'A' && character <= 'Z' ) ||
		( character == '_' );
}

static bool Char_IsDigit(char character)
{
	return character >= '0' && character <= '9';
}

static bool Char_IsAlphaNumeric(char character)
{
	return Char_IsAlpha(character) || Char_IsDigit(character);
}

static bool DScanner_IsAtEnd(const DScanner &scanner)
{
	return scanner.current >= scanner.textSize;
}

static char DScanner_Advance(DScanner &scanner)
{
	ASSERT(scanner.current < scanner.textSize);
	char currentChar = scanner.text[scanner.current];
	scanner.current++;
	scanner.currentInLine++;
	return currentChar;
}

static bool DScanner_Consume(DScanner &scanner, char expected)
{
	if ( DScanner_IsAtEnd(scanner) ) return false;
	if ( scanner.text[scanner.current] != expected ) return false;
	scanner.current++;
	scanner.currentInLine++;
	return true;
}

static char DScanner_Peek(const DScanner &scanner)
{
	return DScanner_IsAtEnd(scanner) ? '\0' : scanner.text[ scanner.current ];
}

static char DScanner_PeekNext(const DScanner &scanner)
{
	if (scanner.current + 1 >= scanner.textSize) return '\0';
	return scanner.text[ scanner.current + 1 ];
}

static String DScanner_ScannedString(const DScanner &scanner)
{
	const char *lexemeStart = scanner.text + scanner.start;
	u32 lexemeSize = scanner.current - scanner.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

static void DScanner_AddToken(const DScanner &scanner, DTokenList &tokenList, DTokenId tokenId, DLiteral literal)
{
	DToken newToken = {};
	newToken.id = tokenId;
	newToken.lexeme = DScanner_ScannedString(scanner);
	newToken.literal = literal;
	newToken.line = scanner.line;

	PushStruct(*tokenList.arena, DToken);
	tokenList.tokens[tokenList.count++] = newToken;
}

static void DScanner_AddToken(const DScanner &scanner, DTokenList &tokenList, DTokenId tokenId)
{
	DLiteral literal = {};

	if ( tokenId == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = DScanner_ScannedString(scanner);
		literal.s.str += 1; // start after the first " char
		literal.s.size -= 2; // remove both " characters
	}
	if ( tokenId == TOKEN_CHARACTER )
	{
		literal.type = VALUE_TYPE_CHAR;
		String lexeme = DScanner_ScannedString(scanner);
		literal.c = *(lexeme.str+1); // start after the ' char
	}
	else if ( tokenId == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = StrToFloat( DScanner_ScannedString(scanner) );
	}

	DScanner_AddToken(scanner, tokenList, tokenId, literal);
}

static void DScanner_SetError(DScanner &scanner, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	printf("ERROR: %d:%d: ", scanner.line, scanner.currentInLine);
	vprintf(format, vaList);
	printf("\n");
	va_end(vaList);
	scanner.hasErrors = true;
}

static void DScanner_ScanToken(DScanner &scanner, DTokenList &tokenList)
{
	scanner.start = scanner.current;

	char c = DScanner_Advance(scanner);

	switch (c)
	{
		case '{': DScanner_AddToken(scanner, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': DScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_BRACE); break;
		case ',': DScanner_AddToken(scanner, tokenList, TOKEN_COMMA); break;
		case '.': DScanner_AddToken(scanner, tokenList, TOKEN_DOT); break;
		case '-': DScanner_AddToken(scanner, tokenList, TOKEN_MINUS); break;
		case ';': DScanner_AddToken(scanner, tokenList, TOKEN_SEMICOLON); break;
		case '=': DScanner_AddToken(scanner, tokenList, TOKEN_EQUAL ); break;
		case '/':
			if ( DScanner_Consume(scanner, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !Char_IsEOL( DScanner_Peek(scanner) ) && !DScanner_IsAtEnd(scanner) )
				{
					DScanner_Advance(scanner);
				}
			}
			else if ( DScanner_Consume(scanner, '*') )
			{
				while( !(DScanner_Consume(scanner, '*') && DScanner_Consume(scanner, '/')) || DScanner_IsAtEnd(scanner) )
				{
					if ( Char_IsEOL( DScanner_Peek(scanner) ) ) {
						scanner.line++;
						scanner.currentInLine = 0;
					}
					DScanner_Advance(scanner);
				}
			}
			else
			{
				DScanner_SetError( scanner, "Unterminated comment." );
				return;
			}
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanner.line++;
			scanner.currentInLine = 0;
			break;

		case '\"':
			while ( DScanner_Peek(scanner) != '\"' && !DScanner_IsAtEnd(scanner) )
			{
				if ( Char_IsEOL( DScanner_Peek(scanner) ) ) {
					scanner.line++;
					scanner.currentInLine = 0;
				}
				DScanner_Advance(scanner);
			}

			if ( DScanner_IsAtEnd(scanner) )
			{
				DScanner_SetError( scanner, "Unterminated string." );
				return;
			}

			DScanner_Advance(scanner);

			DScanner_AddToken(scanner, tokenList, TOKEN_STRING);
			break;

		case '\'':
			{
				if ( DScanner_IsAtEnd(scanner) )
				{
					DScanner_SetError( scanner, "Unterminated character" );
					return;
				}
				char character = DScanner_Advance(scanner);
				if ( DScanner_Advance(scanner) != '\'' )
				{
					DScanner_SetError( scanner, "Invalid char literal '%c'", character);
					return;
				}
				DScanner_AddToken(scanner, tokenList, TOKEN_CHARACTER);
			}
			break;

		default:
			if ( Char_IsDigit(c) )
			{
				while ( Char_IsDigit( DScanner_Peek(scanner) ) ) DScanner_Advance(scanner);

				if ( DScanner_Peek(scanner) == '.' && Char_IsDigit( DScanner_PeekNext(scanner) ) )
				{
					DScanner_Advance(scanner);
					while ( Char_IsDigit(DScanner_Peek(scanner)) ) DScanner_Advance(scanner);
					DScanner_Consume(scanner, 'f');
				}

				DScanner_AddToken(scanner, tokenList, TOKEN_NUMBER);
			}
			else if ( Char_IsAlpha(c) )
			{
				while ( Char_IsAlphaNumeric( DScanner_Peek(scanner) ) ) DScanner_Advance(scanner);

				DScanner_AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				DScanner_SetError( scanner, "Unexpected character '%c'.", c );
			}
	}
}

static DTokenList DScan(Arena &arena, const char *text, u32 textSize)
{
	DTokenList tokenList = {};
	tokenList.arena = &arena;
	tokenList.tokens = (DToken*)(arena.base + arena.used);

	DScanner scanner = {};
	scanner.line = 1;
	scanner.hasErrors = false;
	scanner.text = text;
	scanner.textSize = textSize;

	while ( ! DScanner_IsAtEnd(scanner) )
	{
		DScanner_ScanToken(scanner, tokenList);
	}

	DScanner_AddToken(scanner, tokenList, TOKEN_EOF);
	tokenList.valid = !scanner.hasErrors;

	return tokenList;
}

struct DParser
{
	const DTokenList *tokenList;
	Arena *arena;
	u32 nextToken;
	u32 lastToken;
	bool hasErrors;
	bool hasFinished;
	AssetDescriptors *descriptors;
};


static DParser DParser_Init(const DTokenList &tokenList, Arena &arena, AssetDescriptors &descriptors)
{
	const DParser parser = {
		.tokenList = &tokenList,
		.arena = &arena,
		.descriptors = &descriptors,
	};
	return parser;
}

static u32 DParser_RemainingTokens(const DParser &parser)
{
	return parser.tokenList->count - parser.nextToken;
}

static const DToken &DParser_GetPreviousToken( const DParser &parser )
{
	ASSERT(parser.nextToken > 0);
	return parser.tokenList->tokens[parser.nextToken-1];
}

static const DToken &DParser_GetNextToken( const DParser &parser )
{
	ASSERT(parser.nextToken < parser.tokenList->count);
	return parser.tokenList->tokens[parser.nextToken];
}

static bool DParser_HasFinished(const DParser &parser)
{
	const bool hasErrors = parser.hasErrors;
	const bool hasFinished = DParser_GetNextToken(parser).id == TOKEN_EOF;
	return hasErrors || hasFinished;
}

static bool DParser_IsNextToken( const DParser &parser, DTokenId tokenId )
{
	if ( DParser_HasFinished( parser ) ) {
		return tokenId == TOKEN_EOF;
	} else {
		ASSERT(parser.nextToken < parser.tokenList->count);
		return tokenId == parser.tokenList->tokens[parser.nextToken].id;
	}
}

static const DToken &DParser_GetLastToken( const DParser &parser )
{
	return parser.tokenList->tokens[parser.lastToken];
}

static void DParser_SetError(DParser &parser, const char *message)
{
	DToken token = DParser_GetNextToken(parser);
	LOG(Error, "DParser error: %d:%.*s %s\n", token.line, token.lexeme.size, token.lexeme.str, message);
	parser.hasErrors = true;
}

static const DToken &DParser_Consume( DParser &parser )
{
	if ( !DParser_HasFinished( parser ) ) {
		parser.nextToken++;
		parser.lastToken = parser.nextToken > parser.lastToken ? parser.nextToken : parser.lastToken;
		return DParser_GetPreviousToken(parser);
	} else {
		DParser_SetError(parser, "Reached end of file");
		return gNullToken;
	}
}

static bool DParser_TryConsume( DParser &parser, DTokenId tokenId )
{
	if ( DParser_IsNextToken( parser, tokenId ) ) {
		DParser_Consume( parser );
		return true;
	}
	return false;
}

static void DParser_ConsumeUntil( DParser &parser, DTokenId tokenId )
{
	while ( !DParser_IsNextToken( parser, tokenId ) && !DParser_HasFinished( parser ) )
	{
		DParser_Consume( parser );
	}
	while ( DParser_TryConsume( parser, tokenId ) ) {
		// Do nothing, just consume the expected token
	}
}

static const String sMaterialStr = MakeString("Material");
static const String sTextureStr = MakeString("Texture");
static const String sEntityStr = MakeString("Entity");
static const String sAudioClipStr = MakeString("AudioClip");

static void DParseDescriptors(DParser &parser)
{
	while ( !DParser_HasFinished( parser ) )
	{
		if ( DParser_TryConsume(parser, TOKEN_IDENTIFIER) )
		{
			const DToken &token = DParser_GetPreviousToken(parser);
			const String tokenStr = token.lexeme;

			if ( StrEq(tokenStr, sTextureStr) ) {
				// TODO
			} else if ( StrEq(tokenStr, sMaterialStr) ) {
				// TODO
			} else if ( StrEq(tokenStr, sEntityStr) ) {
				// TODO
			} else if ( StrEq(tokenStr, sAudioClipStr) ) {
				// TODO
			}
		}
	}
}

static void DParseDescriptorCounts(DParser &parser)
{
	while ( !DParser_HasFinished( parser ) )
	{
		if ( DParser_TryConsume(parser, TOKEN_IDENTIFIER) )
		{
			const DToken &token = DParser_GetPreviousToken(parser);
			const String tokenStr = token.lexeme;

			if ( StrEq(tokenStr, sTextureStr) ) {
				parser.descriptors->textureDescCount++;
			} else if ( StrEq(tokenStr, sMaterialStr) ) {
				parser.descriptors->materialDescCount++;
			} else if ( StrEq(tokenStr, sEntityStr) ) {
				parser.descriptors->entityDescCount++;
			} else if ( StrEq(tokenStr, sAudioClipStr) ) {
				parser.descriptors->audioClipDescCount++;
			}

			DParser_ConsumeUntil(parser, TOKEN_SEMICOLON);
		}
	}
}

AssetDescriptors ParseDescriptors(const char *filepath, Arena &arena)
{
	AssetDescriptors descriptors = {};
	DataChunk *chunk = PushFile( arena, filepath );

	if ( chunk )
	{
		DTokenList tokenList = DScan(arena, (const char *)chunk->bytes, chunk->size);
		if ( tokenList.valid )
		{
			DParser parser = DParser_Init(tokenList, arena, descriptors);
			DParseDescriptorCounts(parser);

			if (!parser.hasErrors)
			{
				// Reserve memory and reset counts
				descriptors.textureDescs = PushArray(arena, TextureDesc, descriptors.textureDescCount);
				descriptors.textureDescCount = 0;
				descriptors.materialDescs = PushArray(arena, MaterialDesc, descriptors.materialDescCount);
				descriptors.materialDescCount = 0;
				descriptors.entityDescs = PushArray(arena, EntityDesc, descriptors.entityDescCount);
				descriptors.entityDescCount = 0;
				descriptors.audioClipDescs = PushArray(arena, AudioClipDesc, descriptors.audioClipDescCount);
				descriptors.audioClipDescCount = 0;

				parser = DParser_Init(tokenList, arena, descriptors);
				DParseDescriptors(parser);

				if ( parser.hasErrors )
				{
					LOG(Error, "Could not parser tokens: %s\n", filepath);
				}
			}
			else
			{
				LOG(Error, "Could not parse tokens for counts: %s\n", filepath);
			}
		}
		else
		{
			LOG(Error, "Could not scan tokens: %s\n", filepath);
		}
	}
	else
	{
		LOG(Warning, "Could not open file: %s\n", filepath);
	}

	return descriptors;
}

void BuildAssets(const AssetDescriptors &descriptors, const char *filepath, Arena tempArena)
{
	LOG(Info, "Building data\n");

	CreateDirectory( MakePath(ProjectDir, "build").str );
	CreateDirectory( MakePath(ProjectDir, "build/shaders").str );

	CompileShaders();

	FILE *file = fopen(filepath, "wb");
	if ( file )
	{
		const u32 shaderCount = descriptors.shaderDescCount;
		const u32 shadersOffset = sizeof( BinAssetsHeader );
		const u32 shadersSize = shaderCount * sizeof(BinShaderDesc);

		const u32 imageCount = descriptors.textureDescCount;
		const u32 imagesOffset = shadersOffset + shadersSize;
		const u32 imagesSize = imageCount * sizeof(BinImageDesc);

		const u32 audioClipCount = descriptors.audioClipDescCount;
		const u32 audioClipsOffset = imagesOffset + imagesSize;
		const u32 audioClipsSize = audioClipCount * sizeof(BinAudioClipDesc);

		const u32 materialCount = descriptors.materialDescCount;
		const u32 materialsOffset = audioClipsOffset + audioClipsSize;
		const u32 materialsSize = materialCount * sizeof(BinMaterialDesc);

		const u32 entityCount = descriptors.entityDescCount;
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
			const ShaderSourceDesc &shaderSourceDesc = descriptors.shaderDescs[i];

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
			const TextureDesc &textureDesc = descriptors.textureDescs[i];

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
			const AudioClipDesc &audioClipDesc = descriptors.audioClipDescs[i];

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
			const MaterialDesc &materialDesc = descriptors.materialDescs[i];

			BinMaterialDesc &binMaterialDesc = binMaterialDescs[i];
			StrCopy(binMaterialDesc.name, materialDesc.name);
			StrCopy(binMaterialDesc.textureName, materialDesc.textureName);
			StrCopy(binMaterialDesc.pipelineName, materialDesc.pipelineName);
			binMaterialDesc.uvScale = materialDesc.uvScale;
		}

		// Entities
		for (u32 i = 0; i < entityCount; ++i)
		{
			const EntityDesc &entityDesc = descriptors.entityDescs[i];

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

