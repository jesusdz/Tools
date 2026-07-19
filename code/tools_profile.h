#ifndef TOOLS_PROFILE
#define TOOLS_PROFILE

typedef u64 Ticks;

enum ProfileEventType
{
	ProfileEventType_Begin,
	ProfileEventType_End,
};

struct ProfileEvent
{
	ProfileEventType type;
	const char *name;
	Ticks time;
};

struct ProfileNode
{
	const char *name;
	Ticks begin;
	Ticks end;
	u32 level;
};

struct ProfileNodes
{
	ProfileNode *nodes;
	u32 nodeCount;
};

constexpr u32 MAX_PROFILE_EVENTS = 1024;
constexpr u32 MAX_PROFILE_NODES = 512;
constexpr u32 MAX_PROFILE_STACKED_EVENTS = 16;

// Balanced begin/end pairs produce one node per two events
static_assert(MAX_PROFILE_NODES >= MAX_PROFILE_EVENTS / 2);

struct Profile
{
	ProfileEvent events[MAX_PROFILE_EVENTS];
	u32 eventCount;

	ProfileNode nodes[MAX_PROFILE_NODES];
	u32 nodeCount;
};

void ProfileNewFrame();
void ProfileBeginEvent(const char *name);
void ProfileEndEvent(const char *name);
ProfileNodes ProfileGetNodes();


////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile blocks

struct ProfileBlock
{
	const char *_name;

	ProfileBlock(const char *name) : _name(name) {
		ProfileBeginEvent(name);
	}
	~ProfileBlock() {
		ProfileEndEvent(_name);
	}
};

#define PROFILE_BLOCK(name) ProfileBlock profileBlock##name(#name)

#endif // TOOLS_PROFILE

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

#ifdef TOOLS_PROFILE_IMPLEMENTATION

static Profile sProfile = {};

void ProfileNewFrame()
{
	// Build tree from previous frame (nodes are emitted at begin events, so they are stored in tree pre-order)
	u32 stack[MAX_PROFILE_STACKED_EVENTS] = {};
	u32 stackSize = 0;

	sProfile.nodeCount = 0;

	for (u32 i = 0; i < sProfile.eventCount; ++i)
	{
		ProfileEvent *event = &sProfile.events[i];
		if (event->type == ProfileEventType_Begin)
		{
			ASSERT(stackSize < MAX_PROFILE_STACKED_EVENTS);
			ASSERT(sProfile.nodeCount < MAX_PROFILE_NODES);

			stack[stackSize] = sProfile.nodeCount;
			sProfile.nodes[sProfile.nodeCount++] = {
				.name = event->name,
				.begin = event->time,
				.end = event->time,
				.level = stackSize,
			};
			stackSize++;
		}
		else if (stackSize == 0)
		{
			LOG(Warning, "ProfileEndEvent('%s') does not match any begin event\n", event->name);
		}
		else
		{
			ProfileNode *node = &sProfile.nodes[stack[--stackSize]];
			if ( !StrEq(event->name, node->name) )
			{
				LOG(Warning, "ProfileEndEvent('%s') does not match ProfileBeginEvent('%s')\n", event->name, node->name);
			}
			node->end = event->time;
		}
	}

	while (stackSize > 0)
	{
		stackSize--;
		LOG(Warning, "ProfileBeginEvent('%s') does not match any end event\n", sProfile.nodes[stack[stackSize]].name);
	}

	// Restart event count for the new frame
	sProfile.eventCount = 0;
}

void ProfileBeginEvent(const char *name)
{
	ASSERT(sProfile.eventCount < MAX_PROFILE_EVENTS);

	sProfile.events[sProfile.eventCount++] = {
		.type = ProfileEventType_Begin,
		.name = name,
		.time = GetTicks(),
	};
}

void ProfileEndEvent(const char *name)
{
	ASSERT(sProfile.eventCount < MAX_PROFILE_EVENTS);

	sProfile.events[sProfile.eventCount++] = {
		.type = ProfileEventType_End,
		.name = name,
		.time = GetTicks(),
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

#endif // TOOLS_PROFILE_IMPLEMENTATION

