#include <ao/ao.h>

#include "audio.h"
#include "main.h"

struct audio_backend_ao {
  ao_sample_format format;
  ao_device        *device;
  int              driver_id;
};

void* audio_backend_init()
{
  struct audio_backend_ao *ao_ctx = malloc(sizeof(struct audio_backend_ao));

  ao_ctx->format.bits        = 16;
  ao_ctx->format.rate        = 0;
  ao_ctx->format.channels    = 0;
  ao_ctx->format.byte_format = AO_FMT_LITTLE;
  ao_ctx->device = NULL;

  ao_initialize();

  ao_ctx->driver_id = ao_default_driver_id();
  if (ao_ctx->driver_id == -1)
    panic("ao_default_driver_id() failed");

  return ao_ctx;
}

void audio_backend_cleanup(void *ctx)
{
  struct audio_backend_ao *ao_ctx = ctx;

  if (ao_ctx->device && !ao_close(ao_ctx->device))
    panic("ao_close() failed");

  ao_shutdown();
}

void audio_backend_play(void *ctx, int sample_rate, int channels, void *frames, int frames_size)
{
  struct audio_backend_ao *ao_ctx = ctx;

  if (!ao_ctx->device || ao_ctx->format.channels != channels || ao_ctx->format.rate != sample_rate) {
    if (ao_ctx->device)
      if (!ao_close(ao_ctx->device))
        panic("ao_close() failed");

    ao_ctx->format.rate     = sample_rate;
    ao_ctx->format.channels = channels;

    ao_ctx->device = ao_open_live(ao_ctx->driver_id, &ao_ctx->format, NULL);
    if (!ao_ctx->device)
      panic("ao_open_live() failed");
  }

  if (!ao_play(ao_ctx->device, frames, frames_size))
    panic("ao_play() failed");
}
