#ifndef POLSPOT_SESSION_H
#define POLSPOT_SESSION_H

#include <event2/event.h>
#include <spotify/api.h>

typedef enum sess_state {
  SESS_OFFLINE,
  SESS_ONLINE,
  SESS_CONNECTING,
  SESS_DISCONNECTING,
  SESS_ERROR
} sess_state_t;

typedef struct sess_search {
  struct sp_search   *res;
  struct sess_search *next;
} sess_search_t;

typedef struct session {
  sp_session *spotify;
  struct event *spot_ev;
  struct event *stop_ev;
  struct event_base *evbase;
  sess_state_t state;
  char *username;
  char *password;

  bool exiting;
  bool playing;
  bool paused;

  sess_search_t *search;
  unsigned int   search_len;

  sp_track *current_track;
} session_t;

void sess_init(struct event_base *evbase);
void sess_cleanup();

void sess_connect();
void sess_disconnect();

void sess_username(const char *username);
void sess_password(const char *password);

void sess_search(const char *query);
void sess_play(sp_track *t);
void sess_stop();
void sess_pause();

#endif
