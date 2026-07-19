#ifndef TOOLS_PROFILE
#define TOOLS_PROFILE

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

struct ProfileNodes
{
	ProfileNode *nodes;
	u32 nodeCount;
};

constexpr u32 MAX_PROFILE_EVENTS = 1024;
constexpr u32 MAX_PROFILE_NODES = 512;
constexpr u32 MAX_PROFILE_STACKED_EVENTS = 16;
constexpr u32 MAX_PROFILE_NAMES = 256;
constexpr u32 MAX_PROFILE_NAME_SLOTS = 512;
constexpr u32 MAX_PROFILE_STRING_CHARS = KB(8);

// Balanced begin/end pairs produce one node per two events
CT_ASSERT(MAX_PROFILE_NODES >= MAX_PROFILE_EVENTS / 2);
CT_ASSERT((MAX_PROFILE_NAME_SLOTS & (MAX_PROFILE_NAME_SLOTS - 1)) == 0); // Is power of 2
CT_ASSERT(MAX_PROFILE_NAME_SLOTS >= 2 * MAX_PROFILE_NAMES);

struct Profile
{
	ProfileEvent events[MAX_PROFILE_EVENTS];
	u32 eventCount;

	ProfileNode nodes[MAX_PROFILE_NODES];
	u32 nodeCount;

	const char *names[MAX_PROFILE_NAMES];
	u32 nameCount;

	u16 nameSlots[MAX_PROFILE_NAME_SLOTS];

	char stringArena[MAX_PROFILE_STRING_CHARS];
	u32 stringArenaSize;

	u32 droppedEventCount;     // Events lost in the frame being recorded
	u32 lastDroppedEventCount; // Events lost in the last built frame
};

u16 ProfileRegisterName(const char *name);
const char *ProfileGetName(u16 nameId);
void ProfileNewFrame();
void ProfileBeginEvent(u16 nameId);
void ProfileEndEvent(u16 nameId);
ProfileNodes ProfileGetNodes();
u32 ProfileGetDroppedEventCount();


////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile blocks

struct ProfileBlock
{
	u16 id;
	ProfileBlock(u16 nameId) : id(nameId) { ProfileBeginEvent(nameId); }
	~ProfileBlock() { ProfileEndEvent(id); }
};

#define PROFILE_BLOCK(name) \
	static const u16 profileNameId_##name = ProfileRegisterName(#name); \
	ProfileBlock profileBlock_##name(profileNameId_##name)

#endif // TOOLS_PROFILE

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

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
	return sProfile.names[nameId];
}

void ProfileNewFrame()
{
	// Build tree from previous frame (nodes are emitted at begin events, so they are stored in tree pre-order)
	u32 stack[MAX_PROFILE_STACKED_EVENTS] = {};
	u16 stackSize = 0;
	u32 skippedDepth = 0; // Begin events without node (capacity exceeded), so their end events must be consumed too

	sProfile.nodeCount = 0;

	for (u32 i = 0; i < sProfile.eventCount; ++i)
	{
		ProfileEvent *event = &sProfile.events[i];
		if (event->type == ProfileEventType_Begin)
		{
			if (skippedDepth > 0 ||
					stackSize >= MAX_PROFILE_STACKED_EVENTS ||
					sProfile.nodeCount >= MAX_PROFILE_NODES)
			{
				skippedDepth++;
				sProfile.droppedEventCount++;
				continue;
			}

			stack[stackSize] = sProfile.nodeCount;
			sProfile.nodes[sProfile.nodeCount++] = {
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
			ProfileNode *node = &sProfile.nodes[stack[--stackSize]];
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
			LOG(Warning, "ProfileBeginEvent('%s') does not match any end event\n", ProfileGetName(sProfile.nodes[stack[stackSize]].nameId));
		}
	}

	// Restart event count for the new frame
	sProfile.eventCount = 0;
	sProfile.lastDroppedEventCount = sProfile.droppedEventCount;
	sProfile.droppedEventCount = 0;
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

ProfileNodes ProfileGetNodes()
{
	ProfileNodes res = {
		.nodes = sProfile.nodes,
		.nodeCount = sProfile.nodeCount,
	};
	return res;
}

u32 ProfileGetDroppedEventCount()
{
	return sProfile.lastDroppedEventCount;
}

#endif // TOOLS_PROFILE_IMPLEMENTATION

