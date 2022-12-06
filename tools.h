#include <assert.h> // assert
#include <stdlib.h> // atof
#include <stdio.h>  // printf, FILE, etc

////////////////////////////////////////////////////////////////////////////////////////////////////
// Base types

typedef char i8;
typedef short int i16;
typedef int i32;
typedef int i32;
typedef long long int i64;
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef	unsigned char byte;

#define KB(x) (1024ul * x)
#define MB(x) (1024ul * KB(x))
#define GB(x) (1024ul * MB(x))
#define TB(x) (1024ul * GB(x))

#define ASSERT(expression) assert(expression)
#define ERROR(message) ASSERT( 0 && message )
#define INVALID_CODE_PATH() ERROR("Invalid code path")
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))



////////////////////////////////////////////////////////////////////////////////////////////////////
// Strings

struct String
{
	const char* str;
	u32 size;
};

String MakeString(const char *str, u32 size)
{
	String string = { str, size };
	return string;
}

void StrCopy(char *dst, const String& src_string)
{
	u32 size = src_string.size;
	const char *src = src_string.str;
	while (size-- > 0) *dst++ = *src++;
	dst[src_string.size] = '\0';
}

void StrCopy(char *dst, const char *src)
{
	while (*src) *dst++ = *src++;
	*dst = 0;
}

void StrCat(char *dst, const char *src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
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

f32 StrToFloat(const String &s)
{
	char buf[256] = {};
	ASSERT(s.size + 1 < ARRAY_COUNT(buf));
	StrCopy(buf, s);
	f32 number = atof(buf);
	return number;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory

#if __APPLE__ || __linux__

#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

void* AllocateVirtualMemory(u32 size)
{
	void* baseAddress = 0;
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	int fd = -1;
	off_t offset = 0;
	void *allocatedMemory = mmap(baseAddress, size, prot, flags, fd, offset);
	ASSERT( allocatedMemory != MAP_FAILED && "Failed to allocate memory." );
	return allocatedMemory;
}

#else
#	error "Missing platform implementation"
#endif

void MemSet(void *ptr, u32 size)
{
	byte *bytePtr = (byte*)ptr;
	while (size-- > 0) *bytePtr++ = 0;
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

byte* PushSize(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "PushSize of bounds of the memory arena.");
	byte* head = arena.base + arena.used;
	arena.used += size;
	return head;
}

void ResetArena(Arena &arena)
{
	// This tells the OS we don't need these pages
	//madvise(arena.base, arena.size, MADV_DONTNEED);
	arena.used = 0;
}

void PrintArenaUsage(Arena &arena)
{
	printf("Memory Arena Usage:\n");
	printf("- size: %u B / %u kB\n", arena.size, arena.size/1024);
	printf("- used: %u B / %u kB\n", arena.used, arena.used/1024);
}

#define ZeroStruct( pointer ) MemSet(pointer, sizeof(*pointer) )
#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))
#define PushArray( arena, type, count ) (type*)PushSize(arena, sizeof(type) * count)



////////////////////////////////////////////////////////////////////////////////////////////////////
// Files

u32 GetFileSize(const char *filename)
{
	u32 size = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fclose(f);
	}
	return size;
}

u32 ReadEntireFile(const char *filename, char *bytes, u32 bytesSize)
{
	u32 bytesRead = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		bytesRead = fread(bytes, sizeof(unsigned char), bytesSize, f);
		ASSERT(bytesRead <= bytesSize);
		bytes[bytesRead] = 0;
		fclose(f);
	}
	return bytesRead;
}



