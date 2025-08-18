
////////////////////////////////////////////////////////////////////////
// Audio system init

#define AUDIO_CHUNK_MEMORY MB(1)

bool InitializeAudio(Audio &audio, Arena &globalArena)
{
	// Allocate audio chunks

	const u32 totalChunkCount = AUDIO_CHUNK_MEMORY / sizeof(AudioChunk);

	// Make sure we at least have as many audio chunks as twice the number of
	// simultaneos audio sources. This is needed because every sound is split
	// in sequences of chunks, so we play one and prefetch the next one.
	if (totalChunkCount < MAX_AUDIO_SOURCES * 2)
	{
		LOG(Error, "- totalChunkCount (%u) must be <= MAX_AUDIO_SOURCES * 2 (%u)\n", totalChunkCount, MAX_AUDIO_SOURCES * 2);
		return false;
	}

	AudioChunk *chunks = PushArray(globalArena, AudioChunk, totalChunkCount);
	if ( chunks == nullptr )
	{
		LOG(Error, "- Could not allocate memory for %u audio chunks\n", totalChunkCount);
		return false;
	}

	// Initialize doubly linked list of chunks

	for (u32 i = 0; i < totalChunkCount; ++i)
	{
		chunks[i].next = &chunks[i+1];
		chunks[i].prev = &chunks[i-1];
	}

	AudioChunk *first = &chunks[0];
	AudioChunk *last = &chunks[totalChunkCount-1];
	first->prev = &audio.audioChunkSentinel;
	last->next = &audio.audioChunkSentinel;
	audio.audioChunkSentinel.next = first;
	audio.audioChunkSentinel.prev = last;

	Initialize(audio.clipHandles, globalArena, MAX_AUDIO_SOURCES);

	FullWriteBarrier();
	audio.initialized = true;

	return true;
}



////////////////////////////////////////////////////////////////////////
// WAV file loading

#define QUAD_CHAR(a,b,c,d) (a) | (b<<8) | (c<<16) | (d<<24)

