#ifndef ILU_PROFILE
#define ILU_PROFILE

#ifndef USE_PROFILE
#define USE_PROFILE 1
#endif

#if USE_PROFILE

constexpr u32 MAX_PROFILE_EVENTS = 1024;
constexpr u32 MAX_PROFILE_NODES = 512;
constexpr u32 MAX_PROFILE_STACKED_EVENTS = 16;
constexpr u32 MAX_PROFILE_NAMES = 256;
constexpr u32 MAX_PROFILE_NAME_SLOTS = 512;
constexpr u32 MAX_PROFILE_STRING_CHARS = KB(8);
constexpr u32 MAX_PROFILE_FRAMES = 64;
constexpr u32 MAX_PROFILE_THREADS = 8;
constexpr u16 PROFILE_NODE_NONE = 0xFFFF;
constexpr u32 PROFILE_THREAD_NONE = 0xFFFFFFFF;

// Balanced begin/end pairs produce one node per two events
CT_ASSERT(MAX_PROFILE_NODES >= MAX_PROFILE_EVENTS / 2);
CT_ASSERT(MAX_PROFILE_NODES < PROFILE_NODE_NONE); // Node indices must fit in ProfileNode::parentIndex
CT_ASSERT((MAX_PROFILE_NAME_SLOTS & (MAX_PROFILE_NAME_SLOTS - 1)) == 0); // Is power of 2
CT_ASSERT(MAX_PROFILE_NAME_SLOTS >= 2 * MAX_PROFILE_NAMES);

typedef u64 ProfileTime;

enum ProfileEventType : u16
{
	ProfileEventType_Begin,
	ProfileEventType_End,
};

struct ProfileEvent
{
	ProfileTime time;
	ProfileEventType type;
	u16 nameId;
};

struct ProfileNode
{
	ProfileTime begin;
	ProfileTime end;
	u16 nameId;
	u16 parentIndex; // PROFILE_NODE_NONE if root
};

struct ProfileFrame
{
	u32 nodeCount;
	u32 droppedEventCount;
	ProfileNode nodes[MAX_PROFILE_NODES];
	ProfileTime begin;
	ProfileTime end;
	u64 index;
};

struct ProfileThread
{
	const char *name;
	ProfileEvent events[MAX_PROFILE_EVENTS];
	u32 eventCount;
	u32 droppedEventCount;
	ProfileTime frameBegin;
	// u32 threadId; // optional, for the UI later

	// History of frame data
	ProfileFrame frames[MAX_PROFILE_FRAMES]; // Ring buffer
	u64 frameIndex;
};

struct Profile
{
	// Per thread event recorders
	ProfileThread threads[MAX_PROFILE_THREADS];
	volatile_i64 threadClaimCount;
	u32 frameThreadIndex;

	// Ad-hoc string interning for names
	Mutex nameMutex;
	const char *names[MAX_PROFILE_NAMES];
	u32 nameCount;
	u16 nameSlots[MAX_PROFILE_NAME_SLOTS];
	char stringArena[MAX_PROFILE_STRING_CHARS];
	u32 stringArenaSize;
};

void ProfileInit();
u16 ProfileRegisterName(const char *name);
const char *ProfileGetName(u16 nameId);
void ProfileFlush();
void ProfileNewFrame();
void ProfileBeginEvent(u16 nameId);
void ProfileEndEvent(u16 nameId);

void ProfileRegisterThread(const char *name);
const char *ProfileGetThreadName(u32 threadIndex);
u32 ProfileGetThreadCount();
u32 ProfileGetThreadFrameCount(u32 threadIndex);
ProfileFrame ProfileGetThreadFrame(u32 threadIndex, u32 age);
// Convenience functions
u32 ProfileGetFrameCount();
ProfileFrame ProfileGetFrame(u32 age);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile blocks

struct ProfileBlock
{
	u16 id;
	ProfileBlock(u16 nameId) : id(nameId) { ProfileBeginEvent(nameId); }
	~ProfileBlock() { ProfileEndEvent(id); }
};

#define PROFILE_FRAME() ProfileNewFrame()

#define PROFILE_BLOCK(name) \
	static const u16 profileNameId_##name = ProfileRegisterName(#name); \
	ProfileBlock profileBlock_##name(profileNameId_##name)

#else // !USE_PROFILE

