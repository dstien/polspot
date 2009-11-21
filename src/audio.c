#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "main.h"

audio_t g_audio;

struct audio_pcm {
  struct audio_pcm *next;
  int     channels;
  int     sample_rate;
  int     num_frames;
  int     frames_size;
  int16_t frames[0];
};

static void audio_flush();

static void* audio_loop(void *arg);

void audio_init()
{
  g_audio.head = g_audio.tail = NULL;
  g_audio.buffered_frames = 0;
  g_audio.position = 0;

  g_audio.backend_ctx = audio_backend_init();

  pthread_mutex_init(&g_audio.mutex, NULL);
  pthread_cond_init(&g_audio.cond, NULL);
  pthread_create(&g_audio.thread, NULL, audio_loop, NULL);
}

void audio_cleanup()
{
  pthread_cancel(g_audio.thread);
  pthread_join(g_audio.thread, NULL);
  pthread_cond_destroy(&g_audio.cond);
  pthread_mutex_destroy(&g_audio.mutex);

  audio_backend_cleanup(g_audio.backend_ctx);

  audio_flush();
}

int audio_cb_music_delivery(sp_session *sess, const sp_audioformat *format, const void *frames, int num_frames)
{
  (void)sess;

  if (format->sample_type != SP_SAMPLETYPE_INT16_NATIVE_ENDIAN)
    panic("Unknown sample type (%d)", format->sample_type);

  pthread_mutex_lock(&g_audio.mutex);

  // Audio discontinuity, flush buffer.
  if (!num_frames) {
    audio_flush();
    pthread_mutex_unlock(&g_audio.mutex);
    return 0;
  }

  // Buffer already contains more than 0.25 sec of audio.
  if (g_audio.buffered_frames > (format->sample_rate / 4)) {
    pthread_mutex_unlock(&g_audio.mutex);
    return 0;
  }

  size_t frames_size = num_frames * sizeof(int16_t) * format->channels;
  struct audio_pcm *pcm = malloc(sizeof(struct audio_pcm) + frames_size);
  memcpy(pcm->frames, frames, frames_size);
  pcm->next        = NULL;
  pcm->channels    = format->channels;
  pcm->sample_rate = format->sample_rate;
  pcm->num_frames  = num_frames;
  pcm->frames_size = frames_size;

  if (g_audio.head) {
    if (!g_audio.tail)
      panic("Audio queue chaos (g_audio.head != NULL && g_audio.tail == NULL)");

    g_audio.tail->next = pcm;
  }
  else {
    g_audio.head = pcm;
  }

  g_audio.tail = pcm;
  g_audio.buffered_frames += num_frames;

  pthread_cond_signal(&g_audio.cond);
  pthread_mutex_unlock(&g_audio.mutex);

  return num_frames;
}

int audio_position()
{
  pthread_mutex_lock(&g_audio.mutex);
  int pos = g_audio.position;
  pthread_mutex_unlock(&g_audio.mutex);

  return pos;
}

void audio_clear_position()
{
  pthread_mutex_lock(&g_audio.mutex);
  g_audio.position = 0;
  pthread_mutex_unlock(&g_audio.mutex);
}

static void audio_flush()
{
  while (g_audio.head) {
    struct audio_pcm *pcm = g_audio.head;
    g_audio.head = pcm->next;
    free(pcm);
  }

  g_audio.buffered_frames = 0;

  g_audio.tail = NULL;
}

static void* audio_loop(void *arg)
{
  (void)arg;

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  while (1) {
    pthread_mutex_lock(&g_audio.mutex);

    while (!g_audio.head)
      pthread_cond_wait(&g_audio.cond, &g_audio.mutex);

    struct audio_pcm *pcm = g_audio.head;
    if (pcm->next)
      g_audio.head = pcm->next;
    else
      g_audio.head = g_audio.tail = NULL;

    g_audio.buffered_frames -= pcm->num_frames;
    g_audio.position += pcm->num_frames * 1000 / pcm->sample_rate;

    pthread_mutex_unlock(&g_audio.mutex);

    audio_backend_play(g_audio.backend_ctx, pcm->sample_rate, pcm->channels, pcm->frames, pcm->frames_size);

    free(pcm);
  }

  return NULL;
}
