#include "../tools.h"

// Use the newer ALSA API
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

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
SND_PCM_PREPARE* FP_snd_pcm_prepare;
SND_PCM_CLOSE* FP_snd_pcm_close;
SND_PCM_DRAIN* FP_snd_pcm_drain;

int main()
{
	// Load ALSA library
	DynamicLibrary alsa = OpenLibrary("libasound.so");
	if (!alsa)
	{
		LOG(Error, "Cound not open libasound.so\n");
		return -1;
	}

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
	FP_snd_pcm_prepare = (SND_PCM_PREPARE*) LoadSymbol(alsa, "snd_pcm_prepare");
	FP_snd_pcm_close = (SND_PCM_CLOSE*) LoadSymbol(alsa, "snd_pcm_close");
	FP_snd_pcm_drain = (SND_PCM_DRAIN*) LoadSymbol(alsa, "snd_pcm_drain");

	int res; // results

	// Open PCM device
	snd_pcm_t *handle;
	res = FP_snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (res < 0) {
		LOG(Error, "unable to open pcm device: %s\n", FP_snd_strerror(res));
		return 1;
	}

	int dir = 0; // direction of approximate values
	unsigned int sampleRate = 48000; // bits/second (CD quality)
	unsigned int channelCount = 2;
	unsigned int bytesPerSample = 2;
	snd_pcm_uframes_t frames = 32; // period size of 32 frames

	// Allocate and configure hardware parameters
	snd_pcm_hw_params_t *params;
	FP_snd_pcm_hw_params_malloc(&params);
	FP_snd_pcm_hw_params_any(handle, params); // default values
	FP_snd_pcm_hw_params_set_channels(handle, params, channelCount);
	FP_snd_pcm_hw_params_set_rate_near(handle, params, &sampleRate, &dir);
	FP_snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE); // 16 bit little endian
	FP_snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	FP_snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	// Write the parameters to the driver
	res = FP_snd_pcm_hw_params(handle, params);
	if (res < 0) {
		LOG(Error, "unable to set hw parameters: %s\n", FP_snd_strerror(res));
		return 1;
	}

	unsigned int finalSampleRate = 0;
	FP_snd_pcm_hw_params_get_rate(params, &finalSampleRate, &dir);

	#if 0
	{
		// Check configuration
		unsigned int channelCount = 0;
		unsigned int sampleRate = 0; int dir = 0;
		snd_pcm_format_t format;
		snd_pcm_access_t access;
		FP_snd_pcm_hw_params_get_channels(params, &channelCount);
		FP_snd_pcm_hw_params_get_rate(params, &sampleRate, &dir);
		FP_snd_pcm_hw_params_get_format(params, &format);
		FP_snd_pcm_hw_params_get_access(params, &access);
		LOG(Debug, "channelCount %u\n", channelCount);
		LOG(Debug, "sampleRate %u %i\n", sampleRate, dir);
		LOG(Debug, "format %d\n", format);
		LOG(Debug, "access %d\n", access);
	}
	#endif // CHECK_CONFIGURATION

	#if 0
	// We want to loop for 5 seconds
	unsigned int periodTimeUs;
	FP_snd_pcm_hw_params_get_period_time(params, &periodTimeUs, &dir);

	snd_pcm_uframes_t framesPerPeriod;
	FP_snd_pcm_hw_params_get_period_size(params, &framesPerPeriod, &dir);
	LOG(Debug, "Period time us: %u\n", periodTimeUs);
	LOG(Debug, "Frames per period: %u\n", framesPerPeriod);
	#endif

	// Sin wave parameters
	const u32 toneHz = 261;
	const u32 toneVolume = 12000;
	const float wavePeriod = finalSampleRate / toneHz;
	float tSine = 0.0f;

	// Target frames per iteration
	const float time = 2.0f / 30.0f; // Two times what's needed to render two game frames
	const snd_pcm_uframes_t maxFramesToRender = finalSampleRate * time;

	// Buffer allocation
	const int bufferSize = maxFramesToRender * channelCount * bytesPerSample;
	char * buffer = (char *) malloc(bufferSize);

	// Iterate for 3 seconds
	long frameCount = 0;
	const long maxFrames = finalSampleRate * 3;

	while (frameCount < maxFrames)
	{
		snd_pcm_sframes_t availableFrames;
		snd_pcm_sframes_t delayFrames;
		if ( FP_snd_pcm_avail_delay(handle, &availableFrames, &delayFrames) == 0 ) {
			//LOG(Debug, "avail:%u / delay:%u\n", availableFrames, delayFrames);
		}

		snd_pcm_uframes_t framesToRender = maxFramesToRender - delayFrames;

		framesToRender = framesToRender < availableFrames ? framesToRender : availableFrames;

		u16 *samplePtr = (u16*)buffer;
		for (u32 i = 0; i < framesToRender; ++i)
		{
			tSine += TwoPi / wavePeriod;
			while ( tSine >= TwoPi ) { tSine -= TwoPi; }
			const float sinValue = Sin( tSine );
			const i16 sample = (i16)(sinValue * toneVolume);

			*samplePtr++ = sample;
			*samplePtr++ = sample;
			++frameCount;
		}

		if ( framesToRender > 0 )
		{
			res = FP_snd_pcm_writei(handle, buffer, framesToRender);

			if ( res >= 0 ) {
				//LOG(Info, "%u frames written\n", res);
			} else if (res == -EPIPE) {
				LOG(Error, "An underrun occurred: %s\n", FP_snd_strerror(res));
				FP_snd_pcm_prepare(handle);
			} else if (res == -EBADFD) {
				LOG(Error, "PCM is not in the right state (PREPARED or RUNNING): %s\n", FP_snd_strerror(res));
			} else if (res == -ESTRPIPE) {
				LOG(Error, "Underrun error: %s\n", FP_snd_strerror(res));
			} else {
				LOG(Error, "Unknown error: %s\n", FP_snd_strerror(res));
			}
		}
	}

	FP_snd_pcm_drain(handle);
	FP_snd_pcm_close(handle);

	free(buffer);

	CloseLibrary(alsa);

	return 0;
}
