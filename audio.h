#ifndef AUDIO_H
#define AUDIO_H

#define MAX_AUDIO_CLIPS 16
#define MAX_AUDIO_SOURCES 16
#define AUDIO_CHUNK_SAMPLE_COUNT (48000u/4u)

////////////////////////////////////////////////////////////////////////
// Types

struct AudioClip
{
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	bool loadedFromAssetsFile;
	union
	{
		BinLocation location;
		const char *filename;
	};
};

enum AudioSourceState
{
	AUDIO_SOURCE_STATE_IDLE,
	AUDIO_SOURCE_STATE_PLAYING,
	AUDIO_SOURCE_STATE_PAUSED,
	AUDIO_SOURCE_STATE_LOCKED,
};

typedef Handle AudioClipH;

struct AudioSource
{
	AudioClipH clip;
	u32 lastWriteSampleIndex = 0;
	volatile_u32 state;
};

struct AudioChunk
{
	AudioClipH clipH;
	u32 index;
	i16 samples[AUDIO_CHUNK_SAMPLE_COUNT];
	AudioChunk *prev;
	AudioChunk *next;
};

struct Audio
{
	AudioClip clips[MAX_AUDIO_CLIPS] = {};
	AudioClipDesc clipDescs[MAX_AUDIO_CLIPS] = {};
	HandleManager clipHandles;

	AudioSource sources[MAX_AUDIO_SOURCES] = {};

	// Circular list of audio chunks
	AudioChunk audioChunkSentinel;

	bool initialized;
};


////////////////////////////////////////////////////////////////////////
// Functions

struct Engine;

bool InitializeAudio(Audio &audio, Arena &globalArena);

bool LoadAudioClipFromWAVFile(const char *filename, Arena &arena, AudioClip &audioClip, void **outSamples);
bool LoadSamplesFromWAVFile(const char *filename, void *samples, u32 firstSampleIndex, u32 sampleCount);

AudioClip &GetAudioClip(Audio &audio, Handle handle);
AudioClipDesc &GetAudioClipDesc(Audio &audio, Handle handle);
void CreateAudioClip(Engine &engine, const BinAudioClip &binAudioClip);
void CreateAudioClip(Engine &engine, const AudioClipDesc &audioClipDesc);
void RemoveAudioClip(Engine &engine, AudioClipH handle, bool freeHandle = true);
u32 PlayAudioClip(Engine &engine, u32 audioClipIndex);
bool IsActiveAudioSource(Engine &engine, u32 audioSourceIndex);
bool IsPausedAudioSource(Engine &engine, u32 audioSourceIndex);
void PauseAudioSource(Engine &engine, u32 audioSourceIndex);
void ResumeAudioSource(Engine &engine, u32 audioSourceIndex);
void StopAudioSource(Engine &engine, u32 audioSourceIndex);

void RenderAudio(Engine &engine, SoundBuffer &soundBuffer);

#endif // AUDIO_H