#define PROFILE_FRAME()
#define PROFILE_BLOCK(name)

#endif // USE_PROFILE

#endif // ILU_PROFILE


////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

#if USE_PROFILE
#ifdef ILU_PROFILE_IMPLEMENTATION

static Profile sProfile = {};
thread_local u32 tThreadIndex = PROFILE_THREAD_NONE;

void ProfileInit()
{
	CreateMutex(sProfile.nameMutex);
}

static u32 ProfileThreadIndex()
{
	if (tThreadIndex == PROFILE_THREAD_NONE) {
		const i64 claimed = AtomicPreIncrement(&sProfile.threadClaimCount);
		tThreadIndex = claimed < MAX_PROFILE_THREADS ? (u32)claimed : PROFILE_THREAD_NONE;
	}
	return tThreadIndex;
}

u16 ProfileRegisterName(const char *name)
{
	MutexScope lock(sProfile.nameMutex);

	Profile &p = sProfile;

	// Lazy init of reserved/invalid id 0
	if (p.nameCount == 0) {
		p.names[0] = "<?>";
		p.nameCount = 1;
	}

	// Find existing id
	const u32 hash = HashStringFNV(name);
	u32 slot = hash & (MAX_PROFILE_NAME_SLOTS - 1);
	while (p.nameSlots[slot] != 0)
	{
		const u16 id = p.nameSlots[slot];
		if (StrEq(p.names[id], name)) {
			return id;
		}
		slot = (slot + 1) & (MAX_PROFILE_NAME_SLOTS - 1);
	}

	// If no space for the new name...
	const u32 len = StrLen(name);
	if (p.nameCount >= MAX_PROFILE_NAMES ||
			p.stringArenaSize + len + 1 > MAX_PROFILE_STRING_CHARS) {
		return 0;
	}

	// Copy string
	char *allocatedName = p.stringArena + p.stringArenaSize;
	MemCopy(allocatedName, name, len + 1);
	p.stringArenaSize += len + 1;

	// Return new id
	const u16 id = (u16)p.nameCount++;
	p.names[id] = allocatedName;
	p.nameSlots[slot] = id;

	return id;
}

const char *ProfileGetName(u16 nameId)
{
	const char *name = nameId < sProfile.nameCount ? sProfile.names[nameId] : "<?>";
	return name;
}

void ProfileFlush()
{
	// Build tree from previous frame (nodes are emitted at begin events, so they are stored in tree pre-order)
	u32 stack[MAX_PROFILE_STACKED_EVENTS] = {};
	u16 stackSize = 0;
	u32 skippedDepth = 0; // Begin events without node (capacity exceeded), so their end events must be consumed too

	const u32 index = ProfileThreadIndex();
	if (index == PROFILE_THREAD_NONE) { return; }
	ProfileThread &thread = sProfile.threads[index];

	ProfileFrame &frame = thread.frames[thread.frameIndex & (MAX_PROFILE_FRAMES - 1)];
	frame.nodeCount = 0;

	for (u32 i = 0; i < thread.eventCount; ++i)
	{
		ProfileEvent *event = &thread.events[i];
		if (event->type == ProfileEventType_Begin)
		{
			if (skippedDepth > 0 ||
					stackSize >= MAX_PROFILE_STACKED_EVENTS ||
					frame.nodeCount >= MAX_PROFILE_NODES)
			{
				skippedDepth++;
				thread.droppedEventCount++;
				continue;
			}

			const u16 parentIndex = stackSize > 0 ? (u16)stack[stackSize - 1] : PROFILE_NODE_NONE;
			stack[stackSize] = frame.nodeCount;
			frame.nodes[frame.nodeCount++] = {
				.begin = event->time,
				.end = event->time,
				.nameId = event->nameId,
				.parentIndex = parentIndex,
			};
			stackSize++;
		}
		else if (skippedDepth > 0)
		{
			skippedDepth--;
		}
		else if (stackSize == 0)
		{
			if (thread.droppedEventCount == 0) {
				LOG(Warning, "ProfileEndEvent('%s') does not match any begin event\n", ProfileGetName(event->nameId));
			}
		}
		else
		{
			ProfileNode *node = &frame.nodes[stack[--stackSize]];
			if ( event->nameId != node->nameId && thread.droppedEventCount == 0 )
			{
				LOG(Warning, "ProfileEndEvent('%s') does not match ProfileBeginEvent('%s')\n",
						ProfileGetName(event->nameId), ProfileGetName(node->nameId));
			}
			node->end = event->time;
		}
	}

	const ProfileTime now = GetTicks();

	// Close nodes left open
	while (stackSize > 0)
	{
		stackSize--;
		ProfileNode &node = frame.nodes[stack[stackSize]];
		node.end = now;
		if (thread.droppedEventCount == 0) {
			LOG(Warning, "ProfileBeginEvent('%s') does not match any end event\n", ProfileGetName(frame.nodes[stack[stackSize]].nameId));
		}
	}

	frame.begin = thread.frameBegin > 0 ? thread.frameBegin : now;
	frame.end = now;
	frame.index = thread.frameIndex;
	frame.droppedEventCount = thread.droppedEventCount; // build-time drops (loop) + record-time drops

	// Restart event count for the new frame
	thread.frameBegin = now;
	thread.eventCount = 0;
	thread.droppedEventCount = 0;
	thread.frameIndex++;
}

