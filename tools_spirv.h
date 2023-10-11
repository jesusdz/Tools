#ifndef TOOLS_SPIRV_H
#define TOOLS_SPIRV_H

#include "tools.h"

#if SPV_INCLUDE_VULKAN_CORE
#include "vulkan/vk_core.h"
#endif


// NOTE: Check for reference: https://github.com/KhronosGroup/SPIRV-Tools/tree/main/source
// NOTE: Check for reference: https://github.com/KhronosGroup/SPIRV-Tools/blob/main/source/spirv_endian.cpp


// Defines ///////////////////////////////////////////////////////////////////////////////

#define SPV_INDEX_INSTRUCTION 5u
#define SPV_MAX_DESCRIPTOR_SETS 4u
#define SPV_MAX_DESCRIPTORS_PER_SET 8u
#define SPV_MAX_DESCRIPTORS ( SPV_MAX_DESCRIPTOR_SETS * SPV_MAX_DESCRIPTORS_PER_SET )


// Endianness ////////////////////////////////////////////////////////////////////////////

union SpvEndianness
{
	unsigned char values[4];
	struct {
		unsigned int value;
	};
};

static const SpvEndianness SPIRV_ENDIANNESS_LITTLE_ENDIAN = { 4, 3, 2, 1 };
static const SpvEndianness SPIRV_ENDIANNESS_BIG_ENDIAN = { 1, 2, 3, 4 };
static const SpvEndianness SPIRV_MAGIC_LITTLE_ENDIAN = { 0x03, 0x02, 0x23, 0x07 }; // Magic number: 0x07230203
static const SpvEndianness SPIRV_MAGIC_BIG_ENDIAN = { 0x07, 0x23, 0x02, 0x03 }; // Magic number: 0x07230203
static const unsigned int SPIRV_HOST_ENDIANNESS = 0x01020304;

unsigned int SpvFixWord(unsigned int word, SpvEndianness endianness)
{
	const unsigned int swappedWord = (word & 0x000000ff) << 24 | (word & 0x0000ff00) << 8 | (word & 0x00ff0000) >> 8 | (word & 0xff000000) >> 24;
	const unsigned int fixedWord = ( endianness.value == SPIRV_HOST_ENDIANNESS ? word : swappedWord );
	return fixedWord;
}


// Types /////////////////////////////////////////////////////////////////////////////////

enum SpvOp
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

enum SpvExecutionModel
{
	SpvExecutionModelVertex = 0,
	SpvExecutionModelFragment = 4,
};

enum SpvDecoration
{
	SpvDecorationBinding = 33,
	SpvDecorationDescriptorSet = 34,
};

enum SpvStorageClass
{
	SpvStorageClassUniformConstant = 0,
	SpvStorageClassUniform = 2,
};

enum SpvDescriptorType
{
	SpvDescriptorTypeUniformBuffer,
};

enum SpvStageFlags
{
	SpvStageFlagsVertexBit,
};

struct SpvHeader
{
	u32 magic;
	u32 version;
	u32 generator;
	u32 bound;
	u32 schema;
};

struct SpvParser
{
	// Stream
	u32 *words;
	u32 wordCount;

	// Header
	SpvHeader *header;

	// State
	u32 *instructionWords;
	u16 instructionOpCode;
	u16 instructionWordCount;
};

struct SpvDescriptor
{
	u32 binding;
	u32 descriptorSet;
	u32 descriptorType;
	u32 stageFlags;
};

struct SpvDescriptorSet
{
	SpvDescriptor descriptors[SPV_MAX_DESCRIPTORS_PER_SET];
};

struct SpvModule
{
	SpvHeader *header;
};


// Parsing ///////////////////////////////////////////////////////////////////////////////

u8 SpvVersionMajor(u32 version)
{
	const u8 major = ( version & 0x00ff0000 ) >> 16;
	return major;
}

u8 SpvVersionMinor(u32 version)
{
	const u8 minor = ( version & 0x0000ff00 ) >> 8;
	return minor;
}

u16 SpvGetWordCount(u32 firstInstructionWord)
{
	const u16 wordCount = ( firstInstructionWord >> 16 ) & 0xffff;
	return wordCount;
}