enum RIFFCode
{
	RIFF_RIFF = QUAD_CHAR('R', 'I', 'F', 'F'),
	RIFF_WAVE = QUAD_CHAR('W', 'A', 'V', 'E'),
	RIFF_fmt  = QUAD_CHAR('f', 'm', 't', ' '),
	RIFF_data = QUAD_CHAR('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)

struct WAVE_header
{
	u32 ChunkID;
	u32 ChunkSize;
	u32 Format;
};

struct WAVE_chunk
{
	u32 ID;
	u32 Size;
};

struct WAVE_fmt
{
	u16 AudioFormat;
	u16 NumChannels;
	u32 SampleRate;
	u32 ByteRate;
	u16 BlockAlign;
	u16 BitsPerSample;
	u16 cbSize;
	u16 ValidBitsPerSample;
	u32 ChannelMask;
	u8  SubFormat[16];
};

#pragma pack(pop)

bool LoadAudioClipFromWAVFile(const char *filename, Arena &arena, AudioClip &audioClip, void **outSamples)
{
	FILE *file = fopen(filename, "rb");
	if (file != nullptr)
	{
		WAVE_header Header;
		WAVE_chunk Chunk;
		WAVE_fmt Fmt;
		u32 dataSize;
		void *data = nullptr;

		fread(&Header, sizeof(Header), 1, file);
		ASSERT(Header.ChunkID == RIFF_RIFF); // "RIFF"
		ASSERT(Header.Format == RIFF_WAVE); // "WAVE"

		while (1)
		{
			fread(&Chunk, sizeof(Chunk), 1, file);
			if (feof(file)) {
				break;
			}

			switch (Chunk.ID)
			{
				case RIFF_fmt:
					fread(&Fmt, Chunk.Size, 1, file);
					ASSERT(Fmt.AudioFormat == 1); // 1 means PCM
					ASSERT(Fmt.SampleRate == 48000);
					ASSERT(Fmt.NumChannels == 2);
					ASSERT(Fmt.BitsPerSample == 16);
					break;
				case RIFF_data:
					ASSERT(data == nullptr);
					dataSize = Chunk.Size;
					if (outSamples != nullptr)
					{
						data = PushSize(arena, dataSize);
						ASSERT(data != nullptr);
						fread(data, dataSize, 1, file);
						*outSamples = data;
					}
					else
					{
						fseek(file, dataSize, SEEK_CUR);
					}
					break;
				default:
					ASSERT(Chunk.Size > 0);
					fseek(file, Chunk.Size, SEEK_CUR);
					break;
			}
		}

		audioClip.filename = filename;
		audioClip.sampleSize = Fmt.BitsPerSample / 8;
		audioClip.samplingRate = Fmt.SampleRate;
		audioClip.channelCount = Fmt.NumChannels;
		audioClip.sampleCount = dataSize / audioClip.sampleSize;

		fclose(file);
		return true;
	}
	else
	{
		LOG(Warning, "Could not load sound file %s\n", filename);
		return false;
	}
}

bool LoadSamplesFromWAVFile(const char *filename, void *samples, u32 firstSampleIndex, u32 sampleCount)
{
	FILE *file = fopen(filename, "rb");
	if (file != nullptr)
	{
		WAVE_header Header;
		WAVE_chunk Chunk;
		WAVE_fmt Fmt;
		const u32 dataOffset = firstSampleIndex * sizeof(i16);
		const u32 dataSize = sampleCount * sizeof(i16);;

		fread(&Header, sizeof(Header), 1, file);
		ASSERT(Header.ChunkID == RIFF_RIFF); // "RIFF"
		ASSERT(Header.Format == RIFF_WAVE); // "WAVE"

		bool keepReading = true;

		while (keepReading)
		{
			fread(&Chunk, sizeof(Chunk), 1, file);
			if (feof(file)) {
				break;
			}

			switch (Chunk.ID)
			{
				case RIFF_fmt:
					fread(&Fmt, Chunk.Size, 1, file);
					ASSERT(Fmt.AudioFormat == 1); // 1 means PCM
					ASSERT(Fmt.SampleRate == 48000);
					ASSERT(Fmt.NumChannels == 2);
					ASSERT(Fmt.BitsPerSample == 16);
					break;
				case RIFF_data:
					ASSERT( dataOffset + dataSize <= Chunk.Size );
					fseek(file, dataOffset, SEEK_CUR);
					fread(samples, dataSize, 1, file);
					keepReading = false;
					break;
				default:
					ASSERT(Chunk.Size > 0);
					fseek(file, Chunk.Size, SEEK_CUR);
					break;
			}
		}

		fclose(file);
		return true;
	}
	else
	{
		LOG(Warning, "Could not load sound file %s\n", filename);
		return false;
	}
}



////////////////////////////////////////////////////////////////////////
// AudioClip and AudioSource management

AudioClip &GetAudioClip(Audio &audio, Handle handle)
{
	ASSERT( IsValidHandle(audio.clipHandles, handle) );
	AudioClip &audioClip = audio.clips[handle.idx];
	return audioClip;
}

void CreateAudioClip(Engine &engine, const BinAudioClip &binAudioClip)
{
	Audio &audio = engine.audio;

	Handle handle = NewHandle(audio.clipHandles);

	if ( IsValidHandle(audio.clipHandles, handle) )
	{
		AudioClip &audioClip = GetAudioClip(audio, handle);

		const BinAudioClipDesc &desc = *binAudioClip.desc;
		audioClip.sampleSize = desc.sampleSize;
		audioClip.samplingRate = desc.samplingRate;
		audioClip.channelCount = desc.channelCount;
		audioClip.sampleCount = desc.sampleCount;
		audioClip.loadedFromAssetsFile = true;
		audioClip.location = desc.location;
	}
	else
	{
		LOG(Warning, "Could not load audio clip %s (no more space left for audio clips)\n", "<audio-clip>");
	}
}

void CreateAudioClip(Engine &engine, const AudioClipDesc &audioClipDesc)
{
	Audio &audio = engine.audio;
	Arena &arena = engine.platform.dataArena;

	Handle handle = NewHandle(audio.clipHandles);

	if ( IsValidHandle(audio.clipHandles, handle) )
	{
		AudioClip &audioClip = GetAudioClip(audio, handle);
		//ASSERT(!audioClip.samples);

		LoadAudioClipFromWAVFile(audioClipDesc.filename, arena, audioClip, nullptr);
		audioClip.loadedFromAssetsFile = false;
	}
	else
	{
		LOG(Warning, "Could not load audio clip %s (no more space left for audio clips)\n", audioClipDesc.filename);
	}
}

void RemoveAudioClip(Engine &engine, AudioClipH handle, bool freeHandle)
{
	AudioClip &clip = GetAudioClip(engine.audio, handle);
	clip = {};

	if (freeHandle) {
		FreeHandle(engine.audio.clipHandles, handle);
	}
}

#define INVALID_AUDIO_CLIP U32_MAX
#define INVALID_AUDIO_SOURCE U32_MAX

//u32 FindAudioClipIndex(Engine &engine, const char *name)
//{
//	u32 audioClipIndex = INVALID_AUDIO_CLIP;
//	for (u32 i = 0; i < audio.clipCount; ++i)
//	{
//		const AudioClip &audioClip = audio.clips[i];
//		if (StrEq(audioClip.name, name))
//		{
//			audioClipIndex = INVALID_AUDIO_CLIP;
//			break;
//		}
//	}
//	return audioClipIndex;
//}

u32 PlayAudioClip(Engine &engine, u32 audioClipIndex)
{
	Audio &audio = engine.audio;
	u32 audioSourceIndex = INVALID_AUDIO_SOURCE;

	if (audioClipIndex < audio.clipHandles.handleCount)
	{
		Handle clipHandle = GetHandleAt(audio.clipHandles, audioClipIndex);
		AudioClip &audioClip = GetAudioClip(audio, clipHandle);

		for (u32 i = 0; i < ARRAY_COUNT(audio.sources); ++i)
		{
			AudioSource &audioSource = audio.sources[i];

			if (audioSource.state == AUDIO_SOURCE_STATE_IDLE)
			{
				audioSource.clip = clipHandle;
				audioSource.lastWriteSampleIndex = 0;
				audioSourceIndex = i;

				FullWriteBarrier();
				audioSource.state = AUDIO_SOURCE_STATE_PLAYING;
				break;
			}
		}
	}

	if (audioSourceIndex == INVALID_AUDIO_SOURCE) {
		LOG(Warning, "Could not play audio clip %u\n", audioClipIndex);
	}

	return audioSourceIndex;
}

bool IsActiveAudioSource(Engine &engine, u32 audioSourceIndex)
{
	Audio &audio = engine.audio;
	bool active = false;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		AudioSource &audioSource = audio.sources[audioSourceIndex];
		active = audioSource.state != AUDIO_SOURCE_STATE_IDLE;
	}
	return active;
}

bool IsPausedAudioSource(Engine &engine, u32 audioSourceIndex)
{
	Audio &audio = engine.audio;
	bool paused = false;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		AudioSource &audioSource = audio.sources[audioSourceIndex];
		paused = audioSource.state == AUDIO_SOURCE_STATE_PAUSED;
	}
	return paused;
}

