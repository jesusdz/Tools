#ifndef AUDIO_H
#define AUDIO_H

//#include "tools_mod.h"
#include "ibxm/ibxm.h"


#define MAX_AUDIO_CLIPS 16
#define MAX_AUDIO_SOURCES 16
#define AUDIO_CHUNK_SAMPLE_COUNT (48000u/4u)

////////////////////////////////////////////////////////////////////////
// Types

enum AudioClipLoadSource
{
	AUDIO_CLIP_LOAD_SOURCE_WAV,
	//AUDIO_CLIP_LOAD_SOURCE_MOD,
	AUDIO_CLIP_LOAD_SOURCE_ASSETS,
};

struct AudioClip
{
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	AudioClipLoadSource loadSource;
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

	// Music ring buffer
	i16 *musicBuffer;
	u32 musicBufferSampleCount; // Mono samples count

	// Music play state
	bool musicIsPlaying;
	u32 musicBufferReadSampleIndex;
	u32 musicBufferWriteSampleIndex;

	// MOD tracks
	Arena moduleArena;
	struct module *module;
	bool moduleLoaded;
	u32 moduleSampleCount;
	struct replay *moduleReplay;

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
Handle CreateAudioClip(Engine &engine, const BinAudioClip &binAudioClip);
Handle CreateAudioClip(Engine &engine, const AudioClipDesc &audioClipDesc);
void RemoveAudioClip(Engine &engine, AudioClipH handle, bool freeHandle = true);
u32 PlayAudioClip(Engine &engine, u32 audioClipIndex);
bool IsActiveAudioSource(Engine &engine, u32 audioSourceIndex);
bool IsPausedAudioSource(Engine &engine, u32 audioSourceIndex);
void PauseAudioSource(Engine &engine, u32 audioSourceIndex);
void ResumeAudioSource(Engine &engine, u32 audioSourceIndex);
void StopAudioSource(Engine &engine, u32 audioSourceIndex);

void PreRenderAudio(Engine &engine);
void RenderAudio(Engine &engine, SoundBuffer &soundBuffer);

//struct module LoadModule(const byte *data, u32 size);

void MusicLoad(Audio &audio);
void MusicPlay(Engine &engine);
void MusicPause(Engine &engine);
void MusicStop(Engine &engine);
bool MusicIsPlaying(Engine &engine);

#endif // AUDIO_H
