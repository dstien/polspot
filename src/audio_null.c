#include <stddef.h>

void* audio_backend_init()
{
  return NULL;
}

void audio_backend_cleanup(void *ctx)
{
  (void)ctx;
}

void audio_backend_play(void *ctx, int sample_rate, int channels, void *frames, int frames_size)
{
  (void)ctx;
  (void)sample_rate;
  (void)channels;
  (void)frames;
  (void)frames_size;
}
