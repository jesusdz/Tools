#ifndef HANDLE_MANAGER_H
#define HANDLE_MANAGER_H

struct Handle
{
	u16 gen;
	u16 idx;
};

constexpr Handle InvalidHandle = {};

#define CHECK_HANDLE(name) bool name(Handle, const char *, void *)
typedef CHECK_HANDLE(CheckHandle);

struct HandleFinder
{
	CheckHandle *checkHandle;
	const char *name;
	void *data;
};

struct HandleManager
{
	Handle *handles;
	u16 *indices; // First all used indices, then all free indices
	u16 handleCount;
	u16 handleLimit;
	bool initialized;
};

inline bool operator==(Handle a, Handle b)
{
	const bool equal = ( a.idx == b.idx ) && ( a.gen == b.gen );
	return equal;
}

void Initialize(HandleManager &manager, Arena &arena, u16 handleLimit)
{
	manager.handles = PushArray(arena, Handle, handleLimit);
	manager.indices = PushArray(arena, u16, handleLimit);

	for (u32 i = 0; i < handleLimit; ++i)
	{
		manager.indices[i] = i;
		manager.handles[i].idx = i;
		manager.handles[i].gen = 1;
	}

	manager.handleCount = 0;
	manager.handleLimit = handleLimit;
	manager.initialized = true;
}

bool IsValidHandle(const HandleManager &manager, Handle handle)
{
	Handle storedHandle = manager.handles[handle.idx];
	const bool valid = handle == storedHandle;
	return valid;
}

Handle NewHandle(HandleManager &manager)
{
	ASSERT(manager.initialized);
	ASSERT(manager.handleCount < manager.handleLimit);
	u16 index = manager.indices[manager.handleCount++];
	Handle &handle = manager.handles[index];
	ASSERT(handle.idx == index);
	return handle;
}

void FreeHandle(HandleManager &manager, Handle handle)
{
	ASSERT(IsValidHandle(manager, handle));
	manager.handles[handle.idx].gen++;

	ASSERT(manager.handleCount > 0);
	manager.handleCount--;
	bool found = false;
	for (u16 i = 0; i < manager.handleCount; ++i) {
		found = found || handle.idx == manager.indices[i];
		if (found) { // compact indices from found index onwards
			manager.indices[i] = manager.indices[i+1];
		}
	}
	// Insert the freed index at the end
	manager.indices[manager.handleCount] = handle.idx;
}

Handle GetHandleAt(const HandleManager &manager, u16 index)
{
	u16 handleIndex = manager.indices[index];
	const Handle handle = manager.handles[handleIndex];
	return handle;
}

Handle FindHandle(const HandleManager &manager, const HandleFinder &finder)
{
	for (u16 i = 0; i < manager.handleCount; ++i)
	{
		u16 index = manager.indices[i];
		const Handle handle = manager.handles[index];
		if ( finder.checkHandle(handle, finder.name, finder.data) )
		{
			return handle;
		}
	}
	LOG(Warning, "Could not find handle <%s>.\n", finder.name);
	return InvalidHandle;
}

#endif // HANDLE_MANAGER_H

