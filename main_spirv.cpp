#include "tools.h"


// NOTE: Check for reference: https://github.com/KhronosGroup/SPIRV-Tools/tree/main/source
// NOTE: Check for reference: https://github.com/KhronosGroup/SPIRV-Tools/blob/main/source/spirv_endian.cpp

// Endianness ////////////////////////////////////////////////////////////////////////////

union SpirvEndianness
{
	unsigned char values[4];
	struct {
		unsigned int value;
	};
};

static const SpirvEndianness SPIRV_ENDIANNESS_LITTLE_ENDIAN = { 4, 3, 2, 1 };
static const SpirvEndianness SPIRV_ENDIANNESS_BIG_ENDIAN = { 1, 2, 3, 4 };
static const SpirvEndianness SPIRV_MAGIC_LITTLE_ENDIAN = { 0x03, 0x02, 0x23, 0x07 }; // Magic number: 0x07230203
static const unsigned int SPIRV_HOST_ENDIANNESS = 0x01020304;

unsigned int SpirvFixWord(unsigned int word, SpirvEndianness endianness)
{
	const unsigned int swappedWord = (word & 0x000000ff) << 24 | (word & 0x0000ff00) << 8 | (word & 0x00ff0000) >> 8 | (word & 0xff000000) >> 24;
	const unsigned int fixedWord = ( endianness.value == SPIRV_HOST_ENDIANNESS ? word : swappedWord );
	return fixedWord;
}


// Types /////////////////////////////////////////////////////////////////////////////////

#define SPV_INDEX_INSTRUCTION 5u


// Types /////////////////////////////////////////////////////////////////////////////////

enum SpvOpCode
{
	SpvOpSource = 3,
	SpvOpSourceExtension = 4,
	SpvOpName = 5,
	SpvOpMemberName = 6,
	SpvOpExtInstImport = 11,
	SpvOpMemoryModel = 14,
	SpvOpEntryPoint = 15,
	SpvOpExecutionMode = 16,
	SpvOpCapability = 17,
	SpvOpTypeVoid = 19,
	SpvOpTypeBool = 20,
	SpvOpTypeInt = 21,
	SpvOpTypeFloat = 22,
	SpvOpTypeVector = 23,
	SpvOpTypeMatrix = 24,
	SpvOpTypeImage = 25,
	SpvOpTypeSampler = 26,
	SpvOpTypeSampledImage = 27,
	SpvOpTypeArray = 28,
	SpvOpTypeRuntimeArray = 29,
	SpvOpTypeStruct = 30,
	SpvOpTypeOpaque = 31,
	SpvOpTypePointer = 32,
	SpvOpTypeFunction = 33,
	SpvOpConstant = 43,
	SpvOpFunction = 54,
	SpvOpFunctionEnd = 56,
	SpvOpFunctionCall = 57,
	SpvOpVariable = 59,
	SpvOpLoad = 61,
	SpvOpStore = 62,
	SpvOpAccessChain = 65,
	SpvOpDecorate = 71,
	SpvOpMemberDecorate = 72,
	SpvOpVectorShuffle = 79,
	SpvOpCompositeConstruct = 80,
	SpvOpCompositeExtract = 81,
	SpvOpVectorTimesMatrix = 144,
	SpvOpMatrixTimesVector = 145,
	SpvOpMatrixTimesMatrix = 146,
	SpvOpOuterProduct = 147,
	SpvOpDot = 148,
	SpvOpLabel = 248,
	SpvOpReturn = 253,
};

struct SpirvHeader
{
	u32 magic;
	u32 version;
	u32 generator;
	u32 bound;
	u32 schema;
};

struct SpirvModule
{
	SpirvHeader *header;
	SpirvEndianness endianness;
};

struct SpirvParser
{
	u32 *words;
	u32 wordCount;
	u32 wordIndex;
};


// Parsing ///////////////////////////////////////////////////////////////////////////////

bool SpirvParserFinished(SpirvParser *parser)
{
	return parser->wordIndex >= parser->wordCount;
}

