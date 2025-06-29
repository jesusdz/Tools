#include "tools.h"

// This example reads standard from input and writes
// to the default PCM device for 5 seconds of data.

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
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_TIME(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_SIZE(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
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
SND_PCM_HW_PARAMS_GET_PERIOD_TIME* FP_snd_pcm_hw_params_get_period_time;
SND_PCM_HW_PARAMS_GET_PERIOD_SIZE* FP_snd_pcm_hw_params_get_period_size;
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
	FP_snd_pcm_hw_params_get_period_time = (SND_PCM_HW_PARAMS_GET_PERIOD_TIME*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_time");
	FP_snd_pcm_hw_params_get_period_size = (SND_PCM_HW_PARAMS_GET_PERIOD_SIZE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_size");
	FP_snd_pcm_writei = (SND_PCM_WRITEI*) LoadSymbol(alsa, "snd_pcm_writei");
	FP_snd_pcm_prepare = (SND_PCM_PREPARE*) LoadSymbol(alsa, "snd_pcm_prepare");
	FP_snd_pcm_close = (SND_PCM_CLOSE*) LoadSymbol(alsa, "snd_pcm_close");
	FP_snd_pcm_drain = (SND_PCM_DRAIN*) LoadSymbol(alsa, "snd_pcm_drain");

	int res; // results
	int dir; // direction of approximate values

	// Open PCM device
	snd_pcm_t *handle;
	res = FP_snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (res < 0) {
		LOG(Error, "unable to open pcm device: %s\n", FP_snd_strerror(res));
		return 1;
	}

	unsigned int sampleRate = 44100; // bits/second (CD quality)
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

	// Use a buffer large enough to hold one period
	snd_pcm_uframes_t periodSamplesPerChannel;
	FP_snd_pcm_hw_params_get_period_size(params, &periodSamplesPerChannel, &dir);
	const int size = channelCount * periodSamplesPerChannel * bytesPerSample;
	char * buffer = (char *) malloc(size);

	// We want to loop for 5 seconds
	unsigned int periodTime;
	FP_snd_pcm_hw_params_get_period_time(params, &periodTime, &dir);

	// 5 seconds in microseconds divided by period time
	long loops = 5000000 / periodTime;

#define USE_SIGNAL 1
#if USE_SIGNAL
	long stereoIteration = 0;
	const u32 toneHz = 256;
	const u32 toneVolume = 1000;
	const u32 tonePeriodUs = 1000000 / toneHz;
	const u32 tonePeriodIterations = tonePeriodUs * periodSamplesPerChannel / periodTime ;
	const u32 toneHalfPeriodIterations = tonePeriodIterations / 2;
#endif

	while (loops > 0)
	{
		loops--;

#if USE_SIGNAL
		u16 *samplePtr = (u16*)buffer;
		for (u32 i = 0; i < periodSamplesPerChannel; ++i)
		{
			const bool up = stereoIteration++ / toneHalfPeriodIterations % 2;
			const u16 sample = up ? toneVolume : -toneVolume;
			*samplePtr++ = sample;
			*samplePtr++ = sample;
		}
#else
		res = read(0, buffer, size);

		if (res == 0) {
			LOG(Error, "end of file on input\n");
			break;
		} else if (res != size) {
			LOG(Error, "short read: read %d bytes\n", res);
		}
#endif

		res = FP_snd_pcm_writei(handle, buffer, frames);

		if (res == -EPIPE) {
			// EPIPE means underrun
			LOG(Error, "underrun occurred\n");
			FP_snd_pcm_prepare(handle);
		} else if (res < 0) {
			LOG(Error, "error from writei: %s\n", FP_snd_strerror(res));
		}  else if (res != (int)frames) {
			LOG(Error, "short write, write %d frames\n", res);
		}
	}

	FP_snd_pcm_drain(handle);
	FP_snd_pcm_close(handle);

	free(buffer);

	CloseLibrary(alsa);

	return 0;
}
