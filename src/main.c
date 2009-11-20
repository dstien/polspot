#include <locale.h>
#include <signal.h>
#include <stdlib.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "audio.h"
#include "session.h"
#include "ui.h"

void panic(const char *fmt, ...)
{
  stdscr_cleanup();

  fprintf(stderr, "Fatal error: ");

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n");

  abort();
}

static void sigsegv_cb(evutil_socket_t fd, short event, void *arg)
{
  (void)fd;
  (void)event;
  (void)arg;

  // Avoid infinite loop.
  signal(SIGINT, SIG_IGN);
  panic("Ouch, I segfaulted. How embarrassing. :-(\n");
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  setlocale(LC_ALL, "");

  // Initialize libevent.
  if (evthread_use_pthreads())
    panic("libevent built without pthreads.");

  struct event_config *evconf = event_config_new();
  event_config_require_features(evconf, EV_FEATURE_FDS);

  struct event_base *evbase = event_base_new_with_config(evconf);
  if (!evbase)
    panic("No suitable libevent backend found.");

  // Signal handlers.
  struct event *sigint_ev   = evsignal_new(evbase, SIGINT,   ui_quit_cb,   evbase);
  struct event *sigterm_ev  = evsignal_new(evbase, SIGTERM,  ui_quit_cb,   evbase);
  struct event *sigsegv_ev  = evsignal_new(evbase, SIGSEGV,  sigsegv_cb,   NULL);
  struct event *sigwinch_ev = evsignal_new(evbase, SIGWINCH, ui_redraw_cb, NULL);

  evsignal_add(sigint_ev,   NULL);
  evsignal_add(sigterm_ev,  NULL);
  evsignal_add(sigsegv_ev,  NULL);
  evsignal_add(sigwinch_ev, NULL);

  // Initialize audio.
  audio_init();

  // Initialize Spotify session.
  sess_init(evbase);

  // Initialize UI.
  ui_init(evbase);

  // Main loop.
  // sess_cleanup() must be called inside the main loop in order to process
  // events for graceful disconnecting.
  int ret = event_base_dispatch(evbase);

  // Cleanup UI.
  ui_cleanup();

  // Cleanup audio.
  audio_cleanup();

  // Cleanup signal handlers.
  event_free(sigint_ev);
  event_free(sigterm_ev);
  event_free(sigsegv_ev);
  event_free(sigwinch_ev);

  // Cleanup libevent.
  event_base_free(evbase);
  event_config_free(evconf);

  return ret;
}