void PauseAudioSource(Engine &engine, u32 audioSourceIndex)
{
	Audio &audio = engine.audio;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		AudioSource &audioSource = audio.sources[audioSourceIndex];
		if (audioSource.state == AUDIO_SOURCE_STATE_PLAYING) {
			do {
				AtomicSwap(&audioSource.state, AUDIO_SOURCE_STATE_PLAYING, AUDIO_SOURCE_STATE_PAUSED);
			} while (audioSource.state != AUDIO_SOURCE_STATE_PAUSED && audioSource.state != AUDIO_SOURCE_STATE_IDLE);
		}
	}
}

void ResumeAudioSource(Engine &engine, u32 audioSourceIndex)
{
	Audio &audio = engine.audio;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		AudioSource &audioSource = audio.sources[audioSourceIndex];
		if (audioSource.state == AUDIO_SOURCE_STATE_PAUSED) {
			do {
				AtomicSwap(&audioSource.state, AUDIO_SOURCE_STATE_PAUSED, AUDIO_SOURCE_STATE_PLAYING);
			} while (audioSource.state != AUDIO_SOURCE_STATE_PLAYING && audioSource.state != AUDIO_SOURCE_STATE_IDLE);
		}
	}
}

void StopAudioSource(Engine &engine, u32 audioSourceIndex)
{
	Audio &audio = engine.audio;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		AudioSource &audioSource = audio.sources[audioSourceIndex];
		bool stopped = false;
		while ( !stopped ) {
			stopped = AtomicSwap(&audioSource.state, AUDIO_SOURCE_STATE_PLAYING, AUDIO_SOURCE_STATE_IDLE);
			stopped = stopped || AtomicSwap(&audioSource.state, AUDIO_SOURCE_STATE_PAUSED, AUDIO_SOURCE_STATE_IDLE);
		}
		audioSource = {};
	}
}