bool SpirvParseInstruction(SpirvParser *parser, SpirvModule *spirv)
{
	const u32 instructionOffset = parser->wordIndex;

	const u32 word0 = parser->words[instructionOffset + 0];
	const u32 wordCount = ( word0 >> 16 ) & 0xffff;
	const u32 opCode = ( word0 >> 0 ) & 0xffff;


	switch (opCode)
	{
		case SpvOpSource: break;
		case SpvOpSourceExtension: break;
		case SpvOpName: break;
		case SpvOpMemberName: break;
		case SpvOpExtInstImport: break;
		case SpvOpMemoryModel: break;
		case SpvOpEntryPoint: break;
		case SpvOpExecutionMode: break;
		case SpvOpCapability: break;
		case SpvOpTypeVoid: break;
		case SpvOpTypeBool: break;
		case SpvOpTypeInt: break;
		case SpvOpTypeFloat: break;
		case SpvOpTypeVector: break;
		case SpvOpTypeMatrix: break;
		case SpvOpTypeImage: break;
		case SpvOpTypeSampler: break;
		case SpvOpTypeSampledImage: break;
		case SpvOpTypeArray: break;
		case SpvOpTypeRuntimeArray: break;
		case SpvOpTypeStruct: break;
		case SpvOpTypeOpaque: break;
		case SpvOpTypePointer: break;
		case SpvOpTypeFunction: break;
		case SpvOpConstant: break;
		case SpvOpFunction: break;
		case SpvOpFunctionEnd: break;
		case SpvOpFunctionCall: break;
		case SpvOpVariable: break;
		case SpvOpLoad: break;
		case SpvOpStore: break;
		case SpvOpAccessChain: break;
		case SpvOpDecorate: break;
		case SpvOpMemberDecorate: break;
		case SpvOpVectorShuffle: break;
		case SpvOpCompositeConstruct:break;
		case SpvOpCompositeExtract: break;
		case SpvOpVectorTimesMatrix: break;
		case SpvOpMatrixTimesVector: break;
		case SpvOpMatrixTimesMatrix: break;
		case SpvOpOuterProduct: break;
		case SpvOpDot: break;
		case SpvOpLabel: break;
		case SpvOpReturn: break;
		default:
		{
			LOG(Debug, "- opCode: %u - wordCount: %u\n", opCode, wordCount);
			return false;
		}
	};

	parser->wordIndex += wordCount;
	return true;
}

bool SpirvParse(SpirvParser *parser, SpirvModule *spirv)
{
	SpirvHeader *header = (SpirvHeader*)parser->words;

	// Check module endianness based on the magic number
	SpirvEndianness endianness;
	if ( header->magic == SPIRV_MAGIC_LITTLE_ENDIAN.value ) {
		endianness = SPIRV_ENDIANNESS_LITTLE_ENDIAN;
	} else {
		endianness = SPIRV_ENDIANNESS_BIG_ENDIAN;
	}

	// Fix endianness
	for (u32 i = 0; i < parser->wordCount; ++i) {
		parser->words[i] = SpirvFixWord( parser->words[i], endianness );
	}

	// Set output values
	spirv->header = header;
	spirv->endianness = endianness;

	parser->wordIndex = SPV_INDEX_INSTRUCTION;

	while ( !SpirvParserFinished(parser) )
	{
		if ( !SpirvParseInstruction(parser, spirv) )
		{
			return false;
		}
	}

	return true;
}



// Getters ///////////////////////////////////////////////////////////////////////////////

u8 SpirvVersionMajor(u32 version)
{
	const u8 major = ( version & 0x00ff0000 ) >> 16;
	return major;
}

u8 SpirvVersionMinor(u32 version)
{
	const u8 minor = ( version & 0x0000ff00 ) >> 8;
	return minor;
}



// Main program //////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	// Get shader filename
	if ( argc != 2 ) {
		LOG(Error, "Usage: %s <spirv_file.spv>\n", argv[0]);
		return -1;
	}
	const char *shaderFile = argv[1];

	// Create a memory arena
	u32 memorySize = MB(1);
	byte *memory = (byte*)AllocateVirtualMemory(memorySize);
	Arena arena = MakeArena(memory, memorySize);

	// Get shader file memory blob
	DataChunk *chunk = PushFile(arena, shaderFile);
	if (!chunk) {
		LOG(Error, "Error reading %s\n", shaderFile);
		return -1;
	}

	// Parse spirv bytecode
	u32 *words = (u32*)chunk->data;
	const u32 wordCount = chunk->size / 4;
	ASSERT( chunk->size % 4 == 0 );
	SpirvParser spirvParser = { words, wordCount };
	SpirvModule spirv = {};
	const bool ok = SpirvParse(&spirvParser, &spirv);

	// Print endianness information
	LOG(Info, "Endianness\n");
	LOG(Info, "- Host endianness: %s endian\n", SPIRV_HOST_ENDIANNESS == SPIRV_ENDIANNESS_LITTLE_ENDIAN.value ? "little" : "big" );
	LOG(Info, "- Spirv endianness: %s endian\n", spirv.endianness.value == SPIRV_ENDIANNESS_LITTLE_ENDIAN.value ? "little" : "big");

	// Log spirv module header information
	LOG(Info, "\nHeader info\n");
	LOG(Info, "- magic: %x\n", spirv.header->magic);
	LOG(Info, "- major: %d\n", SpirvVersionMajor(spirv.header->version));
	LOG(Info, "- minor: %d\n", SpirvVersionMinor(spirv.header->version));
	LOG(Info, "- generator: %d\n", spirv.header->generator);
	LOG(Info, "- bound: %d\n", spirv.header->bound);
	LOG(Info, "- schema: %d\n", spirv.header->schema);

	LOG(Info, "\nParsed information\n");
	LOG(Info, "- Result: %s\n", ok ? "Success" : "Fail");

	return 0;
}