u16 SpvGetOpCode(u32 firstInstructionWord)
{
	const u16 opCode = ( firstInstructionWord >> 0 ) & 0xffff;
	return opCode;
}

void SpvParserUpdateInstruction(SpvParser *parser)
{
	const u32 firstInstructionWord = *parser->instructionWords;
	parser->instructionOpCode = SpvGetOpCode(firstInstructionWord);
	parser->instructionWordCount = SpvGetWordCount(firstInstructionWord);
}

void SpvParserRewind(SpvParser *parser)
{
	parser->instructionWords = parser->words + SPV_INDEX_INSTRUCTION;
	SpvParserUpdateInstruction(parser);
}

u32 SpvParserInstructionOpCode(SpvParser *parser)
{
	return parser->instructionOpCode;
}

u32 SpvParserInstructionWordCount(SpvParser *parser)
{
	return parser->instructionWordCount;
}

void SpvParserAdvance(SpvParser *parser)
{
	parser->instructionWords += parser->instructionWordCount;
	SpvParserUpdateInstruction(parser);
}

bool SpvParserFinished(SpvParser *parser)
{
	return parser->instructionWords - parser->words >= parser->wordCount;
}

bool SpvParseInstruction(SpvParser *parser, SpvModule *spirv)
{
	const u32 *words = parser->instructionWords;
	const u16 opCode = SpvParserInstructionOpCode(parser);
	const u16 wordCount = SpvParserInstructionWordCount(parser);

	switch (opCode)
	{
		case SpvOpSource:
			{
				const u32 sourceLanguage = words[1];
				const u32 version = words[2];
				LOG(Info, "      SpvOpSource - sourceLanguage: %u - version: %u", sourceLanguage, version);
				if (wordCount > 3) {
					const u32 fileId = words[3];
					LOG(Info, " - fileId: %u", fileId);
				}
				if (wordCount > 4) {
					const char *source = (const char *)&words[4];
					LOG(Info, " - source: %s", source);
				}
				LOG(Info, "\n");
			}
			break;
		case SpvOpSourceExtension: LOG(Info, "SpvOpSourceExtension - wordCount: %u\n", wordCount); break;
		case SpvOpName:
			{
				const u32 targetId = words[1];
				const char *name = (const char *)&words[2];
				LOG(Info, "      SpvOpName - targetId: %u - name: %s\n", targetId, name);
			}
			break;
		case SpvOpMemberName:
			{
				const u32 typeId = words[1];
				const u32 member = words[2];
				const char *name = (const char *)&words[3];
				LOG(Info, "      SpvOpMemberName - typeId: %u - member: %u - name: %s\n", typeId, member, name);
			}
			break;
		case SpvOpExtInstImport: LOG(Info, "SpvOpExtInstImport - wordCount: %u\n", wordCount); break;
		case SpvOpMemoryModel:
			{
				const u32 addressingModel = words[1];
				const u32 memoryModel = words[2];
				LOG(Info, "      SpvOpMemoryModel - addressingModel: %u - memoryModel: %u\n", addressingModel, memoryModel);
			}
			break;
		case SpvOpEntryPoint:
			{
				const u32 executionModel = words[1];
				const u32 entryPointId = words[2];
				const char *name = (const char *)&words[3];
				LOG(Info, "      SpvOpEntryPoint - executionModel: %u - entryPointId: %u - name: %s - ...\n", executionModel, entryPointId, name);
			}
			break;
		case SpvOpExecutionMode:
			{
				const u32 entryPointId = words[1];
				const u32 executionModel = words[2];
				LOG(Info, "      SpvOpExecutionMode - entryPointId: %u - executionModel: %u - ...\n", entryPointId, executionModel);
			}
			break;
		case SpvOpCapability:
			{
				const u32 capability = words[1];
				LOG(Info, "      SpvOpCapability - capability: %u\n", capability);
			}
			break;
		case SpvOpTypeVoid:
			{
				const u32 resultId = words[1];
				LOG(Info, "%%%u = SpvOpTypeVoid\n", resultId);
			}
			break;
		case SpvOpTypeBool:
			{
				const u32 resultId = words[1];
				LOG(Info, "%%%u = SpvOpTypeBool\n", resultId);
			}
			break;
		case SpvOpTypeInt:
			{
				const u32 resultId = words[1];
				const u32 width = words[2];
				const u32 sign = words[3];
				LOG(Info, "%%%u = SpvOpTypeInt - width: %u - sign: %u\n", resultId, width, sign);
			}
			break;
		case SpvOpTypeFloat:
			{
				const u32 resultId = words[1];
				const u32 width = words[2];
				LOG(Info, "%%%u = SpvOpTypeFloat - width: %u\n", resultId, width);
			}
			break;
		case SpvOpTypeVector:
			{
				const u32 resultId = words[1];
				const u32 componentTypeId = words[2];
				const u32 componentCount = words[3];
				LOG(Info, "%%%u = SpvOpTypeVector - componentTypeId: %u - componentCount: %u\n", resultId, componentTypeId, componentCount);
			}
			break;
		case SpvOpTypeMatrix:
			{
				const u32 resultId = words[1];
				const u32 columnTypeId = words[2];
				const u32 columnCount = words[3];
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
				const u32 resultId = words[1];
				LOG(Info, "%%%u = SpvOpTypeStruct", resultId);
				for (u32 i = 2; i < wordCount; ++i)
				{
					const u32 typeId = words[i];
					LOG(Info, " - typeId: %u", typeId);
				}
				if (wordCount > 2) LOG(Info, "\n");
			}
			break;
		case SpvOpTypeOpaque: LOG(Info, "SpvOpTypeOpaque - wordCount: %u\n", wordCount); break;
		case SpvOpTypePointer:
			{
				const u32 resultId = words[1];
				const u32 storageClass = words[2];
				const u32 typeId = words[3];
				LOG(Info, "%%%u = SpvOpTypePointer - storageClass: %u - typeId: %u\n", resultId, storageClass, typeId);
				if (storageClass == SpvStorageClassUniform)
				{
					// TODO: Save this id
				}
			}
			break;
		case SpvOpTypeFunction:
			{
				const u32 resultId = words[1];
				const u32 returnTypeId = words[2];
				LOG(Info, "%%%u = SpvOpTypeFunction - returnTypeId: %u\n", resultId, returnTypeId);
				for (u32 i = 3; i < wordCount; ++i)
				{
					const u32 typeId = words[i];
					LOG(Info, " - typeId: %u", typeId);
				}
				if (wordCount > 3) LOG(Info, "\n");
			}
			break;
		case SpvOpConstant:
			{
				const u32 resultTypeId = words[1];
				const u32 resultId = words[2];
				const u32 value = words[3];
				LOG(Info, "%%%u = SpvOpConstant - returnTypeId: %u - value: %u\n", resultId, resultTypeId, value);
			}
			break;
		case SpvOpFunction: LOG(Info, "SpvOpFunction - wordCount: %u\n", wordCount); break;
		case SpvOpFunctionEnd: LOG(Info, "SpvOpFunctionEnd - wordCount: %u\n", wordCount); break;
		case SpvOpFunctionCall: LOG(Info, "SpvOpFunctionCall - wordCount: %u\n", wordCount); break;
		case SpvOpVariable:
			{
				const u32 resultTypeId = words[1];
				const u32 resultId = words[2];
				const u32 storageClass = words[3];
				LOG(Info, "%%%u = SpvOpVariable - resultTypeId: %u - storageClass: %u - ...\n", resultId, resultTypeId, storageClass);
			}
			break;
		case SpvOpLoad: LOG(Info, "SpvOpLoad - wordCount: %u\n", wordCount); break;
		case SpvOpStore: LOG(Info, "SpvOpStore - wordCount: %u\n", wordCount); break;
		case SpvOpAccessChain: LOG(Info, "SpvOpAccessChain - wordCount: %u\n", wordCount); break;
		case SpvOpDecorate:
			{
				const u32 targetId = words[1];
				const u32 decoration = words[2];
				LOG(Info, "      SpvOpDecorate - targetId: %u - decoration: %u...\n", targetId, decoration);
			}
			break;
		case SpvOpMemberDecorate:
			{
				const u32 structyreTypeId = words[1];
				const u32 member = words[2];
				const u32 decoration = words[3];
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
			LOG(Debug, "Unhandled instruction - opCode: %u - wordCount: %u\n", opCode, wordCount);
			//return false;
		}
	};

	return true;
}

