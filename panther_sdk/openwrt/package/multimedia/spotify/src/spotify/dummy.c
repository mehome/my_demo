#include "dummy.h"

static SpError dummy_audio_init(void) { return kSpErrorOk; }
static SpError dummy_audio_flush(void) { return kSpErrorOk; }
static size_t dummy_audio_data(const int16_t *samples, size_t sample_count,
			       const struct SpSampleFormat *sample_format,
			       uint32_t *samples_buffered, void *context)
{
	return 0;
}
static void dummy_audio_pause(int pause) {}
static void dummy_audio_volume(int vol) {}
static void dummy_audio_changed(int duration_ms) {}
static uint32_t dummy_get_buffered_ms(void) { return 0; }
static void dummy_reset_written_frames(void) {}
static uint32_t dummy_get_written_frames(void) { return 0; }

void SpExampleAudioGetDummyCallbacks(struct SpExampleAudioCallbacks *callbacks)
{
	callbacks->audio_init		      = dummy_audio_init;
	callbacks->audio_flush		      = dummy_audio_flush;
	callbacks->audio_data		      = dummy_audio_data;
	callbacks->audio_pause		      = dummy_audio_pause;
	callbacks->audio_volume		      = dummy_audio_volume;
	callbacks->audio_changed	      = dummy_audio_changed;
	callbacks->audio_get_buffered_ms      = dummy_get_buffered_ms;
	callbacks->audio_reset_written_frames = dummy_reset_written_frames;
	callbacks->audio_get_written_frames   = dummy_get_written_frames;
}
