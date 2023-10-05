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
	const u32 *instructionWords = parser->words + instructionOffset;

	const u32 word0 = instructionWords[0];
	const u32 wordCount = ( word0 >> 16 ) & 0xffff;
	const u32 opCode = ( word0 >> 0 ) & 0xffff;


	switch (opCode)
	{
		case SpvOpSource:
			{
				const u32 sourceLanguage = instructionWords[1];
				const u32 version = instructionWords[2];
				LOG(Info, "      SpvOpSource - sourceLanguage: %u - version: %u", sourceLanguage, version);
				if (wordCount > 3) {
					const u32 fileId = instructionWords[3];
					LOG(Info, " - fileId: %u", fileId);
				}
				if (wordCount > 4) {
					const char *source = (const char *)&instructionWords[4];
					LOG(Info, " - source: %s", source);
				}
				LOG(Info, "\n");
			}
			break;
		case SpvOpSourceExtension: LOG(Info, "SpvOpSourceExtension - wordCount: %u\n", wordCount); break;
		case SpvOpName:
			{
				const u32 targetId = instructionWords[1];
				const char *name = (const char *)&instructionWords[2];
				LOG(Info, "      SpvOpName - targetId: %u - name: %s\n", targetId, name);
			}
			break;
		case SpvOpMemberName:
			{
				const u32 typeId = instructionWords[1];
				const u32 member = instructionWords[2];
				const char *name = (const char *)&instructionWords[3];
				LOG(Info, "      SpvOpMemberName - typeId: %u - member: %u - name: %s\n", typeId, member, name);
			}
			break;
		case SpvOpExtInstImport: LOG(Info, "SpvOpExtInstImport - wordCount: %u\n", wordCount); break;
		case SpvOpMemoryModel:
			{
				const u32 addressingModel = instructionWords[1];
				const u32 memoryModel = instructionWords[2];
				LOG(Info, "      SpvOpMemoryModel - addressingModel: %u - memoryModel: %u\n", addressingModel, memoryModel);
			}
			break;
		case SpvOpEntryPoint:
			{
				const u32 executionModel = instructionWords[1];
				const u32 entryPointId = instructionWords[2];
				const char *name = (const char *)&instructionWords[3];
				LOG(Info, "      SpvOpEntryPoint - executionModel: %u - entryPointId: %u - name: %s - ...\n", executionModel, entryPointId, name);
			}
			break;
		case SpvOpExecutionMode:
			{
				const u32 entryPointId = instructionWords[1];
				const u32 executionModel = instructionWords[2];
				LOG(Info, "      SpvOpExecutionMode - entryPointId: %u - executionModel: %u - ...\n", entryPointId, executionModel);
			}
			break;
		case SpvOpCapability:
			{
				const u32 capability = instructionWords[1];
				LOG(Info, "      SpvOpCapability - capability: %u\n", capability);
			}
			break;
		case SpvOpTypeVoid:
			{
				const u32 resultId = instructionWords[1];
				LOG(Info, "%%%u = SpvOpTypeVoid\n", resultId);
			}
			break;
		case SpvOpTypeBool:
			{
				const u32 resultId = instructionWords[1];
				LOG(Info, "%%%u = SpvOpTypeBool\n", resultId);
			}
			break;
		case SpvOpTypeInt:
			{
				const u32 resultId = instructionWords[1];
				const u32 width = instructionWords[2];
				const u32 sign = instructionWords[3];
				LOG(Info, "%%%u = SpvOpTypeInt - width: %u - sign: %u\n", resultId, width, sign);
			}
			break;
		case SpvOpTypeFloat:
			{
				const u32 resultId = instructionWords[1];
				const u32 width = instructionWords[2];
				LOG(Info, "%%%u = SpvOpTypeFloat - width: %u\n", resultId, width);
			}
			break;
		case SpvOpTypeVector:
			{
				const u32 resultId = instructionWords[1];
				const u32 componentTypeId = instructionWords[2];
				const u32 componentCount = instructionWords[3];
				LOG(Info, "%%%u = SpvOpTypeVector - componentTypeId: %u - componentCount: %u\n", resultId, componentTypeId, componentCount);
			}
			break;
		case SpvOpTypeMatrix:
			{
				const u32 resultId = instructionWords[1];
				const u32 columnTypeId = instructionWords[2];
				const u32 columnCount = instructionWords[3];
				LOG(Info, "%%%u = SpvOpTypeMatrix - columnTypeId: %u - columnCount: %u\n", resultId, columnTypeId, columnCount);
			}
			break;
		case SpvOpTypeImage: LOG(Info, "SpvOpTypeImage - wordCount: %u\n", wordCount); break;
		case SpvOpTypeSampler: LOG(Info, "SpvOpTypeSampler - wordCount: %u\n", wordCount); break;
		case SpvOpTypeSampledImage: LOG(Info, "SpvOpTypeSampledImage - wordCount: %u\n", wordCount); break;
		case SpvOpTypeArray: LOG(Info, "SpvOpTypeArray - wordCount: %u\n", wordCount); break;
		case SpvOpTypeRuntimeArray: LOG(Info, "SpvOpTypeRuntimeArray - wordCount: %u\n", wordCount); break;
		case SpvOpTypeStruct:
			{
				const u32 resultId = instructionWords[1];
				LOG(Info, "%%%u = SpvOpTypeStruct", resultId);
				for (u32 i = 2; i < wordCount; ++i)
				{
					const u32 typeId = instructionWords[i];
					LOG(Info, " - typeId: %u", typeId);
				}
				if (wordCount > 2) LOG(Info, "\n");
			}
			break;
		case SpvOpTypeOpaque: LOG(Info, "SpvOpTypeOpaque - wordCount: %u\n", wordCount); break;
		case SpvOpTypePointer:
			{
				const u32 resultId = instructionWords[1];
				const u32 storageClass = instructionWords[2];
				const u32 typeId = instructionWords[3];
				LOG(Info, "%%%u = SpvOpTypePointer - storageClass: %u - typeId: %u\n", resultId, storageClass, typeId);
			}
			break;
		case SpvOpTypeFunction:
			{
				const u32 resultId = instructionWords[1];
				const u32 returnTypeId = instructionWords[2];
				LOG(Info, "%%%u = SpvOpTypeFunction - returnTypeId: %u\n", resultId, returnTypeId);
				for (u32 i = 3; i < wordCount; ++i)
				{
					const u32 typeId = instructionWords[i];
					LOG(Info, " - typeId: %u", typeId);
				}
				if (wordCount > 3) LOG(Info, "\n");
			}
			break;
		case SpvOpConstant:
			{
				const u32 resultTypeId = instructionWords[1];
				const u32 resultId = instructionWords[2];
				const u32 value = instructionWords[3];
				LOG(Info, "%%%u = SpvOpConstant - returnTypeId: %u - value: %u\n", resultId, resultTypeId, value);
			}
			break;
		case SpvOpFunction: LOG(Info, "SpvOpFunction - wordCount: %u\n", wordCount); break;
		case SpvOpFunctionEnd: LOG(Info, "SpvOpFunctionEnd - wordCount: %u\n", wordCount); break;
		case SpvOpFunctionCall: LOG(Info, "SpvOpFunctionCall - wordCount: %u\n", wordCount); break;
		case SpvOpVariable:
			{
				const u32 resultTypeId = instructionWords[1];
				const u32 resultId = instructionWords[2];
				const u32 storageClass = instructionWords[3];
				LOG(Info, "%%%u = SpvOpVariable - resultTypeId: %u - storageClass: %u - ...\n", resultId, resultTypeId, storageClass);
			}
			break;
		case SpvOpLoad: LOG(Info, "SpvOpLoad - wordCount: %u\n", wordCount); break;
		case SpvOpStore: LOG(Info, "SpvOpStore - wordCount: %u\n", wordCount); break;
		case SpvOpAccessChain: LOG(Info, "SpvOpAccessChain - wordCount: %u\n", wordCount); break;
		case SpvOpDecorate:
			{
				const u32 targetId = instructionWords[1];
				const u32 decoration = instructionWords[2];
				LOG(Info, "      SpvOpDecorate - targetId: %u - decoration: %u...\n", targetId, decoration);
			}
			break;
		case SpvOpMemberDecorate:
			{
				const u32 structyreTypeId = instructionWords[1];
				const u32 member = instructionWords[2];
				const u32 decoration = instructionWords[3];
				LOG(Info, "      SpvOpMemberDecorate - structyreTypeId: %u - member: %u - decoration: %u...\n", structyreTypeId, member, decoration);
			}
			break;
		case SpvOpVectorShuffle: LOG(Info, "SpvOpVectorShuffle - wordCount: %u\n", wordCount); break;
		case SpvOpCompositeConstruct: LOG(Info, "SpvOpCompositeConstruct - wordCount: %u\n", wordCount); break;
		case SpvOpCompositeExtract: LOG(Info, "SpvOpCompositeExtract - wordCount: %u\n", wordCount); break;
		case SpvOpVectorTimesMatrix: LOG(Info, "SpvOpVectorTimesMatrix - wordCount: %u\n", wordCount); break;
		case SpvOpMatrixTimesVector: LOG(Info, "SpvOpMatrixTimesVector - wordCount: %u\n", wordCount); break;
		case SpvOpMatrixTimesMatrix: LOG(Info, "SpvOpMatrixTimesMatrix - wordCount: %u\n", wordCount); break;
		case SpvOpOuterProduct: LOG(Info, "SpvOpOuterProduct - wordCount: %u\n", wordCount); break;
		case SpvOpDot: LOG(Info, "SpvOpDot - wordCount: %u\n", wordCount); break;
		case SpvOpLabel: LOG(Info, "SpvOpLabel - wordCount: %u\n", wordCount); break;
		case SpvOpReturn: LOG(Info, "SpvOpReturn - wordCount: %u\n", wordCount); break;
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