////////////////////////////////////////////////////////////////////////
// Audio mixer

void RenderAudio(Engine &engine, SoundBuffer &soundBuffer)
{
	// Wave parameters
	const u32 ToneHz = 256;
	const i32 ToneVolume = 4000;
	const u32 WavePeriod = soundBuffer.samplesPerSecond / ToneHz;
	static f32 tSine = 0.0f;

	//LOG(Debug, "bitrate:%u, wavePeriod:%u\n", soundBuffer.samplesPerSecond, WavePeriod);

#if 1
	Audio &audio = engine.audio;

	if (!audio.initialized) {
		return;
	}

	Scratch scratch;

	f32 *realSamples = PushArray(scratch.arena, f32, soundBuffer.sampleCount * 2.0f );

	// Clear sound buffer
	f32 *samplePtr = realSamples;
	for (u32 i = 0; i < soundBuffer.sampleCount; ++i)
	{
		*samplePtr++ = 0.0f;
		*samplePtr++ = 0.0f;
	}

	for (u32 i = 0; i < ARRAY_COUNT(audio.sources); ++i)
	{
		AudioSource &audioSource = audio.sources[i];

		bool audioSourceIsValid = IsValidHandle(audio.clipHandles, audioSource.clip);

		if (audioSourceIsValid && AtomicSwap(&audioSource.state, AUDIO_SOURCE_STATE_PLAYING, AUDIO_SOURCE_STATE_LOCKED))
		{
			AudioClip &audioClip = GetAudioClip(audio, audioSource.clip);

			const u32 chunkCount = (audioClip.sampleCount - 1) / AUDIO_CHUNK_SAMPLE_COUNT + 1;
			const u32 currChunkIndex = audioSource.lastWriteSampleIndex / AUDIO_CHUNK_SAMPLE_COUNT;
			const u32 nextChunkIndex = Min(currChunkIndex + 1, chunkCount - 1);

			const u32 chunkIndices[] = { currChunkIndex, nextChunkIndex };
			const u32 prefetchChunkCount = nextChunkIndex - currChunkIndex + 1;
			ASSERT(prefetchChunkCount <= 2);

			// soundBuffer.sampleCount is for stereo samples (each stereo sample is 2 mono samples)
			u32 requestedSampleCount = soundBuffer.sampleCount * 2;

			f32 *dstSample = realSamples;

			for (u32 i = 0; i < prefetchChunkCount; ++i)
			{
				const u32 chunkIndex = chunkIndices[i];
				const u32 firstSampleIndex = chunkIndex * AUDIO_CHUNK_SAMPLE_COUNT;

				// Search chunk
				AudioChunk *chunk = audio.audioChunkSentinel.next;
				AudioChunk *end = &audio.audioChunkSentinel;
				while (chunk != end)
				{
					if ( audioSource.clip == chunk->clipH && chunkIndex == chunk->index ) {
						break;
					}
					chunk = chunk->next;
				}

				// No chunk found, get LRU and populate it
				if ( chunk == end )
				{
					chunk = audio.audioChunkSentinel.prev;
					chunk->clipH = audioSource.clip;
					chunk->index = chunkIndex;

					const u32 chunkSampleCount = (chunkIndex == chunkCount - 1) ? audioClip.sampleCount % AUDIO_CHUNK_SAMPLE_COUNT : AUDIO_CHUNK_SAMPLE_COUNT;
					if ( audioClip.loadedFromAssetsFile )
					{
						FileSeek(engine.assets.file, audioClip.location.offset + firstSampleIndex * sizeof(i16));
						ReadFromFile(engine.assets.file, chunk->samples, chunkSampleCount * sizeof(i16));
					}
					else
					{
						LoadSamplesFromWAVFile(audioClip.filename, chunk->samples, firstSampleIndex, chunkSampleCount);
					}
				}

				// Remove chunk from the list
				chunk->next->prev = chunk->prev;
				chunk->prev->next = chunk->next;
				// Put it back first in the list
				chunk->next = audio.audioChunkSentinel.next;
				chunk->prev = &audio.audioChunkSentinel;
				chunk->next->prev = chunk;
				chunk->prev->next = chunk;

				// Copy requested samples from chunk to real samples
				const i16 *srcSample = chunk->samples + audioSource.lastWriteSampleIndex - firstSampleIndex;

				const u32 remainingClipSampleCount = audioClip.sampleCount - audioSource.lastWriteSampleIndex;
				const u32 remainingChunkSampleCount = firstSampleIndex + AUDIO_CHUNK_SAMPLE_COUNT - audioSource.lastWriteSampleIndex;
				const u32 remainingSampleCount = Min(remainingClipSampleCount, remainingChunkSampleCount);
				const u32 sampleCount = Min(remainingSampleCount, requestedSampleCount);

				for (u32 sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
				{
					*dstSample += (f32)*srcSample;
					dstSample++;
					srcSample++;
				}

				audioSource.lastWriteSampleIndex += sampleCount;
				if (audioSource.lastWriteSampleIndex >= audioClip.sampleCount)
				{
					audioSourceIsValid = false;
				}

				requestedSampleCount -= sampleCount;
			}

			FullWriteBarrier();
			audioSource.state = AUDIO_SOURCE_STATE_PLAYING;
		}

		if ( !audioSourceIsValid )
		{
			// This implies AUDIO_SOURCE_STATE_IDLE
			audioSource = {};
		}
	}

	// Convert f32 samples back to i16 samples
	f32 *srcSample = realSamples;
	i16 *dstSample = soundBuffer.samples;
	for (u32 i = 0; i < soundBuffer.sampleCount; ++i)
	{
		*dstSample++ = (i16)*srcSample++;
		*dstSample++ = (i16)*srcSample++;
	}
#else
	samplePtr = soundBuffer.samples;
	for (u32 i = 0; i < soundBuffer.sampleCount; ++i)
	{
		// Sine wave
		tSine += TwoPi/(f32)WavePeriod;
		while ( tSine >= TwoPi ) { tSine -= TwoPi; }
		const f32 sinValue = Sin(tSine);
		const i16 sample = (i16)(sinValue * ToneVolume);

		*samplePtr++ = sample;
		*samplePtr++ = sample;
	}
#endif

}

