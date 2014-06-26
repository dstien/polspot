#ifndef POLSPOT_AUDIO_H
#define POLSPOT_AUDIO_H

#include <pthread.h>
#include <libspotify/api.h>

typedef struct audio {
  pthread_t       thread;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;

  struct audio_pcm *head;
  struct audio_pcm *tail;
  int              buffered_frames;
  int              position;

  void             *backend_ctx;
} audio_t;

void* audio_backend_init();
void audio_backend_cleanup(void *ctx);
void audio_backend_play(void *ctx, int sample_rate, int channels, void *frames, int frames_size);

void audio_init();
void audio_cleanup();

int audio_cb_music_delivery(sp_session *sess, const sp_audioformat *format, const void *frames, int num_frames);
int audio_position();
void audio_clear_position();

#endif
