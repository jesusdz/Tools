#ifndef TOOLS_PROFILE
#define TOOLS_PROFILE

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

// Balanced begin/end pairs produce one node per two events
CT_ASSERT(MAX_PROFILE_NODES >= MAX_PROFILE_EVENTS / 2);
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
	u16 level;
};

struct ProfileFrame
{
	u32 nodeCount;
	u32 droppedEventCount;
	ProfileNode nodes[MAX_PROFILE_NODES]; // Ring buffer
	ProfileTime begin;
	ProfileTime end;
	u64 index;
};

struct Profile
{
	// Scratch for current frame events
	ProfileEvent events[MAX_PROFILE_EVENTS];
	u32 eventCount;
	u32 droppedEventCount;     // Events lost in the frame being recorded
	ProfileTime frameBegin;

	// History of frame data
	ProfileFrame frames[MAX_PROFILE_FRAMES];
	u64 frameIndex;

	// Ad-hoc string interning for names
	const char *names[MAX_PROFILE_NAMES];
	u32 nameCount;
	u16 nameSlots[MAX_PROFILE_NAME_SLOTS];
	char stringArena[MAX_PROFILE_STRING_CHARS];
	u32 stringArenaSize;
};

u16 ProfileRegisterName(const char *name);
const char *ProfileGetName(u16 nameId);
void ProfileNewFrame();
void ProfileBeginEvent(u16 nameId);
void ProfileEndEvent(u16 nameId);
u32 ProfileGetFrameCount();
const ProfileFrame &ProfileGetFrame(u32 age);


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

#endif // TOOLS_PROFILE

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

#if USE_PROFILE
#ifdef TOOLS_PROFILE_IMPLEMENTATION

static Profile sProfile = {};

u16 ProfileRegisterName(const char *name)
{
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

void ProfileNewFrame()
{
	// Build tree from previous frame (nodes are emitted at begin events, so they are stored in tree pre-order)
	u32 stack[MAX_PROFILE_STACKED_EVENTS] = {};
	u16 stackSize = 0;
	u32 skippedDepth = 0; // Begin events without node (capacity exceeded), so their end events must be consumed too

	ProfileFrame &frame = sProfile.frames[sProfile.frameIndex & (MAX_PROFILE_FRAMES - 1)];
	frame.nodeCount = 0;
	frame.droppedEventCount = 0;

	for (u32 i = 0; i < sProfile.eventCount; ++i)
	{
		ProfileEvent *event = &sProfile.events[i];
		if (event->type == ProfileEventType_Begin)
		{
			if (skippedDepth > 0 ||
					stackSize >= MAX_PROFILE_STACKED_EVENTS ||
					frame.nodeCount >= MAX_PROFILE_NODES)
			{
				skippedDepth++;
				frame.droppedEventCount++;
				continue;
			}

			stack[stackSize] = frame.nodeCount;
			frame.nodes[frame.nodeCount++] = {
				.begin = event->time,
				.end = event->time,
				.nameId = event->nameId,
				.level = stackSize,
			};
			stackSize++;
		}
		else if (skippedDepth > 0)
		{
			skippedDepth--;
		}
		else if (stackSize == 0)
		{
			if (sProfile.droppedEventCount == 0) {
				LOG(Warning, "ProfileEndEvent('%s') does not match any begin event\n", ProfileGetName(event->nameId));
			}
		}
		else
		{
			ProfileNode *node = &frame.nodes[stack[--stackSize]];
			if ( event->nameId != node->nameId && sProfile.droppedEventCount == 0 )
			{
				LOG(Warning, "ProfileEndEvent('%s') does not match ProfileBeginEvent('%s')\n",
						ProfileGetName(event->nameId), ProfileGetName(node->nameId));
			}
			node->end = event->time;
		}
	}

	while (stackSize > 0)
	{
		stackSize--;
		if (sProfile.droppedEventCount == 0) {
			LOG(Warning, "ProfileBeginEvent('%s') does not match any end event\n", ProfileGetName(frame.nodes[stack[stackSize]].nameId));
		}
	}

	const ProfileTime now = GetTicks();
	frame.begin = sProfile.frameBegin > 0 ? sProfile.frameBegin : now;
	frame.end = now;
	frame.index = sProfile.frameIndex;
	frame.droppedEventCount = sProfile.droppedEventCount;

	// Restart event count for the new frame
	sProfile.frameBegin = now;
	sProfile.eventCount = 0;
	sProfile.droppedEventCount = 0;

	sProfile.frameIndex++;
}

void ProfileBeginEvent(u16 nameId)
{
	if (sProfile.eventCount >= MAX_PROFILE_EVENTS) {
		sProfile.droppedEventCount++;
		return;
	}

	sProfile.events[sProfile.eventCount++] = {
		.time = GetTicks(),
		.type = ProfileEventType_Begin,
		.nameId = nameId,
	};
}

void ProfileEndEvent(u16 nameId)
{
	if (sProfile.eventCount >= MAX_PROFILE_EVENTS) {
		sProfile.droppedEventCount++;
		return;
	}

	sProfile.events[sProfile.eventCount++] = {
		.time = GetTicks(),
		.type = ProfileEventType_End,
		.nameId = nameId,
	};
}

u32 ProfileGetFrameCount()
{
	const u64 built = sProfile.frameIndex;
	const u32 frameCount = built < MAX_PROFILE_FRAMES ? (u32)built :  MAX_PROFILE_FRAMES;
	return frameCount;
}

const ProfileFrame &ProfileGetFrame(u32 age)
{
	if (age < ProfileGetFrameCount())
	{
		const u64 index = sProfile.frameIndex - 1 - age;
		ProfileFrame &frame = sProfile.frames[index & (MAX_PROFILE_FRAMES - 1)];
		return frame;
	}
	else
	{
		static const ProfileFrame emptyFrame = {};
		return emptyFrame;
	}
}

#endif // TOOLS_PROFILE_IMPLEMENTATION
#endif // USE_PROFILE