bool SpvParse(SpvParser *parser, SpvModule *spirv)
{
	spirv->header = parser->header;

	while ( !SpvParserFinished(parser) )
	{
		if ( !SpvParseInstruction(parser, spirv) )
		{
			return false;
		}
		SpvParserAdvance(parser);
	}

	return true;
}

void SpvParseEntryPoint(SpvParser *parser, u32 *executionModel)
{
	const u32 *words = parser->instructionWords;
	const u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpEntryPoint )
	{
		*executionModel = words[1];

		if ( *executionModel == SpvExecutionModelVertex ) LOG(Info, "Execution model: Vertex\n");
		else if ( *executionModel == SpvExecutionModelFragment ) LOG(Info, "Execution model: Fragment\n");
		else LOG(Info, "Execution model: Unknown\n");
	}
}

void SpvParseDescriptorId(SpvParser *parser, u32 descriptorIds[SPV_MAX_DESCRIPTORS], u32 *descriptorIdCount)
{
	const u32 *words = parser->instructionWords;
	const u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpVariable )
	{
		const u32 resultTypeId = words[1];
		const u32 resultId = words[2];
		const u32 storageClass = words[3];
		if ( storageClass == SpvStorageClassUniform || storageClass == SpvStorageClassUniformConstant )
		{
			LOG(Info, "Found Uniform with id %u and type %u\n", resultId, resultTypeId);
			const u32 descriptorId = *descriptorIdCount;
			descriptorIds[descriptorId] = resultId;
			*descriptorIdCount = descriptorId + 1;
		}
	}
}