void ProfileNewFrame()
{
	ProfileFlush();
	sProfile.frameThreadIndex = ProfileThreadIndex();
}

void ProfileBeginEvent(u16 nameId)
{
	const u32 index = ProfileThreadIndex();
	if (index != PROFILE_THREAD_NONE)
	{
		ProfileThread &thread = sProfile.threads[index];
		if (thread.eventCount >= MAX_PROFILE_EVENTS) {
			thread.droppedEventCount++;
			return;
		}

		thread.events[thread.eventCount++] = {
			.time = GetTicks(),
			.type = ProfileEventType_Begin,
			.nameId = nameId,
		};
	}
}

void ProfileEndEvent(u16 nameId)
{
	const u32 index = ProfileThreadIndex();
	if (index != PROFILE_THREAD_NONE)
	{
		ProfileThread &thread = sProfile.threads[index];
		if (thread.eventCount >= MAX_PROFILE_EVENTS) {
			thread.droppedEventCount++;
			return;
		}

		thread.events[thread.eventCount++] = {
			.time = GetTicks(),
			.type = ProfileEventType_End,
			.nameId = nameId,
		};
	}
}

void ProfileRegisterThread(const char *name)
{
    const u32 index = ProfileThreadIndex();
    if (index != PROFILE_THREAD_NONE) {
		sProfile.threads[index].name = name;
	}
}

const char *ProfileGetThreadName(u32 threadIndex)
{
	const char *name = "Thread";
    if (threadIndex < MAX_PROFILE_THREADS && sProfile.threads[threadIndex].name) {
        name = sProfile.threads[threadIndex].name;
	}
    return name;
}

u32 ProfileGetThreadCount()
{
	const u32 c = (u32)sProfile.threadClaimCount;
	const u32 count = c < MAX_PROFILE_THREADS ? c : MAX_PROFILE_THREADS;
	return count;
}

u32 ProfileGetThreadFrameCount(u32 threadIndex)
{
	u32 frameCount = 0;
	if (threadIndex < MAX_PROFILE_THREADS)
	{
		const u64 built = sProfile.threads[threadIndex].frameIndex;
		frameCount = built < MAX_PROFILE_FRAMES ? (u32)built :  MAX_PROFILE_FRAMES;
	}
	return frameCount;
}

ProfileFrame ProfileGetThreadFrame(u32 threadIndex, u32 age)
{
	ProfileFrame frame = {};
	if (threadIndex < MAX_PROFILE_THREADS && age < ProfileGetThreadFrameCount(threadIndex))
	{
		const ProfileThread &thread = sProfile.threads[threadIndex];
		const u64 index = thread.frameIndex - 1 - age;
		frame = thread.frames[index & (MAX_PROFILE_FRAMES - 1)];
	}
	return frame;
}

// Convenience functions

u32 ProfileGetFrameCount()
{
	const u32 frameCount = ProfileGetThreadFrameCount(sProfile.frameThreadIndex);
	return frameCount;
}

ProfileFrame ProfileGetFrame(u32 age)
{
	const ProfileFrame frame = ProfileGetThreadFrame(sProfile.frameThreadIndex, age);
	return frame;
}

#endif // ILU_PROFILE_IMPLEMENTATION
#endif // USE_PROFILE

