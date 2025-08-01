/*
 * tools.h
 * Author: Jesus Diaz Garcia
 *
 * Single file library with several utilities among the following:
 * - Platform identification
 * - Assertions, debugging, errors, logging
 * - Aliases for sized types
 * - Intrinsics
 * - Strings
 * - Hashing
 * - Memory allocators
 * - File/directory/path management
 * - Process execution
 * - Dynamic library loading
 * - Mathematics
 * - Clock / timing
 * - Window creation
 * - Input handling (mouse and keyboard)
 * - Audio
 */

#ifndef TOOLS_H
#define TOOLS_H

////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform definitions

#if _WIN32
#	define PLATFORM_WINDOWS 1
#elif __ANDROID__
#	define PLATFORM_ANDROID 1
#elif __linux__
#	define PLATFORM_LINUX 1
#elif __APPLE__
#	define PLATFORM_APPLE 1
#else
#	error "Unsupported platform"
#endif


#if PLATFORM_WINDOWS || PLATFORM_LINUX
#define USE_IMGUI 0
#endif


#if PLATFORM_WINDOWS

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WindowsX.h>
#include <xinput.h>
#include <mmsystem.h> // audio
#include <dsound.h>   // audio
#include <direct.h>   // _getcwd
#include <intrin.h>   // _WriteBarrier
#elif PLATFORM_LINUX || PLATFORM_ANDROID
#include <time.h>     // TODO: Find out if this header belongs to the C runtime library...
#include <sys/stat.h> // stat
#include <fcntl.h>    // open
#include <unistd.h>   // read, close, getcwd
#include <string.h>   // strerror_r
#include <errno.h>    // errno
#include <sys/mman.h> // mmap
#include <dirent.h>   // opendir/readdir/closedir
#include <pthread.h>
#include <semaphore.h>
#endif

#if PLATFORM_LINUX || PLATFORM_ANDROID
#include <sys/wait.h>
#include <spawn.h>
#include <dlfcn.h>
#endif

#if PLATFORM_LINUX
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <linux/ioctl.h> // ioctl
#include <linux/input.h> // input_event
#endif

#if PLATFORM_ANDROID
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#endif


#include <stdio.h>  // printf
#include <stdarg.h>
#include <math.h>
// TODO: Remove C runtime library includes. But first...
// TODO: Remove calls to printf.



////////////////////////////////////////////////////////////////////////////////////////////////////
// Useful defines

#define internal static

#define KB(x) (1024ul * x)
#define MB(x) (1024ul * KB(x))
#define GB(x) (1024ul * MB(x))
#define TB(x) (1024ul * GB(x))

#define MAX_PATH_LENGTH 512

#if PLATFORM_ANDROID
#define Debug ANDROID_LOG_DEBUG
#define Info ANDROID_LOG_INFO
#define Warning ANDROID_LOG_WARN
#define Error ANDROID_LOG_ERROR
#define LOG(channel, fmt, ...) ((void)__android_log_print(channel, "tools", fmt, ##__VA_ARGS__))
#else
#define Debug 0
#define Info 1
#define Warning 2
#define Error 3
#define LOG(channel, fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

#if PLATFORM_ANDROID
#define QUIT_ABNORMALLY() __builtin_trap();
#else
#define QUIT_ABNORMALLY() *((int*)0) = 0;
#endif

#define ASSERT(expression) if ( !(expression) ) { \
		LOG(Error, "%s", "Assertion failed: ASSERT( " #expression " )\n" ); \
		QUIT_ABNORMALLY(); \
	}
#define INVALID_CODE_PATH() ASSERT(0 && "Invalid code path")
#define INVALID_CODE_PATH_MSG(message) ASSERT(0 && message)
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))

#define CT_ASSERT3(expression, number) static int ct_assert_##number[(expression) ? 1 : -1]
#define CT_ASSERT2(expression, number) CT_ASSERT3(expression, number)
#define CT_ASSERT(expression) CT_ASSERT2(expression, __COUNTER__)



////////////////////////////////////////////////////////////////////////////////////////////////////
// Sized types

typedef char i8;
typedef short int i16;
typedef int i32;
typedef long long int i64;
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef	unsigned char byte;

CT_ASSERT(sizeof(i8) == 1);
CT_ASSERT(sizeof(i16) == 2);
CT_ASSERT(sizeof(i32) == 4);
CT_ASSERT(sizeof(i64) == 8);
CT_ASSERT(sizeof(u8) == 1);
CT_ASSERT(sizeof(u16) == 2);
CT_ASSERT(sizeof(u32) == 4);
CT_ASSERT(sizeof(u64) == 8);
CT_ASSERT(sizeof(f32) == 4);
CT_ASSERT(sizeof(f64) == 8);
CT_ASSERT(sizeof(byte) == 1);

#define I8_MAX 128
#define U8_MAX 255
#define I16_MAX 32767
#define U16_MAX 65535
#define I32_MAX 2147483647
#define U32_MAX 4294967295

#if PLATFORM_WINDOWS
CT_ASSERT(sizeof(long) == sizeof(i32));
typedef volatile LONG volatile_i32;
typedef volatile ULONG volatile_u32;
typedef volatile LONGLONG volatile_i64;
typedef volatile ULONGLONG volatile_u64;
#else
typedef volatile i32 volatile_i32;
typedef volatile u32 volatile_u32;
typedef volatile i64 volatile_i64;
typedef volatile u64 volatile_u64;
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Truncations

u32 U64ToU32(u64 value)
{
	ASSERT(value <= U32_MAX);
	const u32 res = (u64)value;
	return res;
}

u16 I32ToU16(i32 value)
{
	ASSERT( value >= 0 && value < U16_MAX );
	const u16 res = (u16)value;
	return res;
}

u8 I32ToU8(i32 value)
{
	ASSERT( value >= 0 && value < U8_MAX );
	const u8 res = (u8)value;
	return res;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// System helpers

#if PLATFORM_WINDOWS

void Win32ReportError(const char *context)
{
	DWORD errorCode = ::GetLastError();
	if ( errorCode != ERROR_SUCCESS )
	{
		LPVOID messageBuffer;
		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				errorCode,
				MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				(LPTSTR)&messageBuffer,
				0, NULL );

		LOG(Error, "Error (%s): %s\n", context, (char*)messageBuffer);
		LocalFree( messageBuffer );
	}
}

#elif PLATFORM_LINUX || PLATFORM_ANDROID

void LinuxReportError(const char *context)
{
	char buffer[1024] = {};
	const char *errorString = strerror_r(errno, buffer, ARRAY_COUNT(buffer));
	LOG(Error, "Error (%s): %s\n", context, errorString);
}

#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Intrinsics

#define TOOLS_USE_INTRINSICS 1

#if TOOLS_USE_INTRINSICS
#if PLATFORM_WINDOWS
// Count trailing zeros
u32 CTZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	unsigned long firstBitSetIndex;
	_BitScanForward(&firstBitSetIndex, bitMask);
	return firstBitSetIndex;
}

// Count leading zeros
u32 CLZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	unsigned long lastBitSetIndex;
	_BitScanReverse(&lastBitSetIndex, bitMask);
	return 31 - lastBitSetIndex;
}
#else // #if PLATFORM_WINDOWS
// Count trailing zeros
u32 CTZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	u32 count = __builtin_ctz(bitMask);
	return count;
}

// Count leading zeros
u32 CLZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	u32 count = __builtin_clz(bitMask);
	return count;
}
#endif // #else // #if PLATFORM_WINDOWS
#else // #if TOOLS_USE_INTRINSICS
// Count trailing zeros
u32 CTZ(u32 bitMask)
{
	ASSERT(bitMask != 0);
	u32 count = 0;
	while ((bitMask & 1) == 0) {
		bitMask >>= 1;
		++count;
	}
	return count;
}

// Count leading zeros
u32 CLZ(u32 bitMask)
{
	// TODO: This has not been tested
	ASSERT(bitMask != 0);
	u32 count = 0;
	while ((bitMask & 0x80000000) == 0) {
		bitMask <<= 1;
		++count;
	}
	return count;
}
#endif // #else // #if TOOLS_USE_INTRINSICS

// First bit set
u32 FBS(u32 bitMask)
{
	const u32 res = CTZ(bitMask);
	return res;
}

// First bit zero
u32 FBZ(u32 bitMask)
{
	const u32 res = CTZ(~bitMask);
	return res;
}

bool AtomicSwap(volatile_u32 *currValue, u32 oldValue, u32 newValue)
{
#if PLATFORM_WINDOWS
		const bool swapped = InterlockedCompareExchange(currValue, newValue, oldValue) != *currValue;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
		const bool swapped = __sync_bool_compare_and_swap(currValue, oldValue, newValue);
#else
#error "Missing implementation"
#endif
	return swapped;
}