void SpvParseDescriptor(SpvParser *parser, u32 descriptorIds[SPV_MAX_DESCRIPTORS], u32 *descriptorIdCount)
{
	const u32 *words = parser->instructionWords;
	const u32 opCode = SpvParserInstructionOpCode(parser);

	if ( opCode == SpvOpDecorate )
	{
		const u32 targetId = words[1];
		const u32 decoration = words[2];

		if ( decoration == SpvDecorationBinding )
		{
			const u32 binding = words[3];
		}
		else if ( decoration == SpvDecorationDescriptorSet )
		{
			const u32 descriptorSet = words[3];
		}
	}
}

bool SpvParse(SpvParser *parser, SpvDescriptorSet descriptorSets[SPV_MAX_DESCRIPTOR_SETS])
{
	u32 executionModel = 0;
	u32 descriptorIds[SPV_MAX_DESCRIPTORS];
	u32 descriptorIdCount = 0;

	SpvParserRewind(parser);

	while ( !SpvParserFinished(parser) )
	{
		const u32 opCode = SpvParserInstructionOpCode(parser);
		switch (opCode) {
			case SpvOpEntryPoint: SpvParseEntryPoint(parser, &executionModel); break;
			case SpvOpVariable: SpvParseDescriptorId(parser, descriptorIds, &descriptorIdCount); break;
		};
		SpvParserAdvance(parser);
	}

	SpvParserRewind(parser);

	while ( !SpvParserFinished(parser) )
	{
		SpvParseDescriptor(parser, descriptorIds, &descriptorIdCount);
		SpvParserAdvance(parser);
	}

	return true;
}

SpvParser SpvParserInit( u32 *words, u32 wordCount )
{
	SpvHeader *header = (SpvHeader*)words;

	// Check module endianness based on the magic number
	SpvEndianness endianness;
	if ( header->magic == SPIRV_MAGIC_LITTLE_ENDIAN.value ) {
		endianness = SPIRV_ENDIANNESS_LITTLE_ENDIAN;
	} else if ( header->magic == SPIRV_MAGIC_BIG_ENDIAN.value ) {
		endianness = SPIRV_ENDIANNESS_BIG_ENDIAN;
	} else {
		ASSERT(0 && "Invalid SPIRV word stream.");
	}

	// Fix endianness
	for (u32 i = 0; i < wordCount; ++i) {
		words[i] = SpvFixWord( words[i], endianness );
	}

	// Skip the header after parsing it
	SpvParser parser = { words, wordCount };
	SpvParserRewind( &parser );
	return parser;
}



#endif // #if TOOLS_SPIRV_H