bool AtomicSwap(volatile_u64 *currValue, u64 oldValue, u64 newValue)
{
#if PLATFORM_WINDOWS
		const bool swapped = InterlockedCompareExchange(currValue, newValue, oldValue) != *currValue;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
		const bool swapped = __sync_bool_compare_and_swap(currValue, oldValue, newValue);
#else
#error "Missing implementation"
#endif
	return swapped;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Strings

struct String
{
	const char* str;
	u32 size;
};

u32 StrLen(const char *str)
{
	u32 len = 0;
	while (*str++) ++len;
	return len;
}

String MakeString(const char *str, u32 size)
{
	String string = { str, size };
	return string;
}

String MakeString(const char *str)
{
	u32 size = StrLen(str);
	String string = { str, size };
	return string;
}

void StrCopy(char *dst, const String& src_string)
{
	u32 size = src_string.size;
	const char *src = src_string.str;
	while (size-- > 0) *dst++ = *src++;
	*dst = '\0';
}

void StrCopy(char *dst, const char *src)
{
	while (*src) *dst++ = *src++;
	*dst = 0;
}

void StrCopyN(char *dst, const char *src, u32 size)
{
	while (*src && size-- > 0) *dst++ = *src++;
	*dst = 0;
}

void StrCat(char *dst, const char *src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
}

void StrCat(char *dst, const String &src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
}

bool StrEq(const String &s1, const String &s2)
{
	if ( s1.size != s2.size ) return false;

	const char *str1 = s1.str;
	const char *str2 = s2.str;
	const char *str2end = s2.str + s2.size;

	while ( *str1 == *str2 && str2 != str2end )
	{
		str1++;
		str2++;
	}

	return str2 == str2end;
}

bool StrEq(const String &s11, const char *s2)
{
	const char *s1 = s11.str;
	u32 count = s11.size;
	while ( count > 0 && *s1 == *s2 )
	{
		s1++;
		s2++;
		count--;
	}
	return count == 0 && *s2 == 0;
}

bool StrEq(const char *s1, const char *s2)
{
	while ( *s1 == *s2 && *s1 )
	{
		s1++;
		s2++;
	}
	return *s1 == *s2;
}

bool StrEqN(const char *s1, const char *s2, u32 n)
{
	while ( *s1 == *s2 && *s1 && n > 0 )
	{
		s1++;
		s2++;
		n--;
	}
	return n == 0 || *s1 == *s2;
}

// Searches str2 in str1 and returns a pointer to the last matching character '\0'.
// If the beginning of str1 does not completely match str2, then NULL is returned.
const char *StrConsume( const char *str1, const char *str2 )
{
	while ( *str2 && *str1++ == *str2++ );
	// If str does not point to the last char, there's no match so NULL is returned.
	// Otherwise the 'advanced' str1 pointer is returned.
	return *str2 ? NULL: str1;
}


bool StrToBool(const char *str, u32 len = U32_MAX)
{
	const bool value = *str == '1' || StrEqN(str, "true", len);
	return value;
}

bool StrToBool(const String &s)
{
	const bool value = StrToBool(s.str, s.size);
	return value;
}

char StrToChar(const char *str, u32 len = U32_MAX)
{
	return len > 0 ? *str : '?';
}

char StrToChar(const String &s)
{
	const char value = StrToChar(s.str, s.size);
	return value;
}

const char *StrChar(const char *str, char c)
{
	while (*str && *str != c) {
		str++;
	}
	return *str ? str : nullptr;
}

void StrReplace(char *str, char a, char b)
{
	while (*str) {
		if (*str == a) {
			*str = b;
		}
		str++;
	}
}

const char *StrCharR(const char *str, char c)
{
	const char *res = nullptr;
	while ( (str = StrChar(str, c)) != 0 ) {
		res = str;
		str++;
	}
	return res;
}

i32 StrToInt(const char *str, u32 len = U32_MAX)
{
	i32 integer = 0;

	// scan sign
	bool negative = false;
	if (*str == '-') {
		negative = true;
		str++;
		len--;
	}

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	const i32 result = negative ? -integer : integer;
	return result;
}

i32 StrToInt(const String &s)
{
	const i32 number = StrToInt(s.str, s.size);
	return number;
}

u32 StrToUnsignedInt(const char *str, u32 len = U32_MAX)
{
	u32 integer = 0;

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	return integer;
}

u32 StrToUnsignedInt(const String &s)
{
	const u32 number = StrToUnsignedInt(s.str, s.size);
	return number;
}

f32 StrToFloat(const char *str, u32 len = U32_MAX)
{
	i32 integer = 0;

	// scan sign
	i32 sign = 1.0f;
	if (*str == '-') {
		sign = -1.0f;
		str++;
		len--;
	}

	// scan integer part
	while (*str >= '0' && *str <= '9' && len > 0) {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	switch (*str++) {
		case '.': break;
		default: return integer;
	}
	len--;

	// scan decimal part
	u32 tenPower = 1;
	while (*str >= '0' && *str <= '9' && len > 0) {
		tenPower = (tenPower << 3) + (tenPower << 1);
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
		len--;
	}

	const float value = sign * integer / (f32)tenPower;
	return value;
}

f32 StrToFloat(const String &s)
{
	const f32 number = StrToFloat(s.str, s.size);
	return number;
}

i32 VSPrintf(char *buffer, const char *format, va_list vaList)
{
	const i32 res = vsprintf(buffer, format, vaList);
	return res;
}

i32 SPrintf(char *buffer, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	const i32 res = VSPrintf(buffer, format, vaList);
	va_end(vaList);
	return res;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Hashing

#define TOOLS_HASH_FNV_SEED 16777619

u32 HashFNV(const void *data, u32 size, u32 initial = 0, u32 prime = TOOLS_HASH_FNV_SEED)
{
	u32 hash = initial;
	const unsigned char *str = (const unsigned char *)data;
	for (u32 i = 0; i < size; ++i) {
		hash = (hash * prime) ^ str[i];
	}
	return hash;
}

u32 HashStringFNV(const char *data, u32 initial = 0, u32 prime = TOOLS_HASH_FNV_SEED)
{
	u32 hash = initial;
	const unsigned char *str = (const unsigned char *)data;
	while (*str) {
		hash = (hash * prime) ^ *str++;
	}
	return hash;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory

#if PLATFORM_LINUX || PLATFORM_ANDROID || PLATFORM_APPLE

void* AllocateVirtualMemory(u32 size)
{
	void* baseAddress = 0;
	i32 prot = PROT_READ | PROT_WRITE;
	i32 flags = MAP_PRIVATE | MAP_ANONYMOUS;
	i32 fd = -1;
	off_t offset = 0;
	void *allocatedMemory = mmap(baseAddress, size, prot, flags, fd, offset);
	ASSERT( allocatedMemory != MAP_FAILED && "Failed to allocate memory." );
	return allocatedMemory;
}

#elif PLATFORM_WINDOWS

void* AllocateVirtualMemory(u32 size)
{
	void *data = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return data;
}

#endif

void MemSet(void *ptr, u32 size, byte value)
{
	byte *bytePtr = (byte*)ptr;
	while (size-- > 0) *bytePtr++ = value;
}

void MemCopy(void *dst, const void *src, u32 size)
{
	const byte *pSrc = (byte*) src;
	const byte *pEnd = pSrc + size;
	byte *pDst = (byte*) dst;
	while (pSrc != pEnd) *pDst++ = *pSrc++;
}

i32 MemCompare(const void *a, const void *b, u32 size)
{
	const byte *pA = (const byte*) a;
	const byte *pB = (const byte*) b;
	const byte *pEnd = pA + size;
	while (pA != pEnd) {
		const byte valueA = *pA++;
		const byte valueB = *pB++;
		if (valueA != valueB) {
			return valueA - valueB;
		}
	}
	return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Arena

struct Arena
{
	byte* base;
	u32 used;
	u32 size;
};

Arena MakeArena(byte* base, u32 size)
{
	ASSERT(base != NULL && "MakeArena needs a non-null base pointer.");
	ASSERT(size > 0 && "MakeArena needs a greater-than-zero size.");
	Arena arena = {};
	arena.base = base;
	arena.size = size;
	arena.used = 0;
	return arena;
}

Arena MakeSubArena(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "MakeSubArena of bounds of the memory arena.");
	Arena subarena = {};
	subarena.base = arena.base + arena.used;
	subarena.size = size;
	subarena.used = 0;
	return subarena;
}

Arena MakeSubArena(Arena &arena)
{
	u32 remainingSize = arena.size - arena.used;
	Arena subarena = MakeSubArena(arena, remainingSize);
	return subarena;
}

byte* PushSize(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "PushSize of bounds of the memory arena.");
	byte* head = arena.base + arena.used;
	arena.used += size;
	return head;
}

byte* PushZeroSize(Arena &arena, u32 size)
{
	byte *bytes = PushSize(arena, size);
	MemSet(bytes, size, 0);
	return bytes;
}

char *PushStringN(Arena &arena, const char *str, u32 len)
{
	char *bytes = (char*)PushSize(arena, len+1);
	MemCopy(bytes, str, len);
	bytes[len] = 0;
	return bytes;
}

char *PushString(Arena &arena, const char *str)
{
	u32 len = StrLen(str);
	char *bytes = PushStringN(arena, str, len);
	return bytes;
}

char *PushChar(Arena &arena, char c)
{
	char *bytes = (char*)PushSize(arena, sizeof(c));
	*bytes = c;
	return bytes;
}

void ResetArena(Arena &arena)
{
	// This tells the OS we don't need these pages
	//madvise(arena.base, arena.size, MADV_DONTNEED);
	arena.used = 0;
}

void PrintArenaUsage(Arena &arena)
{
	LOG(Info, "Memory Arena Usage:\n");
	LOG(Info, "- size: %u B / %u kB\n", arena.size, arena.size/1024);
	LOG(Info, "- used: %u B / %u kB\n", arena.used, arena.used/1024);
}

#define ZeroStruct( pointer ) MemSet(pointer, sizeof(*pointer), 0)
#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))
#define PushArray( arena, type, count ) (type*)PushSize(arena, sizeof(type) * count)
#define PushZeroStruct( arena, struct_type ) (struct_type*)PushZeroSize(arena, sizeof(struct_type))
#define PushZeroArray( arena, type, count ) (type*)PushZeroSize(arena, sizeof(type) * count)



////////////////////////////////////////////////////////////////////////////////////////////////////
// String interning

struct StringIntern
{
	char *str;
	u32 hash;
};

struct StringInterningNode
{
	StringIntern stringIntern;
	StringInterningNode *next;
};

struct StringInterningTable
{
	StringInterningNode* bins[1024];
};

struct StringInterning
{
	Arena *arena;
	StringInterningTable *table;
};

StringInterning StringInterningCreate(Arena *arena)
{
	StringInterning interning = {
		.arena = arena,
		.table = PushZeroStruct(*arena, StringInterningTable),
	};
	return interning;
}

const char *MakeStringIntern(StringInterning *context, const char *str, u32 len)
{
	ASSERT(context && context->table);
	const u32 hash = HashFNV(str, len);
	const u32 index = hash % ARRAY_COUNT(context->table->bins);

	StringInterningNode *next = NULL;

	if (context->table->bins[index])
	{
		StringInterningNode *node = context->table->bins[index];
		while (node)
		{
			if (node->stringIntern.hash == hash) // same hash?
			{
				if (StrEq(node->stringIntern.str, str)) // same string?
				{
					return node->stringIntern.str; // found!
				}
			}
			node = node->next; // collision! keep searching...
		}
		next = context->table->bins[index];
	}

	// No coincidence found, insert a new node
	StringInterningNode *node = PushZeroStruct(*(context->arena), StringInterningNode);
	node->stringIntern.str = PushStringN(*(context->arena), str, len);
	node->stringIntern.hash = hash;
	node->next = next;
	context->table->bins[index] = node;
	return node->stringIntern.str;
}

const char *MakeStringIntern(StringInterning *context, const char *str)
{
	const u32 len = StrLen(str);
	const char *internStr = MakeStringIntern(context, str, len);
	return internStr;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Files

struct DataChunk
{
	union {
		byte *bytes;
		const char *chars;
	};
	u64 size;
};

bool GetFileSize(const char *filename, u64 &size, bool reportError = true)
{
	bool ok = true;
#if PLATFORM_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA Data;
	if( GetFileAttributesExA(filename, GetFileExInfoStandard, &Data) )
	{
		size = (u64)Data.nFileSizeLow;
		size |= (u64)Data.nFileSizeHigh << 32;
	}
	else
	{
		if ( reportError ) Win32ReportError("GetFileSize");
		ok = false;
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	struct stat attrib;
	if ( stat(filename, &attrib) == 0 )
	{
		size = attrib.st_size;
	}
	else
	{
		if ( reportError )
		{
			char text[MAX_PATH_LENGTH];
			SPrintf(text, "stat %s", filename);
			LinuxReportError(text);
		}
		ok = false;
	}
#endif
	return ok;
}

bool ExistsFile(const char *filename)
{
	u64 size = 0;
	const bool res = GetFileSize(filename, size, false);
	return res;
}

bool ReadEntireFile(const char *filename, void *buffer, u64 bytesToRead)
{
	bool ok = false;
#if PLATFORM_WINDOWS
	HANDLE file = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
	if ( file == INVALID_HANDLE_VALUE  )
	{
		Win32ReportError("ReadEntireFile - CreateFileA");
	}
	else
	{
		DWORD bytesRead = 0;
		ok = ReadFile( file, buffer, bytesToRead, &bytesRead, NULL );
		if ( !ok && bytesToRead == bytesRead )
		{
			Win32ReportError("ReadEntireFile - ReadFile");
		}
		CloseHandle( file );
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	int fd = open(filename, O_RDONLY);
	if ( fd == -1 )
	{
		LinuxReportError("open");
	}
	else
	{
		while ( bytesToRead > 0 )
		{
			u32 bytesRead = read(fd, buffer, bytesToRead);
			if ( bytesRead > 0 )
			{
				bytesToRead -= bytesRead;
				buffer = (byte*)buffer + bytesRead;
			}
			else
			{
				LinuxReportError("read");
				break;
			}
		}
		ok = (bytesToRead == 0);
		close(fd);
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

bool WriteEntireFile(const char *filename, const void *buffer, u64 bytesToWrite)
{
	// TODO: Backup the previous file?

	bool ok = false;
#if PLATFORM_WINDOWS
	HANDLE file = CreateFileA( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( file == INVALID_HANDLE_VALUE  )
	{
		Win32ReportError("WriteEntireFile - CreateFileA");
	}
	else
	{
		DWORD bytesWritten = 0;
		ok = WriteFile( file, buffer, bytesToWrite, &bytesWritten, NULL );
		if ( !ok && bytesToWrite == bytesWritten )
		{
			Win32ReportError("WriteEntireFile - WriteFile");
		}
		CloseHandle( file );
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR );
	if ( fd == -1 )
	{
		LinuxReportError("open");
	}
	else
	{
		while ( bytesToWrite > 0 )
		{
			const u32 bytesWritten = write(fd, buffer, bytesToWrite);
			if ( bytesWritten > 0 )
			{
				bytesToWrite -= bytesWritten;
				buffer = (byte*)buffer + bytesWritten;
			}
			else
			{
				LinuxReportError("write");
				break;
			}
		}
		ok = (bytesToWrite == 0);
		close(fd);
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

DataChunk *PushFile( Arena& arena, const char *filename )
{
	DataChunk *file = 0;

	u64 fileSize;
	if ( GetFileSize( filename, fileSize ) && fileSize > 0 )
	{
		Arena backupArena = arena;
		byte *fileData = PushArray( arena, byte, fileSize + 1 );
		if ( ReadEntireFile( filename, fileData, fileSize ) )
		{
			fileData[fileSize] = 0; // final zero
			file = PushStruct( arena, DataChunk );
			file->bytes = fileData;
			file->size = fileSize;
		}
		else
		{
			// TODO: Log error here?
			arena = backupArena;
		}
	}

	return file;
}

bool GetFileLastWriteTimestamp(const char* filename, u64 &ts)
{
	bool ok = true;

#if PLATFORM_WINDOWS
	union Filetime2u64 {
		FILETIME filetime;
		u64      u64time;
	} conversor;

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if( GetFileAttributesExA(filename, GetFileExInfoStandard, &Data) )
	{
		conversor.filetime = Data.ftLastWriteTime;
		ts = conversor.u64time;
	}
	else
	{
		Win32ReportError("GetFileLastWriteTimestamp");
		ok = false;
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	struct stat attrib;
	if ( stat(filename, &attrib) == 0 )
	{
		ts = attrib.st_mtime;
	}
	else
	{
		char text[MAX_PATH_LENGTH];
		SPrintf(text, "stat %s", filename);
		LinuxReportError(text);
		ok = false;
	}
#endif

	return ok;
}

bool CopyFile(const char *srcPath, const char *dstPath)
{
	bool ok = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	int fd_rd = open(srcPath, O_RDONLY);
	if ( fd_rd == -1 ) {
		LinuxReportError("CopyFile open O_RDONLY");
	} else {
		int fd_wr = open(dstPath, O_WRONLY | O_CREAT | O_EXCL, 0666);
		if ( fd_wr == -1 ) {
			LinuxReportError("CopyFile open O_WRONLY | O_CREAT | O_EXCL");
		} else {
			char buffer[4096];
			ssize_t nread;
			while (nread = read(fd_rd, buffer, sizeof(buffer)), nread > 0) {
				char *out_ptr = buffer;
				ssize_t nwritten;
				do {
					nwritten = write(fd_wr, out_ptr, nread);
					if (nwritten >= 0) {
						nread -= nwritten;
						out_ptr += nwritten;
					} else if (errno != EINTR) {
						LinuxReportError("CopyFile write");
						close(fd_rd);
						close(fd_wr);
						return false;
					}
				} while (nread > 0);
			}
			ok = true;
			close(fd_wr);
		}
		close(fd_rd);
	}
#elif PLATFORM_WINDOWS
	ok = CopyFile(srcPath, dstPath, false);
	if ( !ok ) {
		Win32ReportError("CopyFile");
	}
#else
#error "Missing implementation"
#endif
	return ok;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// File paths

const char *BinDir = "";
const char *DataDir = "";
const char *AssetDir = "";
const char *ProjectDir = "";

struct FilePath
{
	char str[MAX_PATH_LENGTH];
};

FilePath MakePath(const char *basePath, const char *relativePath)
{
	FilePath path = {};
	StrCopy(path.str, basePath);
	StrCat(path.str, "/");
	StrCat(path.str, relativePath);
	return path;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Directories

#if PLATFORM_LINUX || PLATFORM_ANDROID
struct Dir
{
	DIR *handle;
};

struct DirEntry
{
	dirent *data;
	const char *name;
};
#elif PLATFORM_WINDOWS
struct Dir
{
    HANDLE handle;
};

struct DirEntry
{
	WIN32_FIND_DATA data;
	const char *name;
};
#else
#error "Missing implementation"
#endif

bool CreateDirectory(const char *path)
{
	bool ok = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	int res = mkdir(path, S_IRWXU | S_IRWXG | S_IXOTH );
	if ( res != 0 ) {
		char message[MAX_PATH_LENGTH];
		SPrintf(message, "mkdir %s", path);
		perror(message);
		ok = false;
	}
#elif PLATFORM_WINDOWS
	ok = CreateDirectoryA(path, nullptr);
	if ( !ok ) {
		char message[MAX_PATH_LENGTH];
		SPrintf(message, "CreateDirectoryA %s", path);
		Win32ReportError(message);
	}
#else
#error "Missing implementation"
#endif
	return ok;
}

bool OpenDir(Dir &dir, const char *path) 
{
	bool res = false;

#if PLATFORM_LINUX || PLATFORM_ANDROID
	dir.handle = opendir(path);
	res = dir.handle != nullptr;
	return dir.handle != nullptr;
#elif PLATFORM_WINDOWS
	char wildcardPath[MAX_PATH_LENGTH];
    SPrintf(wildcardPath, "%s/*.*", path);

	// NOTE: FindFirstFile/FindNextFile called here to skip . and .. directories
	WIN32_FIND_DATA entryData;
    dir.handle = FindFirstFile(wildcardPath, &entryData);
	if (dir.handle != INVALID_HANDLE_VALUE && FindNextFile(dir.handle, &entryData) ) {
		res = true;
	} else {
		LOG(Warning, "Path not found: %s\n", wildcardPath);
		Win32ReportError("FindFirstFile");
    }
#else
#error "Missing implementation"
#endif

	return res;
}

bool ReadDir(Dir &dir, DirEntry &entry)
{
	bool res = false;
#if PLATFORM_LINUX || PLATFORM_ANDROID
	if ( dir.handle )
	{
		do
		{
			entry.data = readdir(dir.handle);
			entry.name = entry.data->d_name;
			res = entry.data != nullptr;
		}
		// We skip . and .. directories
		while ( res && ( StrEq(entry.name, ".") || StrEq(entry.name, "..") ) );
	}
#elif PLATFORM_WINDOWS
	if ( dir.handle != INVALID_HANDLE_VALUE )
	{
		res = FindNextFile(dir.handle, &entry.data);
		entry.name = entry.data.cFileName;
	}
#else
#error "Missing implementation"
#endif
	return res;
}

void CloseDir(Dir &dir)
{
#if PLATFORM_LINUX || PLATFORM_ANDROID
	if ( dir.handle )
	{
		closedir(dir.handle);
		dir.handle = nullptr;
	}
#elif PLATFORM_WINDOWS
	if ( dir.handle != INVALID_HANDLE_VALUE )
	{
		FindClose(dir.handle);
		dir.handle = INVALID_HANDLE_VALUE;
	}
#else
#error "Missing implementation"
#endif
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Process execution

bool ExecuteProcess(const char *commandLine)
{
	bool success = false;

	char commandLineCopy[MAX_PATH_LENGTH];
	StrCopyN(commandLineCopy, commandLine, MAX_PATH_LENGTH - 1);

#if PLATFORM_WINDOWS
    STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );

	success = CreateProcessA(
		NULL, // No module name, use command line
		commandLineCopy, // Command line
		NULL, // Process handle not inheritable
		NULL, // Thread handle not inheritable
		FALSE, // Set handle inheritance to false
		0, // No creation flags
		NULL, // Use parent's environment block
		NULL, // Use parent's starting directory
		&si, // Pointer to STARTUPINFO structure
		&pi // Pointer to PROCESS_INFORMATION structure
	);

	if (success)
	{
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles.
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
	else
	{
		LOG(Error, "CreateProcessA failed.\n");
	}
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	// Split list of arguments
	char *ptr = commandLineCopy;
	int argc = 0;
	char *argv[128] = {};
	bool addNextArgument = true;
	while (*ptr) {
		if (*ptr == ' ') {
			addNextArgument = true;
			*ptr = 0;
		} else {
			if ( addNextArgument ) {
				ASSERT(argc < ARRAY_COUNT(argv) - 1);
				argv[argc++] = ptr;
				addNextArgument = false;
			}
		}
		ptr++;
	}

	// posix_spawn behaves similar to fork/exec
	pid_t pid;
	int status = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);
	if (status == 0) {
		if (waitpid(pid, &status, 0) != -1) {
			// Log(Debug, "Child exited with status %i\n", status);
			success = true;
		} else {
			perror("waitpid");
		}
	} else {
		LOG(Error, "posix_spawnp failed: %s\n", strerror(status));
		LOG(Error, "- Command: %s\n", commandLine);
	}
#else
#error "Missing implementation"
#endif

	return success;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dynamic library loading

#if PLATFORM_LINUX || PLATFORM_ANDROID

	typedef void* DynamicLibrary;

	DynamicLibrary OpenLibrary(const char *filepath)
	{
		DynamicLibrary library = dlopen(filepath, RTLD_NOW);
		return library;
	}

	void* LoadSymbol(DynamicLibrary library, const char *symbolName)
	{
		void *symbol = dlsym(library, symbolName);
		return symbol;
	}

	void CloseLibrary(DynamicLibrary library)
	{
		dlclose(library);
	}

#elif PLATFORM_WINDOWS

	typedef HINSTANCE DynamicLibrary;

	DynamicLibrary OpenLibrary(const char *filepath)
	{
		DynamicLibrary library = LoadLibrary(filepath);
		return library;
	}

	void* LoadSymbol(DynamicLibrary library, const char *symbolName)
	{
		void *symbol = GetProcAddress(library, symbolName);
		return symbol;
	}

	void CloseLibrary(DynamicLibrary library)
	{
		FreeLibrary(library);
	}

#else

	#error "Missing implementation"

#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Math

struct rgba
{
	byte r, g, b, a;
};

typedef u32 uint;

struct int2
{
	union { i32 x, r; };
	union { i32 y, g; };
};

struct uint2
{
	union { u32 x, r; };
	union { u32 y, g; };
};

struct rect
{
	int2 pos;
	uint2 size;
};

struct float2
{
	union { float x, r; };
	union { float y, g; };
};

struct float3
{
	union { float x, r; };
	union { float y, g; };
	union { float z, b; };
};

struct float4
{
	union
	{
		f32 vec[4];
		struct
		{
			union { float x, r; };
			union { float y, g; };
			union { float z, b; };
			union { float w, a; };
		};
		float3 xyz;
	};
};

struct float3x3
{
	union
	{
		f32 mat[3][3];
		struct
		{
			f32 m00, m01, m02,
				m10, m11, m12,
				m20, m21, m22;
		};
	};
};

struct float4x4
{
	union
	{
		f32 mat[4][4];
		struct
		{
			f32 m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33;
		};
	};
};

static constexpr f32 Pi = 3.14159265358979323846f;
static constexpr f32 TwoPi = 2.0f * Pi;
static constexpr f32 ToRadians = Pi / 180.0f;
static constexpr f32 ToDegrees = 180.0f / Pi;

float2 operator+(float2 a, float2 b)
{
	const float2 res = { .x = a.x + b.x, .y = a.y + b.y };
	return res;
}

float3 operator+(float3 a, float3 b)
{
	const float3 res = { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };
	return res;
}

float2 operator+=(float2& a, float2 b)
{
	a = { .x = a.x + b.x, .y = a.y + b.y };
	return a;
}

float2 operator-(float2 a, float2 b)
{
	const float2 res = { .x = a.x - b.x, .y = a.y - b.y };
	return res;
}

float2 operator*(float a, float2 b)
{
	const float2 res = { .x = a * b.x, .y = a * b.y };
	return res;
}

float3 operator*(float a, float3 b)
{
	const float3 res = { .x = a * b.x, .y = a * b.y, .z = a * b.z };
	return res;
}

int2 operator+(int2 pos, uint2 size)
{
	const int2 pos2 = { pos.x + (i32)size.x, pos.y + (i32)size.y };
	return pos2;
}

int2 operator+(int2 pos, int2 size)
{
	const int2 pos2 = { pos.x + size.x, pos.y + size.y };
	return pos2;
}

int2 operator-(int2 a, int2 b)
{
	const int2 size = { a.x - b.x, a.y - b.y };
	return size;
}

i32 Max(i32 a, i32 b) { return a > b ? a : b; }
u32 Max(u32 a, u32 b) { return a > b ? a : b; }
f32 Max(f32 a, f32 b) { return a > b ? a : b; }
int2 Max(int2 a, int2 b) { return { Max(a.x, b.x), Max(a.y, b.y) }; }
uint2 Max(uint2 a, uint2 b) { return { Max(a.x, b.x), Max(a.y, b.y) }; }
float2 Max(float2 a, float2 b) { return { .x = Max(a.x, b.x), .y = Max(a.y, b.y) }; }

i32 Min(i16 a, i16 b) { return a < b ? a : b; }
i32 Min(i32 a, i32 b) { return a < b ? a : b; }
u32 Min(u32 a, u32 b) { return a < b ? a : b; }
f32 Min(f32 a, f32 b) { return a < b ? a : b; }
int2 Min(int2 a, int2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }
uint2 Min(uint2 a, uint2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }
float2 Min(float2 a, float2 b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }

i32 Clamp( i32 v, i32 min, i32 max ) { return Min( Max( v, min ), max ); }
u32 Clamp( u32 v, u32 min, u32 max ) { return Min( Max( v, min ), max ); }
f32 Clamp( f32 v, f32 min, f32 max ) { return Min( Max( v, min ), max ); }

rgba Rgba(float4 color)
{
	const rgba res = {
		(byte)(color.r * 255),
		(byte)(color.g * 255),
		(byte)(color.b * 255),
		(byte)(color.a * 255),
	};
	return res;
}

float3 Float3(float value)
{
	const float3 res = {value, value, value};
	return res;
}

float3 Float3(float x, float y, float z)
{
	const float3 res = {x, y, z};
	return res;
}

float4 Float4(float3 xyz, f32 w)
{
	const float4 res = { xyz.x, xyz.y, xyz.z, w };
	return res;
}

float3x3 Float3x3(const float4x4 &m)
{
	const float3x3 res = {
		m.m00, m.m01, m.m02,
		m.m10, m.m11, m.m12,
		m.m20, m.m21, m.m22,
	};
	return res;
}

float4x4 Float4x4(const float3x3 &m)
{
	const float4x4 res = {
		m.m00, m.m01, m.m02, 0.0f,
		m.m10, m.m11, m.m12, 0.0f,
		m.m20, m.m21, m.m22, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	return res;
}

f32 Sin(f32 value)
{
	const f32 res = ::sinf(value);
	return res;
}

f32 Cos(f32 value)
{
	const f32 res = ::cosf(value);
	return res;
}

f32 Tan(f32 value)
{
	const f32 res = ::tanf(value);
	return res;
}

f32 Sqrt(f32 value)
{
	const f32 res = ::sqrtf(value);
	return res;
}

f32 Round(f32 value)
{
	const f32 res = static_cast<f32>(static_cast<i32>((value + 0.5f - ( value < 0.0f ))));
	return res;
}

float3 FromTo(const float3 &a, const float3 &b)
{
	const float3 res = { b.x - a.x, b.y - a.y, b.z - a.z };
	return res;
}

float2 Add(const float2 &a, const float2 &b)
{
	const float2 res = { b.x + a.x, b.y + a.y };
	return res;
}

float3 Add(const float3 &a, const float3 &b)
{
	const float3 res = { b.x + a.x, b.y + a.y, b.z + a.z };
	return res;
}

float3 Sub(const float3 &a, const float3 &b)
{
	const float3 res = { a.x - b.x, a.y - b.y, a.z - b.z };
	return res;
}

float3 Mul(const float3 &a, f32 b)
{
	const float3 res = { a.x * b, a.y * b, a.z * b };
	return res;
}

float4 Mul(const float4x4 &a, const float4 &b)
{
#define USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION 1
#if USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION
	const float4 res = {
		a.m00*b.x + a.m10*b.y + a.m20*b.z + a.m30*b.w,
		a.m01*b.x + a.m11*b.y + a.m21*b.z + a.m31*b.w,
		a.m02*b.x + a.m12*b.y + a.m22*b.z + a.m32*b.w,
		a.m03*b.x + a.m13*b.y + a.m23*b.z + a.m33*b.w,
	};
	return res;
#else
#	error "Missing implementation"
#endif
}

float3 MulVector(const float4x4 &a, const float3 &b)
{
	const float3 res = Mul(a, Float4(b, 0.0)).xyz;
	return res;
}

float3 MulPoint(const float4x4 &a, const float3 &b)
{
	const float3 res = Mul(a, Float4(b, 1.0)).xyz;
	return res;
}

float4x4 Mul(const float4x4 &a, const float4x4 &b)
{
#define USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION 1
#if USE_ROWS_TIMES_COLUMNS_MATRIX_MULTIPLICATION
	const float4x4 res = {
		a.m00*b.m00 + a.m10*b.m01 + a.m20*b.m02 + a.m30*b.m03,
		a.m01*b.m00 + a.m11*b.m01 + a.m21*b.m02 + a.m31*b.m03,
		a.m02*b.m00 + a.m12*b.m01 + a.m22*b.m02 + a.m32*b.m03,
		a.m03*b.m00 + a.m13*b.m01 + a.m23*b.m02 + a.m33*b.m03,

		a.m00*b.m10 + a.m10*b.m11 + a.m20*b.m12 + a.m30*b.m13,
		a.m01*b.m10 + a.m11*b.m11 + a.m21*b.m12 + a.m31*b.m13,
		a.m02*b.m10 + a.m12*b.m11 + a.m22*b.m12 + a.m32*b.m13,
		a.m03*b.m10 + a.m13*b.m11 + a.m23*b.m12 + a.m33*b.m13,

		a.m00*b.m20 + a.m10*b.m21 + a.m20*b.m22 + a.m30*b.m23,
		a.m01*b.m20 + a.m11*b.m21 + a.m21*b.m22 + a.m31*b.m23,
		a.m02*b.m20 + a.m12*b.m21 + a.m22*b.m22 + a.m32*b.m23,
		a.m03*b.m20 + a.m13*b.m21 + a.m23*b.m22 + a.m33*b.m23,

		a.m00*b.m30 + a.m10*b.m31 + a.m20*b.m32 + a.m30*b.m33,
		a.m01*b.m30 + a.m11*b.m31 + a.m21*b.m32 + a.m31*b.m33,
		a.m02*b.m30 + a.m12*b.m31 + a.m22*b.m32 + a.m32*b.m33,
		a.m03*b.m30 + a.m13*b.m31 + a.m23*b.m32 + a.m33*b.m33,
	};
#else // columns times rows
	const float4x4 res = {
		a.m00*b.m00 + a.m01*b.m10 + a.m02*b.m20 + a.m03*b.m30,
		a.m00*b.m01 + a.m01*b.m11 + a.m02*b.m21 + a.m03*b.m31,
		a.m00*b.m02 + a.m01*b.m12 + a.m02*b.m22 + a.m03*b.m32,
		a.m00*b.m03 + a.m01*b.m13 + a.m02*b.m23 + a.m03*b.m33,

		a.m10*b.m00 + a.m11*b.m10 + a.m12*b.m20 + a.m13*b.m30,
		a.m10*b.m01 + a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31,
		a.m10*b.m02 + a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32,
		a.m10*b.m03 + a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33,

		a.m20*b.m00 + a.m21*b.m10 + a.m22*b.m20 + a.m23*b.m30,
		a.m20*b.m01 + a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31,
		a.m20*b.m02 + a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32,
		a.m20*b.m03 + a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33,

		a.m30*b.m00 + a.m31*b.m10 + a.m32*b.m20 + a.m33*b.m30,
		a.m30*b.m01 + a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31,
		a.m30*b.m02 + a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32,
		a.m30*b.m03 + a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33,
	};
#endif
	return res;
}

float3 Negate(const float3 &v)
{
	const float3 res = { -v.x, -v.y, -v.z };
	return res;
}

f32 Dot(const float3 &a, const float3 &b)
{
	const f32 res = a.x * b.x + a.y * b.y + a.z * b.z;
	return res;
}

f32 Dot(const float4 &a, const float4 &b)
{
	const f32 res = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	return res;
}

f32 Length2(const float3 &v)
{
	const f32 res = Dot(v, v);
	return res;
}

f32 Length(const float3 &v)
{
	const f32 res = Sqrt(Length2(v));
	return res;
}

bool IsZero(const float3 &v)
{
	return Length2(v) == 0.0f;
}

float3 Normalize(const float3 &v)
{
	const f32 invLen = 1.0f / Length(v);
	const float3 res = {v.x * invLen, v.y * invLen, v.z * invLen};
	return res;
}

float3 NormalizeIfNotZero(const float3 &v)
{
	const float3 res = Length2(v) > 0.0f ? Normalize(v) : v;
	return res;
}

float3 Cross(const float3 &u, const float3 &v)
{
	const float3 res = {
		u.y * v.z - u.z * v.y, // x
		u.z * v.x - u.x * v.z, // y
		u.x * v.y - u.y * v.x  // z
	};
	return res;
}

float4x4 Eye()
{
	float4x4 mat = {};
	mat.m00 = mat.m11 = mat.m22 = mat.m33 = 1.0f;
	return mat;
}

float4x4 Translate(const float3 &t)
{
	const float4x4 mat = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		t.x, t.y, t.z, 1,
	};
	return mat;
}

float4x4 Rotate(const float3 &axis, f32 degrees)
{
	// ASSERT( IsNormalized(axis) );

	const f32 radians = degrees * ToRadians;
	const f32 c = Cos(radians);
	const f32 s = Sin(radians);
	const f32 t = 1.0f - c;

	const f32 m00 = c + axis.x*axis.x*t;
	const f32 m11 = c + axis.y*axis.y*t;
	const f32 m22 = c + axis.z*axis.z*t;

	const f32 tmp1 = axis.x*axis.y*t;
	const f32 tmp2 = axis.z*s;
	const f32 m10 = tmp1 + tmp2;
	const f32 m01 = tmp1 - tmp2;

	const f32 tmp3 = axis.x*axis.z*t;
	const f32 tmp4 = axis.y*s;
	const f32 m20 = tmp3 - tmp4;
	const f32 m02 = tmp3 + tmp4;

	const f32 tmp5 = axis.y*axis.z*t;
	const f32 tmp6 = axis.x*s;
	const f32 m21 = tmp5 + tmp6;
	const f32 m12 = tmp5 - tmp6;

	const float4x4 mat = {
		m00, m01, m02, 0,
		m10, m11, m12, 0,
		m20, m21, m22, 0,
		0,     0,   0, 1,
	};
	return mat;
}

float4x4 Scale(const float3 &s)
{
	const float4x4 mat = {
		s.x, 0, 0, 0,
		0, s.y, 0, 0,
		0, 0, s.y, 0,
		0, 0, 0, 1,
	};
	return mat;
}

float4x4 LookAt(const float3 &vrp, const float3 &obs, const float3 &up)
{
	const float3 forward = Normalize(FromTo(vrp, obs));
	const float3 right = Normalize(Cross(up, forward));
	const float3 newUp = Cross(forward, right);
	const f32 tx = Dot(Negate(right) ,obs);
	const f32 ty = Dot(Negate(newUp) ,obs);
	const f32 tz = Dot(Negate(forward) ,obs);
	const float4x4 mat = {
		right.x, newUp.x, forward.x, 0,
		right.y, newUp.y, forward.y, 0,
		right.z, newUp.z, forward.z, 0,
		tx, ty, tz, 1,
	};
	return mat;
}

//#define USE_DEPTH_ZERO_TO_ONE 0 // For OpenGL
#define USE_DEPTH_ZERO_TO_ONE 1 // For DX12, Vulkan

//#define USE_REVERSE_Z 0
#define USE_REVERSE_Z 1

float4x4 Perspective(float fov, float aspect, float Near, float Far)
{
#if USE_REVERSE_Z
#if USE_DEPTH_ZERO_TO_ONE
	float tmp = Near;
	Near = Far;
	Far = tmp;
#else
#error "Check reverse-z for OpenGL implementations"
#endif
#endif

	const f32 yScale = 1.0f / Tan(fov * ToRadians / 2.0f);
	const f32 xScale = yScale / aspect;
	const f32 NearMinusFar = Near - Far;

	float4x4 mat = {};
	mat.m00 = xScale;
	mat.m11 = yScale;
#if USE_DEPTH_ZERO_TO_ONE
	mat.m22 = Far / NearMinusFar;
	mat.m32 = Far * Near / NearMinusFar;
#else
	mat.m22 = (Far + Near) / NearMinusFar;
	mat.m32 = 2.0f * Far * Near / NearMinusFar;
#endif
	mat.m23 = -1;

	return mat;
}

float4x4 Orthogonal(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f)
{
#if USE_REVERSE_Z
#if USE_DEPTH_ZERO_TO_ONE
	float tmp = n;
	n = f;
	f = tmp;
#else
#error "Check reverse-z for OpenGL implementations"
#endif
#endif

	float4x4 mat = {};
	const f32 RminusL = r - l;
	const f32 TminusB = t - b;
	const f32 FminusN = f - n;

	mat.m00 = 2.0f / RminusL;
	mat.m30 = -(r + l)/RminusL;
	mat.m11 = 2.0f / TminusB;
	mat.m31 = -(t + b)/TminusB;
#if USE_DEPTH_ZERO_TO_ONE
	mat.m22 = -1.0f / FminusN;
	mat.m32 = - n /FminusN;
#else
	mat.m22 = -2.0f / FminusN;
	mat.m32 = -(f + n)/FminusN;
#endif
	mat.m33 = 1.0f;

	return mat;
}

float3x3 Transpose(const float3x3 &m)
{
	const float3x3 res = {
		m.m00, m.m10, m.m20,
		m.m01, m.m11, m.m21,
		m.m02, m.m12, m.m22,
	};
	return res;
}

float4x4 Transpose(const float4x4 &m)
{
	const float4x4 res = {
		m.m00, m.m10, m.m20, m.m30,
		m.m01, m.m11, m.m21, m.m31,
		m.m02, m.m12, m.m22, m.m32,
		m.m03, m.m13, m.m23, m.m33,
	};
	return res;
}

i32 Floor(f32 value)
{
	const f32 res = ::floorf(value);
	return static_cast<i32>(res);
}

f32 Log2(f32 value)
{
	const f32 res = ::log2f(value);
	return res;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Alignment functions

bool IsPowerOfTwo(u32 value)
{
	// First value in the below expression is for the case when value is 0
	return value && (!(value & (value - 1)));
}

u32 AlignUp(u32 value, u32 alignment)
{
	ASSERT( IsPowerOfTwo(alignment) );
	const u32 alignedValue =  ((value - 1) & ~(alignment - 1)) + alignment;
	return alignedValue;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Time

struct Clock
{
#if PLATFORM_WINDOWS
	LARGE_INTEGER counter;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	timespec timeSpec;
#else
#	error "Missing implementation"
#endif
};

#define SECONDS_FROM_NANOSECONDS( ns ) ((float)(ns) / 1000000000.0f)

#if PLATFORM_WINDOWS
internal LONGLONG Win32GetPerformanceCounterFrequency()
{
	LARGE_INTEGER PerfCountFrequencyResult;
	BOOL res = QueryPerformanceFrequency(&PerfCountFrequencyResult);
	ASSERT(res && "QueryPerformanceFrequency() failed!");
	return PerfCountFrequencyResult.QuadPart;
}

// TODO: Investigate... is there any problem in computing this at static init time?
internal LONGLONG gPerformanceCounterFrequency = Win32GetPerformanceCounterFrequency();
#endif

Clock GetClock()
{
	Clock clock;
#if PLATFORM_WINDOWS
	QueryPerformanceCounter(&clock.counter);
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	clock_gettime(CLOCK_MONOTONIC, &clock.timeSpec);
#endif
	return clock;
}

float GetSecondsElapsed(Clock start, Clock end)
{
	f32 elapsedSeconds;
#if PLATFORM_WINDOWS
	ASSERT( start.counter.QuadPart <= end.counter.QuadPart );
	elapsedSeconds = (
			(float)(end.counter.QuadPart - start.counter.QuadPart) /
			(float)gPerformanceCounterFrequency );
	return elapsedSeconds;
#elif PLATFORM_LINUX || PLATFORM_ANDROID
	ASSERT( start.timeSpec.tv_sec < end.timeSpec.tv_sec || (
				start.timeSpec.tv_sec == end.timeSpec.tv_sec &&
				start.timeSpec.tv_nsec <= end.timeSpec.tv_nsec ) );
	if ( start.timeSpec.tv_sec == end.timeSpec.tv_sec )
	{
		elapsedSeconds = SECONDS_FROM_NANOSECONDS( end.timeSpec.tv_nsec - start.timeSpec.tv_nsec );
	}
	else
	{
		elapsedSeconds = end.timeSpec.tv_sec - start.timeSpec.tv_sec - 1;
		elapsedSeconds += 1.0f - SECONDS_FROM_NANOSECONDS( start.timeSpec.tv_nsec );
		elapsedSeconds += SECONDS_FROM_NANOSECONDS( end.timeSpec.tv_nsec );
	}
#endif
	return elapsedSeconds;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Window and input

#if defined(TOOLS_PLATFORM)

#if PLATFORM_LINUX
#	define USE_XCB 1
#elif PLATFORM_ANDROID
#	define USE_ANDROID 1
#elif PLATFORM_WINDOWS
#	define USE_WINAPI 1
#endif


#if USE_XCB
#	include <xcb/xcb.h>
#	include <stdlib.h> // free

void XcbReportError( int xcbErrorCode, const char *context )
{
	static const char *xcbErrorMessages[] = {
		"NO_ERROR",
		"XCB_CONN_ERROR",                   // 1
		"XCB_CONN_CLOSED_EXT_NOTSUPPORTED", // 2
		"XCB_CONN_CLOSED_MEM_INSUFFICIENT", // 3
		"XCB_CONN_CLOSED_REQ_LEN_EXCEED",   // 4
		"XCB_CONN_CLOSED_PARSE_ERR",        // 5
		"XCB_CONN_CLOSED_INVALID_SCREEN",   // 6
		"XCB_CONN_CLOSED_FDPASSING_FAILED", // 7
	};
	LOG(Error, "Xcb error (%s): %s\n", context, xcbErrorMessages[xcbErrorCode]);
}

void XcbReportGenericError( xcb_connection_t *conn, xcb_generic_error_t *err, const char *context )
{
	// TODO: Find a better way to report XCB generic errors
	LOG(Error, "Xcb generic error (%s)\n", context);
}

#endif



enum Key
{
	K_NULL,
	K_LEFT, K_RIGHT, K_UP, K_DOWN,
	K_ESCAPE,
	K_SPACE,
	K_BACKSPACE,
	K_DELETE,
	K_RETURN,
	K_TAB,
	K_CONTROL,
	K_SHIFT,
	K_ALT,
	K_0, K_1, K_2,
	K_3, K_4, K_5,
	K_6, K_7, K_8, K_9,
	K_A, K_B, K_C, K_D,
	K_E, K_F, K_G, K_H,
	K_I, K_J, K_K, K_L,
	K_M, K_N, K_O, K_P,
	K_Q, K_R, K_S, K_T,
	K_U, K_V, K_W, K_X,
	K_Y, K_Z,
	K_COUNT,
};

enum MouseButton
{
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_COUNT,
};

enum KeyState
{
	KEY_STATE_IDLE,
	KEY_STATE_PRESS,
	KEY_STATE_PRESSED,
	KEY_STATE_RELEASE,
};

enum ButtonState
{
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESS,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_RELEASE,
};

enum TouchState
{
	TOUCH_STATE_IDLE,
	TOUCH_STATE_PRESS,
	TOUCH_STATE_PRESSED,
	TOUCH_STATE_RELEASE,
};

struct Keyboard
{
	KeyState keys[K_COUNT];
};

struct Mouse
{
	i32 x, y;
	i32 dx, dy;
	ButtonState buttons[MOUSE_BUTTON_COUNT];
};

#define MAX_INPUT_CHARS 16

struct Chars
{
	char chars[MAX_INPUT_CHARS];
	u32 charCount;
	bool shift;
	bool ctrl;
	bool alt;
};

struct Touch
{
	f32 x0, y0;
	f32 x, y;
	f32 dx, dy;
	TouchState state;
};

bool KeyPress(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESS;
}

bool KeyPressed(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESSED;
}

bool KeyRelease(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_RELEASE;
}

bool ButtonPress(ButtonState state)
{
	return state == BUTTON_STATE_PRESS;
}

bool ButtonPressed(ButtonState state)
{
	return state == BUTTON_STATE_PRESSED;
}

bool ButtonRelease(ButtonState state)
{
	return state == BUTTON_STATE_RELEASE;
}

bool MouseMoved(const Mouse &mouse)
{
	return mouse.dx != 0 || mouse.dy != 0;
}

bool MouseButtonPress(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPress(mouse.buttons[button]);
}

bool MouseButtonPressed(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPressed(mouse.buttons[button]);
}

bool MouseButtonRelease(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonRelease(mouse.buttons[button]);
}

bool MouseButtonChanged(const Mouse &mouse, MouseButton button)
{
	return MouseButtonPress(mouse, button) || MouseButtonRelease(mouse, button);
}

bool MouseChanged(const Mouse &mouse)
{
	return MouseMoved(mouse)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_LEFT)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_RIGHT)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_MIDDLE);
}

enum WindowFlags
{
	WindowFlags_WasCreated  = 1 << 0,
	WindowFlags_WillDestroy = 1 << 1,
	WindowFlags_WasResized  = 1 << 2,
	WindowFlags_Exit        = 1 << 3,
};

struct Window
{
#if USE_XCB
	xcb_connection_t *connection;
	xcb_window_t window;
	xcb_atom_t closeAtom;
#elif USE_ANDROID
	ANativeWindow *nativeWindow;
#elif USE_WINAPI
	HINSTANCE hInstance;
	HWND hWnd;
#endif
	u32 width;
	u32 height;
	u32 flags;

	Keyboard keyboard;
	Mouse mouse;
	Chars chars;
	Touch touches[2];
};

struct Gamepad
{
	union {
		struct {
			ButtonState start;
			ButtonState back;
			ButtonState up;
			ButtonState down;
			ButtonState left;
			ButtonState right;
			ButtonState a;
			ButtonState b;
			ButtonState x;
			ButtonState y;
			ButtonState leftShoulder;
			ButtonState rightShoulder;
		};
		ButtonState buttons[12];
	};
	float leftTrigger;
	float rightTrigger;
	float2 leftAxis;
	float2 rightAxis;
};

struct Input
{
#if PLATFORM_WINDOWS
	DynamicLibrary library;
#elif PLATFORM_LINUX
	int fd; // file descriptor
#endif

	Gamepad gamepad;
};

#if PLATFORM_WINDOWS
typedef HRESULT FP_DirectSoundCreate( LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter );
#endif

struct Audio
{
	// Config
	u16 channelCount;
	u16 bytesPerSample;
	u16 samplesPerSecond;
	u16 bufferSize;
	u16 latencyFrameCount;
	u16 latencySampleCount;
	u16 safetyBytes;
	u32 runningSampleIndex;

#if PLATFORM_WINDOWS
	DynamicLibrary library;
	LPDIRECTSOUNDBUFFER buffer;
#elif PLATFORM_LINUX
	DynamicLibrary library;
	snd_pcm_t *pcm;
#endif

	bool initialized;
	bool isPlaying;
	bool soundIsValid;

	i16 *outputSamples;
};

struct SoundBuffer
{
	u16 samplesPerSecond;
	u16 sampleCount;
	i16* samples;
};

struct PlatformConfig
{
#if PLATFORM_ANDROID
	struct android_app *androidApp;
#endif // PLATFORM_ANDROID
};

#define MAX_SCRATCH_ARENAS 8

struct Platform
{
	// To be configured by the client app

	u32 globalMemorySize = MB(64);
	u32 frameMemorySize = MB(16);
	u32 stringMemorySize = KB(16);
	u32 dataMemorySize = MB(16);

	bool (*InitCallback)(Platform &);
	void (*UpdateCallback)(Platform &);
	void (*RenderAudioCallback)(Platform &, SoundBuffer &soundBuffer);
	void (*CleanupCallback)(Platform &);
	bool (*WindowInitCallback)(Platform &);
	void (*WindowCleanupCallback)(Platform &);

	void *userData;

#if PLATFORM_ANDROID
	struct android_app *androidApp;
#endif // PLATFORM_ANDROID

	// Platform components

	Arena globalArena;
	Arena frameArena;
	Arena stringArena;
	Arena dataArena;

	Arena scratchArenas[MAX_SCRATCH_ARENAS];
	volatile_u32 scratchArenaLockMask;

	StringInterning stringInterning;
	Window window;
	Input input;
	Audio audio;
	f32 deltaSeconds;
	f32 totalSeconds;
};

static Platform *sPlatform = nullptr;

u32 AcquireScratchArena(Arena &outArena)
{
	// Max MAX_SCRATCH_ARENAS attempts to get a scratch arena
	for (u32 i = 0; i < MAX_SCRATCH_ARENAS; ++i)
	{
		// Get the first bit not set
		const u32 oldValue = sPlatform->scratchArenaLockMask;
		const u32 index = FBZ(oldValue);
		ASSERT(index < MAX_SCRATCH_ARENAS);

		// Try to change it
		const u32 newValue = oldValue | (1<<index);
		if (AtomicSwap(&sPlatform->scratchArenaLockMask, oldValue, newValue))
		{
			Arena &arena = sPlatform->scratchArenas[index];
			if (!arena.base)
			{
				const u32 size = MB(1);
				arena.base = (byte*)AllocateVirtualMemory(size);
				arena.size = size;
			}
			outArena = arena;
			outArena.used = 0;
			return index;
		}
	}
	INVALID_CODE_PATH();
	return U32_MAX;
}

void ReleaseScratchArena(u32 index)
{
	const u32 oldValue = sPlatform->scratchArenaLockMask;
	const u32 newValue = oldValue & ~(1<<index);
	const bool swapped = AtomicSwap(&sPlatform->scratchArenaLockMask, oldValue, newValue);
	ASSERT(swapped);
}

struct Scratch
{
	Arena arena;
	u32 lockedBit;

	Scratch() { lockedBit = AcquireScratchArena(arena); }
	~Scratch() { ReleaseScratchArena(lockedBit); }
};


bool IsAbsolutePath(const char *path)
{
#if PLATFORM_LINUX || PLATFORM_ANDROID
	const bool res = *path == '/';
	return res;
#elif PLATFORM_WINDOWS
	const bool res = path[1] == ':' && path[0] >= 'A' && path[0] <= 'Z';
	return res;
#else
#error "Missing implementation"
#endif
}

void CanonicalizePath(char *path)
{
	struct PathPart
	{
		char *str;
		int len;
	};

	PathPart parts[32] = {};
	u32 partCount = 0;

	char *ptr = path;

	PathPart *currentPart = &parts[partCount++];
	currentPart->str = ptr;

	const bool addRootSeparator = (*ptr == '/');

	// Split path in parts
	// NOTE: This replaces separators by '\0' for easier string comparisons later
	while (*ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			if (currentPart->len > 0) {
				ASSERT(partCount < ARRAY_COUNT(parts));
				currentPart = &parts[partCount++];
			}
			currentPart->str = ptr + 1;
		} else {
			currentPart->len++;
		}
		ptr++;
	}

	//LOG(Debug, "Directory parts:\n");
	//for (u32 i = 0; i < partCount; ++i) {
	//	LOG(Debug, "- %.*s\n", parts[i].len,  parts[i].str);
	//}

	// Canonicalize
	u32 finalParts[32] = {};
	u32 finalPartCount = 0;
	for (u32 partIndex = 0; partIndex < partCount; ++partIndex)
	{
		const PathPart &part = parts[partIndex];
		if ( StrEq(part.str, ".") ) {
			// Do nothing
		} else if ( StrEq(part.str, "..") ) {
			// Remove previous part
			ASSERT(finalPartCount > 0);
			finalPartCount--;
		} else if ( part.len > 0 ) {
			finalParts[finalPartCount++] = partIndex;
		}
	}

	// Copy string back to buffer
	ptr = path;
	for (u32 i = 0; i < finalPartCount; ++i)
	{
		const u32 partIndex = finalParts[i];
		const PathPart &part = parts[partIndex];
		if ( i > 0 || addRootSeparator ) {
			*ptr++ = '/';
		}
		for (u32 c = 0; c < part.len; ++c) {
			*ptr++ = part.str[c];
		}
	}
	*ptr = 0;
}

void InitializeDirectories(Platform &platform, int argc, char **argv)
{
	char buffer[MAX_PATH_LENGTH];

#if PLATFORM_ANDROID

	// TODO: Don't hardcode this path here and get it from Android API.
	DataDir = "/sdcard/Android/data/com.tools.game/files";
	BinDir = DataDir;
	ProjectDir = "";

#else

#if PLATFORM_LINUX
	char *workingDir = getcwd(buffer, ARRAY_COUNT(buffer));
#elif PLATFORM_WINDOWS
	char *workingDir = _getcwd(buffer, ARRAY_COUNT(buffer));
#else
#error "Missing implementation"
#endif

	StrReplace(workingDir, '\\', '/'); // Make all separators '/'

	char exeDir[MAX_PATH_LENGTH] = {};
	if (argc > 0)
	{
		StrReplace(argv[0], '\\', '/'); // Make all separators '/'
		const char *exePath = argv[0];
		const char *lastSeparator = StrCharR(exePath, '/');
		const u32 length = lastSeparator ? lastSeparator - exePath : 0;
		StrCopyN(exeDir, exePath, length);
	}

	char directory[MAX_PATH_LENGTH];
	if ( !IsAbsolutePath(exeDir) )
	{
		StrCopy(directory, workingDir);
		StrCat(directory, "/");
	}
	StrCat(directory, exeDir);
	CanonicalizePath(directory);

	DataDir = PushString(platform.stringArena, directory);
	BinDir = DataDir;

	StrCat(directory, "/..");
	CanonicalizePath(directory);
	ProjectDir = PushString(platform.stringArena, directory);

	StrCopy(directory, ProjectDir);
	StrCat(directory, "/assets");
	AssetDir = PushString(platform.stringArena, directory);

#endif

	LOG(Info, "Directories:\n");
	LOG(Info, "- BinDir: %s\n", BinDir);
	LOG(Info, "- DataDir: %s\n", DataDir);
	LOG(Info, "- AssetDir: %s\n", AssetDir);
	LOG(Info, "- ProjectDir: %s\n", ProjectDir);
}



#if USE_XCB && 0
void PrintModifiers(uint32_t mask)
{
	const char **mod, *mods[] = {
		"Shift", "Lock", "Ctrl", "Alt",
		"Mod2", "Mod3", "Mod4", "Mod5",
		"Button1", "Button2", "Button3", "Button4", "Button5"
	};
	LOG(Info, "Modifier mask: ");
	for (mod = mods ; mask; mask >>= 1, mod++)
		if (mask & 1)
			LOG(Info, *mod);
	putchar ('\n');
}
#endif



#if USE_XCB

#include "xcb_key_mappings.h"

#if USE_IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API bool ImGui_ImplXcb_HandleInputEvent(xcb_generic_event_t *event);
#endif

void XcbWindowProc(Window &window, xcb_generic_event_t *event)
{
#if USE_IMGUI
	if (ImGui_ImplXcb_HandleInputEvent(event))
		return;
#endif

	u32 eventType = event->response_type & ~0x80;

	switch ( eventType )
	{
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
			{
				// NOTE: xcb_key_release_event_t is an alias of xcb_key_press_event_t
				xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
				u32 keyCode = ev->detail;
				ASSERT( keyCode < ARRAY_COUNT(XcbKeyMappings) );
				u32 mapping = XcbKeyMappings[ keyCode ];
				ASSERT( mapping < K_COUNT );
				KeyState state = eventType == XCB_KEY_PRESS ? KEY_STATE_PRESS : KEY_STATE_RELEASE;
				window.keyboard.keys[ mapping ] = state;
				break;
			}

		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			{
				// NOTE: xcb_button_release_event_t is an alias of xcb_button_press_event_t
				xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
				ButtonState state = eventType == XCB_BUTTON_PRESS ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE;
				switch (ev->detail) {
					case 1: window.mouse.buttons[ MOUSE_BUTTON_LEFT ] = state; break;
					case 2: window.mouse.buttons[ MOUSE_BUTTON_MIDDLE ] = state; break;
					case 3: window.mouse.buttons[ MOUSE_BUTTON_RIGHT ] = state; break;
					//case 4: // wheel up
					//case 5: // wheel down
					default:;
				}
				break;
			}

		case XCB_MOTION_NOTIFY:
			{
				xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;
				window.mouse.dx = static_cast<i32>(ev->event_x) - static_cast<i32>(window.mouse.x);
				window.mouse.dy = static_cast<i32>(ev->event_y) - static_cast<i32>(window.mouse.y);
				window.mouse.x = ev->event_x;
				window.mouse.y = ev->event_y;
				break;
			}

		case XCB_ENTER_NOTIFY:
			{
				xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;
				//LOG(Info, "Mouse entered window %ld, at coordinates (%d,%d)\n",
				//		ev->event, ev->event_x, ev->event_y);
				break;
			}

		case XCB_LEAVE_NOTIFY:
			{
				xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;
				//LOG(Info, "Mouse left window %ld, at coordinates (%d,%d)\n",
				//		ev->event, ev->event_x, ev->event_y);
				break;
			}

		case XCB_CONFIGURE_NOTIFY:
			{
				const xcb_configure_notify_event_t *ev = (const xcb_configure_notify_event_t *)event;
				if ( window.width != ev->width || window.height != ev->height )
				{
					window.width = ev->width;
					window.height = ev->height;
					window.flags |= WindowFlags_WasResized;
				}
				break;
			}

		case XCB_CLIENT_MESSAGE:
			{
				const xcb_client_message_event_t *ev = (const xcb_client_message_event_t *)event;
				if ( ev->data.data32[0] == window.closeAtom )
				{
					window.flags |= WindowFlags_WillDestroy;
					window.flags |= WindowFlags_Exit;
				}
				break;
			}

		case XCB_MAP_NOTIFY:
			// TODO: Handle this event
			break;

		case XCB_REPARENT_NOTIFY:
			// TODO: Handle this event
			break;

		default:
			/* Unknown event type, ignore it */
			LOG(Info, "Unknown window event: %d\n", event->response_type);
			break;
	}

#if USE_IMGUI
	const Mouse &mouse = window.mouse;

	ImGuiIO& io = ImGui::GetIO();
	if (MouseChanged(mouse))
	{
		if (MouseMoved(mouse))
		{
			io.AddMousePosEvent(mouse.x, mouse.y);
		}
		int button = -1;
		bool press = true;
		if (MouseButtonPress(mouse, MOUSE_BUTTON_LEFT)) { button = 0; press = true; }
		if (MouseButtonPress(mouse, MOUSE_BUTTON_RIGHT)) { button = 1; press = true; }
		if (MouseButtonPress(mouse, MOUSE_BUTTON_MIDDLE)) { button = 2; press = true; }
		if (MouseButtonRelease(mouse, MOUSE_BUTTON_LEFT)) { button = 0; press = false; }
		if (MouseButtonRelease(mouse, MOUSE_BUTTON_RIGHT)) { button = 1; press = false; }
		if (MouseButtonRelease(mouse, MOUSE_BUTTON_MIDDLE)) { button = 2; press = false; }
		if ( button != -1 )
		{
			io.AddMouseButtonEvent(button, press);
			io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
		}
	}

	// TODO: When we detect the mouse leves the window
	// io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);

	// Clear mouse/keyboard events if handled by ImGui
	if ( io.WantCaptureMouse ) window.mouse = {};
	if ( io.WantCaptureKeyboard ) window.keyboard = {};
#endif // USE_IMGUI
}

#elif USE_ANDROID

/**
 * Process the next main command.
 * enum NativeAppGlueAppCmd {
 *   UNUSED_APP_CMD_INPUT_CHANGED = 0
 *   APP_CMD_INIT_WINDOW = 1
 *   APP_CMD_TERM_WINDOW = 2
 *   APP_CMD_WINDOW_RESIZED = 3
 *   APP_CMD_WINDOW_REDRAW_NEEDED = 4
 *   APP_CMD_CONTENT_RECT_CHANGED = 5
 *   APP_CMD_GAINED_FOCUS = 6
 *   APP_CMD_LOST_FOCUS = 7
 *   APP_CMD_CONFIG_CHANGED = 8
 *   APP_CMD_LOW_MEMORY = 9
 *   APP_CMD_START = 10
 *   APP_CMD_RESUME = 11
 *   APP_CMD_SAVE_STATE = 12
 *   APP_CMD_PAUSE = 13
 *   APP_CMD_STOP = 14
 *   APP_CMD_DESTROY = 15
 *   APP_CMD_WINDOW_INSETS_CHANGED = 16
 * }
 */
void AndroidHandleAppCommand(struct android_app *app, int32_t cmd)
{
	Platform *platform = (Platform*)app->userData;

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			// The window is being shown, get it ready.
			ASSERT(app->window != NULL);
			if (app->window && app->window != platform->window.nativeWindow)
			{
				platform->window.nativeWindow = app->window;
				platform->window.flags |= WindowFlags_WasCreated;
			}
			break;
		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			platform->window =  {};
			platform->window.flags |= WindowFlags_WillDestroy;
			break;
		case APP_CMD_WINDOW_RESIZED:
			{
				int32_t newWidth = ANativeWindow_getWidth(app->window);
				int32_t newHeight = ANativeWindow_getHeight(app->window);
				if ( newWidth != platform->window.width || newHeight != platform->window.height )
				{
					platform->window.width = newWidth;
					platform->window.height = newHeight;
					platform->window.flags |= WindowFlags_WasResized;
				}
			}
			break;
		//case APP_CMD_WINDOW_REDRAW_NEEDED: break;
		//case APP_CMD_CONTENT_RECT_CHANGED: break;
		//case APP_CMD_GAINED_FOCUS: break;
		//case APP_CMD_LOST_FOCUS: break;
		//case APP_CMD_CONFIG_CHANGED: break;
		//case APP_CMD_LOW_MEMORY: break;
		//case APP_CMD_START: break;
		//case APP_CMD_RESUME: break;
		//case APP_CMD_SAVE_STATE: break;
		case APP_CMD_PAUSE:
			{
				// Gets activated at APP_CMD_INIT_WINDOW
			}
			break;
		//case APP_CMD_STOP: break;
		//case APP_CMD_DESTROY: break;
		//case APP_CMD_WINDOW_INSETS_CHANGED: break;
		default:
			//LOG( Info, "UNKNOWN ANDROID COMMAND: %d\n", cmd);
			break;
	}
	//LOG( Info, "ANDROID APP COMMAND: %d\n", cmd);
}

int32_t AndroidHandleInputEvent(struct android_app *app, AInputEvent *event)
{
	Platform *platform = (Platform*)app->userData;

	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		const int32_t actionAndPointer = AMotionEvent_getAction( event );
		const uint32_t action = actionAndPointer & AMOTION_EVENT_ACTION_MASK;
		const uint32_t pointerIndex = (actionAndPointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
		const uint32_t pointerCount = AMotionEvent_getPointerCount(event);
		const float x = AMotionEvent_getX(event, pointerIndex);
		const float y = AMotionEvent_getY(event, pointerIndex);

		if (pointerId < ARRAY_COUNT(platform->window.touches))
		{
			Touch *touches = platform->window.touches;

			switch( action )
			{
				case AMOTION_EVENT_ACTION_DOWN:
				case AMOTION_EVENT_ACTION_POINTER_DOWN:
					{
						touches[pointerId].state = TOUCH_STATE_PRESS;
						touches[pointerId].x0 = x;
						touches[pointerId].y0 = y;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
				case AMOTION_EVENT_ACTION_UP:
				case AMOTION_EVENT_ACTION_POINTER_UP:
					{
						touches[pointerId].state = TOUCH_STATE_RELEASE;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
				case AMOTION_EVENT_ACTION_MOVE:
					// On move ements, we are meant to handle all pointers in the gesture
					for (u32 pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex)
					{
						const float x = AMotionEvent_getX(event, pointerIndex);
						const float y = AMotionEvent_getY(event, pointerIndex);
						const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
						touches[pointerId].dx = x - touches[pointerId].x;
						touches[pointerId].dy = y - touches[pointerId].y;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
			}
		}
		return 1;
	}
	return 0;
}

#elif USE_WINAPI

#include "win32_key_mappings.h"

#if USE_IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT CALLBACK Win32WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool processMouseEvents = true;
	bool processKeyboardEvents = true;
#if USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return true;
	}
	if (ImGui::GetCurrentContext() != nullptr)
	{
		const ImGuiIO& io = ImGui::GetIO();
		processKeyboardEvents = !io.WantCaptureKeyboard;
		processMouseEvents = !io.WantCaptureMouse;
	}
#endif

	Window *window = (Window*)GetPropA(hWnd, "WindowPtr");


	switch (uMsg)
	{
		case WM_KEYDOWN:
		case WM_KEYUP:
			if ( processKeyboardEvents )
			{
				ASSERT(window);
				WPARAM keyCode = wParam;
				ASSERT( keyCode < ARRAY_COUNT(Win32KeyMappings) );
				u32 mapping = Win32KeyMappings[ keyCode ];
				ASSERT( mapping < K_COUNT );
				KeyState state = uMsg == WM_KEYDOWN ? KEY_STATE_PRESS : KEY_STATE_RELEASE;
				window->keyboard.keys[ mapping ] = state;
			}
			break;

		case WM_SYSCHAR:
			// If this message is not handled the default window procedure will
			// play a system notification sound when Alt+Enter is pressed.
			break;

		case WM_LBUTTONDOWN:
			if ( processMouseEvents )
			{
				//int xPos = GET_X_LPARAM(lParam);
				//int yPos = GET_Y_LPARAM(lParam);
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_LEFT] = BUTTON_STATE_PRESS;
			}
			break;
		case WM_LBUTTONUP:
			if ( processMouseEvents )
			{
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_LEFT] = BUTTON_STATE_RELEASE;
			}
			break;
		case WM_RBUTTONDOWN:
			if ( processMouseEvents )
			{
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_RIGHT] = BUTTON_STATE_PRESS;
			}
			break;
		case WM_RBUTTONUP:
			if ( processMouseEvents )
			{
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_RIGHT] = BUTTON_STATE_RELEASE;
			}
			break;
		case WM_MBUTTONDOWN:
			if ( processMouseEvents )
			{
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_MIDDLE] = BUTTON_STATE_PRESS;
			}
			break;
		case WM_MBUTTONUP:
			if ( processMouseEvents )
			{
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_MIDDLE] = BUTTON_STATE_RELEASE;
			}
			break;

		case WM_MOUSEMOVE:
			if ( processMouseEvents )
			{
				ASSERT(window);
				i32 xPos = GET_X_LPARAM(lParam);
				i32 yPos = GET_Y_LPARAM(lParam);
				window->mouse.dx = xPos - window->mouse.x;
				window->mouse.dy = yPos - window->mouse.y;
				window->mouse.x = xPos;
				window->mouse.y = yPos;
				//LOG( Info, "Mouse at position (%d, %d)\n", xPos, yPos );
			}
			break;

		case WM_MOUSEHOVER:
		case WM_MOUSELEAVE:
			{
				// These events are disabled by default. See documentation if needed:
				// https://learn.microsoft.com/en-us/windows/win32/learnwin32/other-mouse-operations
				//LOG( Info, "Mouse %s the window\n", uMsg == WM_MOUSEHOVER ? "entered" : "left" );
				break;
			}

		case WM_SIZE:
			{
				ASSERT(window);
				i32 width = LOWORD(lParam);
				i32 height = HIWORD(lParam);
				if ( window->width != width || window->height != height )
				{
					window->width = Max(width, 0);
					window->height = Max(height, 0);
					window->flags |= WindowFlags_WasResized;
				}
				break;
			}

		case WM_SYSCOMMAND:
			{
				WPARAM param = ( wParam & 0xFFF0 );
				Audio &audio = sPlatform->audio;

				if (param == SC_MINIMIZE)
				{
					if ( audio.initialized && audio.isPlaying ) {
						sPlatform->audio.buffer->Stop();
						audio.isPlaying = false;
					}
				}
				else if (param == SC_RESTORE)
				{
					if ( audio.initialized && !audio.isPlaying ) {
						sPlatform->audio.buffer->Play(0, 0, DSBPLAY_LOOPING);
						audio.isPlaying = true;
						sPlatform->audio.soundIsValid = false;
					}
				}

				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			};

		case WM_CLOSE:
			{
				// If we want to show a dialog to ask the user for confirmation before
				// closing the window, it should be done here. Zero should be returned
				// to indicate that we handled this message.
				// Otherwise, calling DefWindowProc will internally call DestroyWindow
				// and will internally send the WM_DESTROY message.
				DestroyWindow(hWnd);
				break;
			}

		case WM_DESTROY:
			{
				// This inserts a WM_QUIT message in the queue, which will in turn cause
				// GetMessage to return zero. We will exit the main loop when that happens.
				// On the other hand, PeekMessage has to handle WM_QUIT messages explicitly.
				window->flags |= WindowFlags_WillDestroy;
				PostQuitMessage(0);
				break;
			}

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

#endif



bool InitializeWindow(
		Window &window,
		u32 width = 640,
		u32 height = 480,
		const char *title = "Example window"
		)
{
	ZeroStruct(&window);
	window.width = width;
	window.height = height;

#if USE_XCB

	// Connect to the X server
	xcb_connection_t *xcbConnection = xcb_connect(NULL, NULL);

	int xcbConnError = xcb_connection_has_error(xcbConnection);
	if ( xcbConnError > 0 )
	{
		XcbReportError(xcbConnError, "xcb_connect");
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Get the first screen
	const xcb_setup_t *setup = xcb_get_setup(xcbConnection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t * screen = iter.data;

	// Configure events to capture
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2] = {
		screen->black_pixel,
		XCB_EVENT_MASK_KEY_PRESS       | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS    | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION  |
		XCB_EVENT_MASK_ENTER_WINDOW    | XCB_EVENT_MASK_LEAVE_WINDOW   |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY
	};

	// Create a window
	xcb_window_t xcbWindow = xcb_generate_id(xcbConnection);
	xcb_void_cookie_t createWindowCookie = xcb_create_window_checked(
		xcbConnection,                 // xcb connection
		XCB_COPY_FROM_PARENT,          // depth
		xcbWindow,                     // window id
		screen->root,                  // parent window
		0, 0,                          // x, y
		width, height,                 // width, height
		0,                             // bnorder_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		screen->root_visual,           // visual
		mask, values);                 // value_mask, value_list

	xcb_generic_error_t *createWindowError = xcb_request_check(xcbConnection, createWindowCookie);
	if ( createWindowError )
	{
		XcbReportGenericError(xcbConnection, createWindowError, "xcb_create_window_checked");
		xcb_destroy_window(xcbConnection, xcbWindow);
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Handle close event
	// TODO: Handle xcb_intern_atom errors
	xcb_intern_atom_cookie_t protocolCookie = // handle error
		xcb_intern_atom_unchecked( xcbConnection, 1, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *protocolReply =
		xcb_intern_atom_reply( xcbConnection, protocolCookie, 0);
	xcb_intern_atom_cookie_t closeCookie = // handle error
		xcb_intern_atom_unchecked( xcbConnection, 0, 16, "WM_DELETE_WINDOW");
	xcb_intern_atom_reply_t *closeReply =
		xcb_intern_atom_reply( xcbConnection, closeCookie, 0);
	u8 dataFormat = 32;
	u32 dataLength = 1;
	void *data = &closeReply->atom;
	xcb_change_property( // handle error
			xcbConnection,
			XCB_PROP_MODE_REPLACE,
			xcbWindow,
			protocolReply->atom, XCB_ATOM_ATOM,
			dataFormat, dataLength, data);
	xcb_atom_t closeAtom = closeReply->atom;
	free(protocolReply);
	free(closeReply);

	// Map the window to the screen
	xcb_void_cookie_t mapWindowCookie = xcb_map_window_checked(xcbConnection, xcbWindow);
	xcb_generic_error_t *mapWindowError = xcb_request_check(xcbConnection, mapWindowCookie);
	if ( mapWindowError )
	{
		XcbReportGenericError(xcbConnection, mapWindowError, "xcb_map_window_checked");
		xcb_destroy_window(xcbConnection, xcbWindow);
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Flush the commands before continuing
	xcb_flush(xcbConnection);

	// Get window info at this point
	xcb_get_geometry_cookie_t cookie= xcb_get_geometry( xcbConnection, xcbWindow ); // handle error
	xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply( xcbConnection, cookie, NULL ); // handle error

	window.connection = xcbConnection;
	window.window = xcbWindow;
	window.closeAtom = closeAtom;
	window.width = reply->width;
	window.height = reply->height;

	window.flags = WindowFlags_WasCreated;

#endif

#if USE_WINAPI

	// Register the window class.
	const char CLASS_NAME[]  = "Sample Window Class";
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = Win32WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	ATOM classAtom = RegisterClassA(&wc);

	if (classAtom == 0)
	{
		Win32ReportError("InitializeWindow - RegisterClassA");
		return false;
	}

	// Given the desired client window size, get the full size
	RECT windowRect = { 0, 0, (int)width, (int)height };
	AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, FALSE );
	int fullWidth = windowRect.right - windowRect.left;
	int fullHeight = windowRect.bottom - windowRect.top;

	HWND hWnd = CreateWindowExA(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			title,                          // Window text
			WS_OVERLAPPEDWINDOW,            // Window style
			CW_USEDEFAULT, CW_USEDEFAULT,   // Position
			fullWidth, fullHeight,          // Size
			NULL,                           // Parent window
			NULL,                           // Menu
			hInstance,                      // Instance handle
			NULL                            // Additional application data
			);

	if (hWnd == NULL)
	{
		Win32ReportError("InitializeWindow - CreateWindowExA");
		return false;
	}

	if ( !SetPropA(hWnd, "WindowPtr", &window) )
	{
		Win32ReportError("InitializeWindow - SetPropA");
		return false;
	}

	ShowWindow(hWnd, SW_SHOW);

	window.hInstance = hInstance;
	window.hWnd = hWnd;

	window.flags = WindowFlags_WasCreated;

#endif

	return true;
}


void CleanupWindow(Window &window)
{
#if USE_XCB
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#elif USE_WINAPI
	DestroyWindow(window.hWnd);
#endif
}


void PlatformUpdateEventLoop(Platform &platform)
{
	Window &window = platform.window;

	// Transition key states
	for ( u32 i = 0; i < K_COUNT; ++i ) {
		if ( window.keyboard.keys[i] == KEY_STATE_PRESS ) {
			window.keyboard.keys[i] = KEY_STATE_PRESSED;
		} else if ( window.keyboard.keys[i] == KEY_STATE_RELEASE ) {
			window.keyboard.keys[i] = KEY_STATE_IDLE;
		}
	}

	// Transition mouse button states
	for ( u32 i = 0; i < MOUSE_BUTTON_COUNT; ++i ) {
		if ( window.mouse.buttons[i] == BUTTON_STATE_PRESS ) {
			window.mouse.buttons[i] = BUTTON_STATE_PRESSED;
		} else if ( window.mouse.buttons[i] == BUTTON_STATE_RELEASE ) {
			window.mouse.buttons[i] = BUTTON_STATE_IDLE;
		}
	}

	window.mouse.dx = 0.0f;
	window.mouse.dy = 0.0f;

	window.chars.charCount = 0;

	// Transition touch states
	for ( u32 i = 0; i < ARRAY_COUNT(window.touches); ++i ) {
		if ( window.touches[i].state == TOUCH_STATE_PRESS ) {
			window.touches[i].state = TOUCH_STATE_PRESSED;
		} else if ( window.touches[i].state == TOUCH_STATE_RELEASE ) {
			window.touches[i].state = TOUCH_STATE_IDLE;
		}
		window.touches[i].dx = 0.0f;
		window.touches[i].dy = 0.0f;
	}

#if USE_XCB

	xcb_generic_event_t *event;
	while ( (event = xcb_poll_for_event(window.connection)) != 0 )
	{
		XcbWindowProc(window, event);
		free(event);
	}

#elif USE_ANDROID

	// Read all pending events.
	int ident;
	int events;
	struct android_poll_source* source;

	const int kWaitForever = -1;
	const int kDontWait = 0;
	while ((ident=ALooper_pollAll(kDontWait, NULL, &events, (void**)&source)) >= 0)
	{
		// Process this event.
		if (source != NULL)
		{
			source->process(platform.androidApp, source);
		}

		// Check if we are exiting.
		if (platform.androidApp->destroyRequested != 0)
		{
			LOG(Info, "androidApp->destroyRequesteds\n");
			window.flags |= WindowFlags_Exit;
		}
	}

#elif USE_WINAPI

	MSG msg = { };
	while ( PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		if ( LOWORD( msg.message ) == WM_QUIT )
		{
			window.flags |= WindowFlags_Exit;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#endif

	// Update key modifiers
	window.chars.shift = KeyPressed(window.keyboard, K_SHIFT);
	window.chars.ctrl = KeyPressed(window.keyboard, K_CONTROL);
	window.chars.alt = KeyPressed(window.keyboard, K_ALT);

	for ( u32 i = 0; i < K_COUNT; ++i )
	{
		if ( KeyPress(window.keyboard, (Key)i) )
		{
			char character = 0;

			if ( i >= K_A && i <= K_Z ) {
				character = window.chars.shift ? 'A' + (i - K_A) : 'a' + (i - K_A);
			} else if ( i >= K_0 && i <= K_9 ) {
				character = '0' + (i - K_0);
			} else if ( i == K_SPACE ) {
				character = ' ';
			}

			if (character)
			{
				window.chars.chars[window.chars.charCount++] = character;
			}
		}
	}
}

#if PLATFORM_WINDOWS

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD userIndex, XINPUT_STATE *state)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD userIndex, XINPUT_VIBRATION *vibration)

typedef X_INPUT_GET_STATE(XInputGetState_t);
typedef X_INPUT_SET_STATE(XInputSetState_t);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_INVALID_FUNCTION;
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_INVALID_FUNCTION;
}

XInputGetState_t *FP_XInputGetState = XInputGetStateStub;
XInputSetState_t *FP_XInputSetState = XInputSetStateStub;

#endif // PLATFORM_WINDOWS

bool InitializeGamepad(Platform &platform)
{
	LOG(Info, "Input system initialization:\n");

	Input &input = platform.input;

#if PLATFORM_WINDOWS

	const char *libraryNames[] = {
		"xinput1_4.dll",
		"xinput9_1_0.dll",
		"xinput1_3.dll",
	};

	const char *libraryName = nullptr;
	for (u32 i = 0; i < ARRAY_COUNT(libraryNames); ++i) {
		libraryName = libraryNames[i];
		input.library = OpenLibrary(libraryName);
		if (input.library) {
			break;
		}
	}

	if (input.library)
	{
		LOG(Info, "- Loaded %s successfully\n", libraryName);

		XInputGetState_t* getState = (XInputGetState_t*)LoadSymbol(input.library, "XInputGetState");
		XInputSetState_t* setState = (XInputSetState_t*)LoadSymbol(input.library, "XInputSetState");

		if ( getState != nullptr ) {
			LOG(Info, "- XInputGetState symbol loaded successfully\n");
			FP_XInputGetState = getState;
		} else {
			LOG(Warning, "- Error loading XInputGetState symbol\n");
			FP_XInputGetState = XInputGetStateStub;
		}
		if ( setState != nullptr ) {
			LOG(Info, "- XInputSetState symbol loaded successfully\n");
			FP_XInputSetState = setState;
		} else {
			LOG(Warning, "- Error loading XInputSetState\n");
			FP_XInputSetState = XInputSetStateStub;
		}
	}

#elif PLATFORM_LINUX

	input.fd = -1;

	char path[MAX_PATH_LENGTH] = {};
	char name[MAX_PATH_LENGTH] = {};
	const char *dirName = "/dev/input";
	bool found = false;

	Dir dir = {};

	if ( OpenDir(dir, dirName) )
	{
		DirEntry entry = {};

		while ( !found && ReadDir(dir, entry) )
		{
			SPrintf(path, "%s/%s", dirName, entry.name);

			if (StrEqN(entry.name, "event", 5))
			{
				int fd = open(path, O_RDONLY | O_NONBLOCK);
				if (fd < 0) {
					continue;
				}

				if ( ioctl(fd, EVIOCGNAME(sizeof(name)), name) != -1 )
				{
					LOG(Info, "- Device path: %s\n", path);
					LOG(Info, "- Device name: %s\n", name);
					input.fd = fd;
					found = true;
				}
				else
				{
					close(fd);
				}
			}
		}

		CloseDir(dir);
	}

	return found;

#else

	LOG(Info, "- Missing implementation\n");

#endif

	return false;
}

#if PLATFORM_WINDOWS

static ButtonState ButtonStateFromXInput(WORD prevButtonMask, WORD currButtonMask, WORD buttonBit)
{
	const u32 wasDown = ( prevButtonMask & buttonBit ) ? 1 : 0;
	const u32 isDown = ( currButtonMask & buttonBit ) ? 1 : 0;
	constexpr ButtonState stateMatrix[2][2] = {
		// isUp,  isDown
		{ BUTTON_STATE_IDLE , BUTTON_STATE_PRESS }, // wasUp
		{ BUTTON_STATE_RELEASE ,BUTTON_STATE_PRESSED }, // wasDown
	};
	const ButtonState state = stateMatrix[wasDown][isDown];
	return state;
}

static f32 TriggerFromXInput(BYTE trigger)
{
	const f32 res = (f32)trigger/255.0f;
	return res;
}

static f32 AxisFromXInput(SHORT axis, SHORT deadzoneThreshold)
{
	f32 normalizedAxis = 0.0f;
	if (axis < -deadzoneThreshold) {
		normalizedAxis = (f32)(axis + deadzoneThreshold)/(32768.0f - deadzoneThreshold);
	} else if (axis > deadzoneThreshold) {
		normalizedAxis = (f32)(axis - deadzoneThreshold)/(32767.0f - deadzoneThreshold);
	}
	return normalizedAxis;
}

#endif // PLATFORM_WINDOWS

#if PLATFORM_LINUX
static ButtonState ButtonStateFromEvent(i32 value)
{
	const ButtonState res = (value == 1) ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE;
	return res;
}

#define GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define GAMEPAD_RIGHT_THUMB_DEADZONE 8689

static f32 AxisFromEvent(i32 axis, i32 deadzoneThreshold)
{
	f32 normalizedAxis = 0.0f;
	if (axis < -deadzoneThreshold) {
		normalizedAxis = (f32)(axis + deadzoneThreshold)/(32768.0f - deadzoneThreshold);
	} else if (axis > deadzoneThreshold) {
		normalizedAxis = (f32)(axis - deadzoneThreshold)/(32767.0f - deadzoneThreshold);
	}
	return normalizedAxis;
}

static f32 TriggerFromEvent(i32 value)
{
	//ASSERT(value >= 0 && value < 256);
	static f32 maxValue = 256.0f;
	maxValue = Max(maxValue, (f32)value); // TODO(jesus): Query per-device limits with ioctl
	const f32 res = (f32)value/maxValue;
	return res;
}

static ButtonState DPadStateFromEvent(ButtonState prevState, i32 expectedValue, i32 value)
{
	ButtonState state = prevState;
	if (expectedValue == value) {
		state = BUTTON_STATE_PRESS;
	} else if (prevState != BUTTON_STATE_IDLE) {
		state = BUTTON_STATE_RELEASE;
	}
	return state;
}
#endif // PLATFORM_LINUX

void UpdateGamepad(Platform &platform)
{
	Gamepad &gamepad = platform.input.gamepad;

#if PLATFORM_WINDOWS

	for ( DWORD i = 0; i < XUSER_MAX_COUNT; ++i )
	{
		XINPUT_STATE controllerState;
		if (FP_XInputGetState(i, &controllerState) == ERROR_SUCCESS)
		{
			static XINPUT_GAMEPAD prevXPad = {};

			//int packetNumber = controllerState.dwPacketNumber;
			const XINPUT_GAMEPAD &xpad = controllerState.Gamepad;

			gamepad.start = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_START);
			gamepad.back = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_BACK);
			gamepad.up = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_UP);
			gamepad.down = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
			gamepad.left = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
			gamepad.right = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
			gamepad.a = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_A);
			gamepad.b = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_B);
			gamepad.x = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_X);
			gamepad.y = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_Y);
			gamepad.leftShoulder = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
			gamepad.rightShoulder = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
			gamepad.leftTrigger = TriggerFromXInput(xpad.bLeftTrigger);
			gamepad.rightTrigger = TriggerFromXInput(xpad.bRightTrigger);
			gamepad.leftAxis.x = AxisFromXInput(xpad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad.leftAxis.y = AxisFromXInput(xpad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad.rightAxis.x = AxisFromXInput(xpad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			gamepad.rightAxis.y = AxisFromXInput(xpad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

			prevXPad = xpad;

			// Only one gamepad supported
			break;
		}
		else
		{
			// Controller not available
		}
	}

#elif PLATFORM_LINUX

	if (platform.input.fd != -1)
	{
		// Update button states
		for (u32 i = 0; i < ARRAY_COUNT(gamepad.buttons); ++i) {
			if (gamepad.buttons[i] == BUTTON_STATE_PRESS) {
				gamepad.buttons[i] = BUTTON_STATE_PRESSED;
			} else if (gamepad.buttons[i] == BUTTON_STATE_RELEASE) {
			}
		}

		ssize_t size = 0;
		input_event event;

		while ( (size = read(platform.input.fd, &event, sizeof(event))) != -1 )
		{
			const u32 type = event.type;
			const u32 code = event.code;
			const i32 value = event.value;

			if (type == EV_KEY) {
				switch (code) {
					case BTN_START: gamepad.start = ButtonStateFromEvent(value); break;
					case BTN_SELECT: gamepad.back = ButtonStateFromEvent(value); break;
					//case BTN_MODE: codeStr = ButtonStateFromEvent(value); break;
					case BTN_TL: gamepad.leftShoulder = ButtonStateFromEvent(value); break;
					case BTN_TR: gamepad.rightShoulder = ButtonStateFromEvent(value); break;
					case BTN_A: gamepad.a = ButtonStateFromEvent(value); break;
					case BTN_B: gamepad.b = ButtonStateFromEvent(value); break;
					case BTN_X: gamepad.x = ButtonStateFromEvent(value); break;
					case BTN_Y: gamepad.y = ButtonStateFromEvent(value); break;
					//case BTN_THUMBL: codeStr = ButtonStateFromEvent(gamepad., value); break;
					//case BTN_THUMBR: codeStr = ButtonStateFromEvent(gamepad., value); break;
					default:;
				}
			} else if (type == EV_ABS) {
				switch (code) {
					case ABS_X: gamepad.leftAxis.x = AxisFromEvent(value, GAMEPAD_LEFT_THUMB_DEADZONE); break;
					case ABS_Y: gamepad.leftAxis.y = AxisFromEvent(-value, GAMEPAD_LEFT_THUMB_DEADZONE); break;
					case ABS_RX: gamepad.rightAxis.x = AxisFromEvent(value, GAMEPAD_RIGHT_THUMB_DEADZONE); break;
					case ABS_RY: gamepad.rightAxis.y = AxisFromEvent(-value, GAMEPAD_RIGHT_THUMB_DEADZONE); break;
					case ABS_Z: gamepad.leftTrigger = TriggerFromEvent(value); break;
					case ABS_RZ: gamepad.rightTrigger = TriggerFromEvent(value); break;
					case ABS_HAT0X:
						gamepad.left = DPadStateFromEvent(gamepad.left, -1, value);
						gamepad.right = DPadStateFromEvent(gamepad.right, 1, value);
						break;
					case ABS_HAT0Y:
						gamepad.up = DPadStateFromEvent(gamepad.up, -1, value);
						gamepad.down = DPadStateFromEvent(gamepad.down, 1, value);
						break;
				}
			} else if (type == EV_MSC) {
				// MSC event (not sure what this is)
			} else if (type == EV_SYN) {
				// Synchronization event (not sure what this is)
			} else {
				LOG(Warning, "- Unknown event type\n");
			}
		}

		if (errno != EAGAIN)
		{
			LOG(Warning, "Error reading gamepad input\n");
		}
	}

#endif
}

#if PLATFORM_WINDOWS
void Win32FillAudioBuffer(Audio &audio, DWORD writeOffset, DWORD writeSize, const i16 *audioSamples);
#endif

#if PLATFORM_LINUX

typedef const char * SND_STRERROR (int errnum);
typedef int SND_PCM_OPEN(snd_pcm_t **pcmp, const char * name, snd_pcm_stream_t stream, int	mode );
typedef int SND_PCM_HW_PARAMS_MALLOC(snd_pcm_hw_params_t **ptr);
typedef int SND_PCM_HW_PARAMS_ANY(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int SND_PCM_HW_PARAMS_SET_ACCESS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
typedef int SND_PCM_HW_PARAMS_SET_FORMAT(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
typedef int SND_PCM_HW_PARAMS_SET_CHANNELS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
typedef int SND_PCM_HW_PARAMS_SET_RATE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
typedef int SND_PCM_HW_PARAMS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int SND_PCM_HW_PARAMS_GET_CHANNELS(const snd_pcm_hw_params_t *params, unsigned int *channelCount);
typedef int SND_PCM_HW_PARAMS_GET_RATE(const snd_pcm_hw_params_t *params, unsigned int *sampleRate, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_FORMAT(const snd_pcm_hw_params_t *params, snd_pcm_format_t *format);
typedef int SND_PCM_HW_PARAMS_GET_ACCESS(const snd_pcm_hw_params_t *params, snd_pcm_access_t *access);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_TIME(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_SIZE(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
typedef int SND_PCM_AVAIL_DELAY(snd_pcm_t *pcm, snd_pcm_sframes_t *availp, snd_pcm_sframes_t *delayp);
typedef snd_pcm_sframes_t SND_PCM_WRITEI(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
typedef int SND_PCM_RECOVER(snd_pcm_t *pcm, int err, int silent);
typedef int SND_PCM_PREPARE(snd_pcm_t *pcm);
typedef int SND_PCM_CLOSE(snd_pcm_t *pcm);
typedef int SND_PCM_DRAIN(snd_pcm_t *pcm);

SND_STRERROR* FP_snd_strerror;
SND_PCM_OPEN* FP_snd_pcm_open;
SND_PCM_HW_PARAMS_MALLOC* FP_snd_pcm_hw_params_malloc;
SND_PCM_HW_PARAMS_ANY* FP_snd_pcm_hw_params_any;
SND_PCM_HW_PARAMS_SET_ACCESS* FP_snd_pcm_hw_params_set_access;
SND_PCM_HW_PARAMS_SET_FORMAT* FP_snd_pcm_hw_params_set_format;
SND_PCM_HW_PARAMS_SET_CHANNELS* FP_snd_pcm_hw_params_set_channels;
SND_PCM_HW_PARAMS_SET_RATE_NEAR* FP_snd_pcm_hw_params_set_rate_near;
SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR* FP_snd_pcm_hw_params_set_period_size_near;
SND_PCM_HW_PARAMS* FP_snd_pcm_hw_params;
SND_PCM_HW_PARAMS_GET_CHANNELS* FP_snd_pcm_hw_params_get_channels;
SND_PCM_HW_PARAMS_GET_RATE* FP_snd_pcm_hw_params_get_rate;
SND_PCM_HW_PARAMS_GET_FORMAT* FP_snd_pcm_hw_params_get_format;
SND_PCM_HW_PARAMS_GET_ACCESS* FP_snd_pcm_hw_params_get_access;
SND_PCM_HW_PARAMS_GET_PERIOD_TIME* FP_snd_pcm_hw_params_get_period_time;
SND_PCM_HW_PARAMS_GET_PERIOD_SIZE* FP_snd_pcm_hw_params_get_period_size;
SND_PCM_AVAIL_DELAY* FP_snd_pcm_avail_delay;
SND_PCM_WRITEI* FP_snd_pcm_writei;
SND_PCM_RECOVER* FP_snd_pcm_recover;
SND_PCM_PREPARE* FP_snd_pcm_prepare;
SND_PCM_CLOSE* FP_snd_pcm_close;
SND_PCM_DRAIN* FP_snd_pcm_drain;

#endif // PLATFORM_LINUX

bool InitializeAudio(Platform &platform)
{
	LOG(Info, "Sound system initialization:\n");

	if ( platform.RenderAudioCallback == nullptr )
	{
		LOG(Info, "- RenderAudioCallback not provided, sound system not required\n");
		return true;
	}

	Audio &audio = platform.audio;
	const Window &window = platform.window;

	const u16 gameUpdateHz = 30;

	// Audio configuration
	audio.channelCount = 2;
	audio.bytesPerSample = 2; // 4 in HH
	audio.samplesPerSecond = 48000; // per channel
	audio.bufferSize = audio.channelCount * audio.samplesPerSecond * audio.bytesPerSample;
	audio.latencyFrameCount = 3;
	audio.latencySampleCount = audio.latencyFrameCount * audio.samplesPerSecond / gameUpdateHz;
	audio.safetyBytes = (audio.samplesPerSecond * audio.bytesPerSample * audio.channelCount)/audio.latencyFrameCount;

	// Allocate buffer to output samples from the engine
	audio.outputSamples = (i16*)AllocateVirtualMemory(audio.bufferSize);

#if PLATFORM_WINDOWS
	audio.library = OpenLibrary("dsound.dll");

	if (audio.library)
	{
		LOG(Info, "- Loaded dsound.dll successfully\n");

		FP_DirectSoundCreate* CreateAudioDevice = (FP_DirectSoundCreate*)LoadSymbol(audio.library, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (CreateAudioDevice && SUCCEEDED(CreateAudioDevice(0, &directSound, 0)))
		{
			LOG(Info, "- Audio device created successfully\n");

			if (SUCCEEDED(directSound->SetCooperativeLevel(window.hWnd, DSSCL_PRIORITY)))
			{
				// We create a primary buffer just to set the wanted format and avoid the API resample sounds to whatever rate it uses by default
				DSBUFFERDESC primaryBufferDesc = {};
				primaryBufferDesc.dwSize = sizeof(primaryBufferDesc);
				primaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDesc, &primaryBuffer, 0)))
				{
					LOG(Info, "- Primary buffer created successfully\n");

					WAVEFORMATEX waveFormat = {};
					waveFormat.wFormatTag = WAVE_FORMAT_PCM;
					waveFormat.nChannels = audio.channelCount;
					waveFormat.nSamplesPerSec = audio.samplesPerSecond;
					waveFormat.nBlockAlign = audio.channelCount * audio.bytesPerSample;
					waveFormat.nAvgBytesPerSec = audio.samplesPerSecond * waveFormat.nBlockAlign;
					waveFormat.wBitsPerSample = audio.bytesPerSample * 8;
					waveFormat.cbSize = 0;
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{
						LOG(Info, "- Primary buffer format set successfully\n");

						// After setting the primary buffer format, we create the secondary buffer where we will be actually writing to
						DSBUFFERDESC secondaryBufferDesc = {};
						secondaryBufferDesc.dwSize = sizeof(secondaryBufferDesc);
						secondaryBufferDesc.dwFlags = 0;
						secondaryBufferDesc.dwBufferBytes = audio.bufferSize;
						secondaryBufferDesc.lpwfxFormat = &waveFormat;
						LPDIRECTSOUNDBUFFER secondaryBuffer;
						if (SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDesc, &secondaryBuffer, 0)))
						{
							LOG(Info, "- Secondary buffer created successfully\n");

							audio.buffer = secondaryBuffer;

							MemSet(audio.outputSamples, audio.bufferSize, 0);
							Win32FillAudioBuffer(sPlatform->audio, 0, sPlatform->audio.bufferSize, audio.outputSamples);

							if (SUCCEEDED(audio.buffer->Play(0, 0, DSBPLAY_LOOPING)))
							{
								LOG(Info, "- Secondary buffer is playing...\n");
								audio.initialized = true;
								audio.isPlaying = true;
								audio.soundIsValid = false;
							}
							else
							{
								LOG(Error, "- Error playing secondaryBuffer.\n");
							}
						}
						else
						{
							LOG(Error, "- Error calling CreateSoundBuffer for secondaryBuffer\n");
						}
					}
					else
					{
						LOG(Error, "- Error setting primary buffer format\n");
					}
				}
				else
				{
					LOG(Error, "- Error calling CreateSoundBuffer for primaryBuffer\n");
				}
			}
			else
			{
				LOG(Error, "- Error setting DirectSound priority cooperative level\n");
			}
		}
		else
		{
			LOG(Error, "- Error loading DirectSoundCreate symbol\n");
		}
	}
	else
	{
		LOG(Error, "- Error loading dsound.dll\n");
	}

	return audio.initialized;

#elif PLATFORM_LINUX

	audio.library = OpenLibrary("libasound.so");

	if (audio.library)
	{
		DynamicLibrary alsa = audio.library;

		// Load functions
		FP_snd_strerror = (SND_STRERROR*) LoadSymbol(alsa, "snd_strerror");
		FP_snd_pcm_open = (SND_PCM_OPEN*) LoadSymbol(alsa, "snd_pcm_open");
		FP_snd_pcm_hw_params_malloc = (SND_PCM_HW_PARAMS_MALLOC*) LoadSymbol(alsa, "snd_pcm_hw_params_malloc");
		FP_snd_pcm_hw_params_any = (SND_PCM_HW_PARAMS_ANY*) LoadSymbol(alsa, "snd_pcm_hw_params_any");
		FP_snd_pcm_hw_params_set_access = (SND_PCM_HW_PARAMS_SET_ACCESS*) LoadSymbol(alsa, "snd_pcm_hw_params_set_access");
		FP_snd_pcm_hw_params_set_format = (SND_PCM_HW_PARAMS_SET_FORMAT*) LoadSymbol(alsa, "snd_pcm_hw_params_set_format");
		FP_snd_pcm_hw_params_set_channels = (SND_PCM_HW_PARAMS_SET_CHANNELS*) LoadSymbol(alsa, "snd_pcm_hw_params_set_channels");
		FP_snd_pcm_hw_params_set_rate_near = (SND_PCM_HW_PARAMS_SET_RATE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_rate_near");
		FP_snd_pcm_hw_params_set_period_size_near = (SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_period_size_near");
		FP_snd_pcm_hw_params = (SND_PCM_HW_PARAMS*) LoadSymbol(alsa, "snd_pcm_hw_params");
		FP_snd_pcm_hw_params_get_channels = (SND_PCM_HW_PARAMS_GET_CHANNELS*) LoadSymbol(alsa, "snd_pcm_hw_params_get_channels");
		FP_snd_pcm_hw_params_get_rate = (SND_PCM_HW_PARAMS_GET_RATE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_rate");
		FP_snd_pcm_hw_params_get_format = (SND_PCM_HW_PARAMS_GET_FORMAT*) LoadSymbol(alsa, "snd_pcm_hw_params_get_format");
		FP_snd_pcm_hw_params_get_access = (SND_PCM_HW_PARAMS_GET_ACCESS*) LoadSymbol(alsa, "snd_pcm_hw_params_get_access");
		FP_snd_pcm_hw_params_get_period_time = (SND_PCM_HW_PARAMS_GET_PERIOD_TIME*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_time");
		FP_snd_pcm_hw_params_get_period_size = (SND_PCM_HW_PARAMS_GET_PERIOD_SIZE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_size");
		FP_snd_pcm_avail_delay = (SND_PCM_AVAIL_DELAY*) LoadSymbol(alsa, "snd_pcm_avail_delay");
		FP_snd_pcm_writei = (SND_PCM_WRITEI*) LoadSymbol(alsa, "snd_pcm_writei");
		FP_snd_pcm_recover = (SND_PCM_RECOVER*) LoadSymbol(alsa, "snd_pcm_recover");
		FP_snd_pcm_prepare = (SND_PCM_PREPARE*) LoadSymbol(alsa, "snd_pcm_prepare");
		FP_snd_pcm_close = (SND_PCM_CLOSE*) LoadSymbol(alsa, "snd_pcm_close");
		FP_snd_pcm_drain = (SND_PCM_DRAIN*) LoadSymbol(alsa, "snd_pcm_drain");

		// Open PCM device
		int res = FP_snd_pcm_open(&audio.pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
		if (res == 0)
		{
			int dir = 0; // direction of approximate values
			unsigned int sampleRate = 48000; // frames/second (CD quality)
			unsigned int channelCount = 2;
			unsigned int bytesPerSample = 2;
			snd_pcm_uframes_t frames = 32; // period size of 32 frames?

			// Allocate and configure hardware parameters
			snd_pcm_hw_params_t *params;
			FP_snd_pcm_hw_params_malloc(&params);
			FP_snd_pcm_hw_params_any(audio.pcm, params); // default values
			FP_snd_pcm_hw_params_set_channels(audio.pcm, params, channelCount);
			FP_snd_pcm_hw_params_set_rate_near(audio.pcm, params, &sampleRate, &dir);
			FP_snd_pcm_hw_params_set_format(audio.pcm, params, SND_PCM_FORMAT_S16_LE); // 16 bit little endian
			FP_snd_pcm_hw_params_set_access(audio.pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
			FP_snd_pcm_hw_params_set_period_size_near(audio.pcm, params, &frames, &dir);

			// Write the parameters to the driver
			res = FP_snd_pcm_hw_params(audio.pcm, params);
			if (res == 0)
			{
				LOG(Info, "- PCM is playing...\n");
				audio.initialized = true;
				audio.isPlaying = true;
				audio.soundIsValid = false;
			}
			else
			{
				LOG(Error, "- Error setting PCM HW parameters: %s\n", FP_snd_strerror(res));
			}

			unsigned int finalSampleRate = 0;
			FP_snd_pcm_hw_params_get_rate(params, &finalSampleRate, &dir);
		}
		else
		{
			LOG(Error, "- Error opening PCM device: %s\n", FP_snd_strerror(res));
			return 1;
		}
	}
	else
	{
		LOG(Error, "- Error loading libasound.so\n");
	}

	return audio.initialized;
#else
	return true;
#endif // PLATFORM_WINDOWS
}

#if PLATFORM_WINDOWS
void Win32FillAudioBuffer(Audio &audio, DWORD writeOffset, DWORD writeSize, const i16 *audioSamples)
{
	ASSERT(writeSize <= audio.bufferSize);

	void *region1;
	DWORD region1Size;
	void *region2;
	DWORD region2Size;

	if (audio.buffer->Lock(writeOffset, writeSize, &region1, &region1Size, &region2, &region2Size, 0) == DS_OK)
	{
		const i16 *srcSample = audioSamples;

		i16 *dstSample = (i16*)region1;
		const u32 region1SampleCount = region1Size / (audio.bytesPerSample * audio.channelCount);
		for (u32 i = 0; i < region1SampleCount; ++i)
		{
			*dstSample++ = *srcSample++;
			*dstSample++ = *srcSample++;
			audio.runningSampleIndex++;
		}

		dstSample = (i16*)region2;
		const u32 region2SampleCount = region2Size / (audio.bytesPerSample * audio.channelCount);
		for (u32 i = 0; i < region2SampleCount; ++i)
		{
			*dstSample++ = *srcSample++;
			*dstSample++ = *srcSample++;
			audio.runningSampleIndex++;
		}

		audio.buffer->Unlock(region1, region1Size, region2, region2Size);
	}
	else
	{
		LOG(Warning, "Failed to Lock sound buffer.\n");
		audio.soundIsValid = false;
	}
}
#endif // PLATFORM_WINDOWS

void UpdateAudio(Platform &platform, float secondsSinceFrameBegin)
{
	Audio &audio = platform.audio;

#if PLATFORM_WINDOWS
	DWORD playCursor;
	DWORD writeCursor;
	if (audio.buffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
	{
		// Audio just started playing
		if (!audio.soundIsValid) {
			audio.runningSampleIndex = writeCursor / (audio.bytesPerSample * audio.channelCount);
			audio.soundIsValid = true;
		}

		const DWORD byteToLock = (audio.runningSampleIndex * audio.bytesPerSample * audio.channelCount) % audio.bufferSize;

		const u32 gameUpdateHz = 30;
		const f32 targetSecondsPerFrame = 1.0f / (f32)gameUpdateHz;

		const DWORD expectedBytesPerFrame = (audio.samplesPerSecond * audio.channelCount * audio.bytesPerSample) / gameUpdateHz;
		const f32 secondsLeftUntilFlip = targetSecondsPerFrame - secondsSinceFrameBegin;
		const DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip/targetSecondsPerFrame)*(f32)expectedBytesPerFrame);
		const DWORD expectedFrameBoundaryByte = playCursor + expectedBytesPerFrame;

		DWORD safeWriteCursor = writeCursor;
		if (safeWriteCursor < playCursor) {
			safeWriteCursor += audio.bufferSize;
		}
		ASSERT(safeWriteCursor >= playCursor);

		const bool audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;

		DWORD targetCursor = 0;
		if (audioCardIsLowLatency) {
			targetCursor = expectedFrameBoundaryByte + expectedBytesPerFrame;
		} else {
			targetCursor = writeCursor + expectedBytesPerFrame + audio.safetyBytes;
		}
		targetCursor = targetCursor % audio.bufferSize;

		DWORD bytesToWrite = 0;
		if (byteToLock > targetCursor) {
			bytesToWrite = targetCursor + (audio.bufferSize - byteToLock);
		} else {
			bytesToWrite = targetCursor - byteToLock;
		}

		#if 0 // Debug code to print audio latency
		// Latency calculation
		DWORD unwrappedWriteCursor = writeCursor;
		if (writeCursor < playCursor) {
			unwrappedWriteCursor += audio.bufferSize;
		}
		const DWORD latencySize = unwrappedWriteCursor - playCursor;
		const DWORD latencySamples = latencySize / ( audio.bytesPerSample * audio.channelCount );
		const f32 latencySeconds = (f32)latencySamples / (f32)audio.samplesPerSecond;
		LOG(Debug, "latency: %u bytes (%fs)\n", latencySize, latencySeconds);
		#endif

		SoundBuffer soundBuffer = {};
		soundBuffer.samplesPerSecond = audio.samplesPerSecond;
		soundBuffer.sampleCount = bytesToWrite / (audio.bytesPerSample * audio.channelCount);
		soundBuffer.samples = audio.outputSamples;
		platform.RenderAudioCallback(platform, soundBuffer);

		Win32FillAudioBuffer(audio, byteToLock, bytesToWrite, soundBuffer.samples);
	}
	else
	{
		LOG(Warning, "Failed to GetCurrentPosition for sound buffer.\n");
	}
#elif PLATFORM_LINUX

	for (u32 i = 0; i < 2; ++i)
	{
		snd_pcm_sframes_t availableFrames;
		snd_pcm_sframes_t delayFrames;
		int res = FP_snd_pcm_avail_delay(audio.pcm, &availableFrames, &delayFrames);
		//LOG(Debug, "avail %u / delay %u\n", availableFrames, delayFrames);

		if ( res == 0 )
		{
			// TODO(jesus): Underruns seem to be fixed increasing this time window (but we add latency to reproduce new sounds)
			const float time = 2.0f / 30.0f; // Two times what's needed to render two game frames
			const snd_pcm_uframes_t maxFramesToRender = audio.samplesPerSecond * time;

			snd_pcm_uframes_t framesToRender = maxFramesToRender - delayFrames;

			framesToRender = framesToRender < availableFrames ? framesToRender : availableFrames;

			if ( framesToRender > 0 )
			{
				SoundBuffer soundBuffer = {};
				soundBuffer.samplesPerSecond = audio.samplesPerSecond;
				soundBuffer.sampleCount = framesToRender;
				soundBuffer.samples = audio.outputSamples;
				platform.RenderAudioCallback(platform, soundBuffer);

				res = FP_snd_pcm_writei(audio.pcm, soundBuffer.samples, framesToRender);

				if ( res < 0 )
				{
					LOG(Error, "Error calling snd_pcm_writei: %s\n", FP_snd_strerror(res));
				}
			}
		}
		else
		{
			LOG(Error, "Error calling snd_pcm_avail_delay: %s\n", FP_snd_strerror(res));
		}

		// Error recovery
		if (res == -EPIPE || res == -ESTRPIPE || res == -EINTR) {
			FP_snd_pcm_recover(audio.pcm, res, 0);
			continue;
		}

		// All good or unrecoverable error (no need to try a second time)
		break;
	}

#endif
}

struct ThreadInfo
{
	u32 globalIndex;
};

#define WORK_QUEUE_CALLBACK(name) void name(const ThreadInfo &threadInfo, void *data)
typedef WORK_QUEUE_CALLBACK(WorkQueueCallback);

#if PLATFORM_WINDOWS

#define THREAD_FUNCTION(name) DWORD WINAPI name(LPVOID arguments)

#define FullWriteBarrier() _WriteBarrier(); _mm_sfence()
#define FullReadBarrier() _ReadBarrier()

typedef HANDLE Semaphore;

static bool CreateSemaphore( Semaphore &semaphore, u32 iniCount, u32 maxCount )
{
	bool success = true;
	semaphore = CreateSemaphoreEx(0, iniCount, maxCount, 0, 0, SEMAPHORE_ALL_ACCESS);
	if ( semaphore == NULL ) {
		Win32ReportError("CreateSemaphoreEx");
		success = false;
	}
	return success;
}

static bool SignalSemaphore( Semaphore semaphore )
{
	bool success = true;
	if ( !ReleaseSemaphore(semaphore, 1, 0) ) {
		Win32ReportError("ReleaseSemaphore");
		success = false;
	}
	return success;
}

static bool WaitSemaphore( Semaphore semaphore )
{
	bool success = true;
	if ( WaitForSingleObjectEx(semaphore, INFINITE, FALSE) == WAIT_FAILED ) {
		Win32ReportError("WaitForSingleObjectEx");
		success = false;
	}
	return success;
}

static bool CreateDetachedThread( THREAD_FUNCTION(threadFunc), const ThreadInfo &threadInfo )
{
	bool success = true;
	DWORD threadId;
	HANDLE threadHandle = CreateThread(0, 0, threadFunc, (LPVOID)&threadInfo, 0, &threadId);
	if ( threadHandle == NULL ) {
		Win32ReportError("CreateThread");
		success = false;
	} else {
		CloseHandle(threadHandle);
	}
	return success;
}

#elif PLATFORM_LINUX || PLATFORM_ANDROID

#define THREAD_FUNCTION(name) void* name(void *arguments)

#define FullWriteBarrier() __sync_synchronize()
#define FullReadBarrier() __sync_synchronize()

typedef sem_t Semaphore;

static bool CreateSemaphore( Semaphore &semaphore, u32 iniCount, u32 maxCount )
{
	bool success = true;
	if ( sem_init(&semaphore, 0, iniCount) != 0 ) { // TODO(jediaz): use sem_init_np to set maxCount
		LinuxReportError("sem_init");
		success = false;
	}
	return success;
}

static bool SignalSemaphore( Semaphore &semaphore )
{
	bool success = true;
	if ( sem_post(&semaphore) != 0 ) {
		LinuxReportError("sem_post");
		success = false;
	}
	return success;
}

static bool WaitSemaphore( Semaphore &semaphore )
{
	bool success = true;
	if ( sem_wait(&semaphore) != 0 ) {
		LinuxReportError("sem_wait");
		success = false;
	}
	return success;
}

static bool CreateDetachedThread( THREAD_FUNCTION(threadFunc), ThreadInfo &threadInfo )
{
	bool success = false;
	pthread_t threadHandle;
	if ( pthread_create(&threadHandle, nullptr, threadFunc, &threadInfo) == 0 ) {
		if ( pthread_detach(threadHandle) == 0 ) {
			success = true;
		} else {
			LinuxReportError("pthread_detach");
		}
	} else {
		LinuxReportError("pthread_create");
	}
	return success;
}

#else
#error "Missing implementation"
#endif

struct WorkQueueEntry
{
	WorkQueueCallback *callback;
	void *data;
};

struct WorkQueue
{
	Semaphore semaphore;
	volatile_u64 head;
	volatile_u64 tail;

	WorkQueueEntry entries[128] = {};
};

static WorkQueue workQueue;

static void WorkQueuePush(WorkQueueEntry entry)
{
	ASSERT(workQueue.head - workQueue.tail < ARRAY_COUNT(workQueue.entries));
	ASSERT(entry.callback);
	ASSERT(entry.data);

	const u32 index = workQueue.head % ARRAY_COUNT(workQueue.entries);
	workQueue.entries[index] = entry;

	FullWriteBarrier();

	workQueue.head++;

	SignalSemaphore(workQueue.semaphore);
}

static bool WorkQueueEmpty()
{
	const bool empty = workQueue.head == workQueue.tail;
	return empty;
}

static bool WorkQueueProcess(const ThreadInfo &threadInfo)
{
	bool thereIsPendingWork = false;

	const u64 tail = workQueue.tail;

	if (tail < workQueue.head)
	{
		if ( AtomicSwap(&workQueue.tail, tail, tail+1) )
		{
			FullReadBarrier();

			const u32 workIndex = tail % ARRAY_COUNT(workQueue.entries);
			WorkQueueEntry &entry = workQueue.entries[workIndex];
			entry.callback(threadInfo, entry.data);
		}

		thereIsPendingWork = true;
	}

	return thereIsPendingWork;
}

static THREAD_FUNCTION(WorkQueueThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;

	while (1)
	{
		if ( !WorkQueueProcess(*threadInfo) )
		{
			WaitSemaphore(workQueue.semaphore);
		}
	}

	return 0;
}

static bool WorkQueueInitialize()
{
	static ThreadInfo threadInfos[16];
	constexpr u32 threadCount = ARRAY_COUNT(threadInfos);

	const u32 iniCount = 0;
	const u32 maxCount = threadCount;
	if ( !CreateSemaphore( workQueue.semaphore, iniCount, maxCount ) )
	{
		return false;
	}

	workQueue.head = 0;
	workQueue.tail = 0;

	for (u32 i = 0; i < threadCount; ++i)
	{
		ThreadInfo &threadInfo = threadInfos[i];
		threadInfo.globalIndex = i;
		if ( !CreateDetachedThread(WorkQueueThread, threadInfo) )
		{
			return false;
		}
	}

	return true;
}

#if 0 // Threading test code
static void PrintString(const ThreadInfo &threadInfo, void *data)
{
	const char *work = (const char *)data;
	LOG(Debug, "Thread %u work: %s\n", threadInfo.globalIndex, work);
}

static void WorkQueuePushString(const char *str)
{
	WorkQueueEntry entry = {
		.callback = PrintString,
		.data = (void*)str
	};
	WorkQueuePush(entry);
}
#endif

bool InitializeThreads(Platform &platform)
{
	if ( !WorkQueueInitialize() )
	{
		return false;
	}

#if 0 // Threading test code
	WorkQueuePushString("A0");
	WorkQueuePushString("A1");
	WorkQueuePushString("A2");
	WorkQueuePushString("A3");
	WorkQueuePushString("A4");
	WorkQueuePushString("A5");
	WorkQueuePushString("A6");
	WorkQueuePushString("A7");
	WorkQueuePushString("A8");
	WorkQueuePushString("A9");
	WorkQueuePushString("B0");
	WorkQueuePushString("B1");
	WorkQueuePushString("B2");
	WorkQueuePushString("B3");
	WorkQueuePushString("B4");
	WorkQueuePushString("B5");
	WorkQueuePushString("B6");
	WorkQueuePushString("B7");
	WorkQueuePushString("B8");
	WorkQueuePushString("B9");
	WorkQueuePushString("C0");
	WorkQueuePushString("C1");
	WorkQueuePushString("C2");
	WorkQueuePushString("C3");
	WorkQueuePushString("C4");
	WorkQueuePushString("C5");
	WorkQueuePushString("C6");
	WorkQueuePushString("C7");
	WorkQueuePushString("C8");
	WorkQueuePushString("C9");

	ThreadInfo threadInfo = { 16 };
	while (!WorkQueueEmpty()) {
		WorkQueueProcess(threadInfo);
	}
#endif

	return true;
}

bool PlatformInitialize(Platform &platform, int argc, char **argv)
{
	ASSERT( platform.globalMemorySize > 0 );
	ASSERT( platform.frameMemorySize > 0 );
	ASSERT( platform.InitCallback );
	ASSERT( platform.UpdateCallback );
	ASSERT( platform.CleanupCallback );
	ASSERT( platform.WindowInitCallback );
	ASSERT( platform.WindowCleanupCallback );

	byte *globalMemory = (byte*)AllocateVirtualMemory(platform.globalMemorySize);
	platform.globalArena = MakeArena(globalMemory, platform.globalMemorySize);

	byte *frameMemory = (byte*)AllocateVirtualMemory(platform.frameMemorySize);
	platform.frameArena = MakeArena(frameMemory, platform.frameMemorySize);

	byte *stringMemory = (byte*)AllocateVirtualMemory(platform.stringMemorySize);
	platform.stringArena = MakeArena(stringMemory, platform.stringMemorySize);
	platform.stringInterning = StringInterningCreate(&platform.stringArena);

	byte *dataMemory = (byte*)AllocateVirtualMemory(platform.dataMemorySize);
	platform.dataArena = MakeArena(dataMemory, platform.dataMemorySize);

#if PLATFORM_ANDROID
	ASSERT( platform.androidApp );
	platform.androidApp = platform.androidApp;
	platform.androidApp->onAppCmd = AndroidHandleAppCommand;
	platform.androidApp->onInputEvent = AndroidHandleInputEvent;
	platform.androidApp->userData = &platform;
#endif // PLATFORM_ANDROID

	InitializeDirectories(platform, argc, argv);

	sPlatform = &platform;

	return true;
}

bool PlatformRun(Platform &platform)
{
	if ( !InitializeWindow(platform.window) )
	{
		return false;
	}

	if ( InitializeGamepad(platform) )
	{
		// Do nothing
	}

	if ( !InitializeAudio(platform) )
	{
		return false;
	}

	if ( !InitializeThreads(platform) )
	{
		return false;
	}

	if ( !platform.InitCallback(platform) )
	{
#if PLATFORM_ANDROID
		ANativeActivity_finish(platform.androidApp->activity);
#endif // PLATFORM_ANDROID
		return false;
	}

	Clock lastFrameClock = GetClock();

	bool windowInitialized = false;

	const Clock firstFrameClock = GetClock();

	while ( 1 )
	{
		ResetArena(platform.frameArena);

		const Clock currentFrameBeginClock = GetClock();
		platform.deltaSeconds = GetSecondsElapsed(lastFrameClock, currentFrameBeginClock);
		platform.totalSeconds = GetSecondsElapsed(firstFrameClock, currentFrameBeginClock);
		lastFrameClock = currentFrameBeginClock;

		PlatformUpdateEventLoop(platform);

		if ( platform.window.flags & WindowFlags_Exit )
		{
			break;
		}
		if ( platform.window.flags & WindowFlags_WasCreated )
		{
			platform.WindowInitCallback(platform);
			windowInitialized = true;
		}
		if ( platform.window.flags & WindowFlags_WillDestroy )
		{
			platform.WindowCleanupCallback(platform);
			CleanupWindow(platform.window);
			windowInitialized = false;
		}

		if ( windowInitialized )
		{
			platform.UpdateCallback(platform);
		}

		platform.window.flags = 0;

		UpdateGamepad(platform);

		if ( platform.audio.isPlaying )
		{
			const f32 secondsSinceFrameBegin = GetSecondsElapsed(currentFrameBeginClock, GetClock());
			UpdateAudio(platform, secondsSinceFrameBegin);
		}

		// TODO: Potentially use a separate RenderCallback here instead of using UpdateCallback for that.
	}

	platform.CleanupCallback(platform);

	PrintArenaUsage(platform.globalArena);

	// TODO: Cleanup window and audio

	return false;
}

#endif // #if defined(TOOLS_PLATFORM)

#endif // #ifndef TOOLS_H

