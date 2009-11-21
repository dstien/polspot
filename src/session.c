#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "main.h"
#include "session.h"
#include "ui_footer.h"
#include "ui_log.h"
#include "ui_sidebar.h"
#include "user_agent.h"

#include "../key.h"

session_t g_session;

static void sess_stop_cb(evutil_socket_t sock, short event, void *arg);

static void sess_cb_logged_in(sp_session *sess, sp_error err)
{
  if (err == SP_ERROR_OK) {
    g_session.state = SESS_ONLINE;
    log_append("Logged in as \"%s\"", sp_user_display_name(sp_session_user(sess)));
    ui_dirty(UI_FOOTER);
    ui_show(UI_SET_BROWSER);
  }
  else {
    g_session.state = SESS_ERROR;
    log_append(sp_error_message(err));
    ui_dirty(UI_FOOTER);
    ui_show(UI_SET_LOG);
  }
}

static void sess_cb_logged_out(sp_session *sess)
{
  (void)sess;

  g_session.state = SESS_OFFLINE;
  log_append("Logged out");
  ui_dirty(UI_FOOTER);
  ui_update_post();

  if (g_session.exiting)
    sess_cleanup();
}

static void sess_cb_connection_error(sp_session *sess, sp_error err)
{
  (void)sess;
  // TODO: Change state to disconnected/reconnecting?
  log_append("Connection error: %d", err);
}

static void sess_cb_message_to_user(sp_session *sess, const char *message)
{
  (void)sess;

  log_append(message);
}

static void sess_cb_notify(sp_session *sess)
{
  (void)sess;
  struct timeval tv = { 0, 0 };
  evtimer_add(g_session.spot_ev, &tv);
}

static void sess_cb_log(sp_session *sess, const char *data)
{
  (void)sess;

  log_append(data);
}

static void sess_cb_play_token_lost(sp_session *sess)
{
  (void)sess;
  // TODO: Notify user what happened.
  log_append("Play token lost. Playback initiated from another session.");
}

// This callback is called from within a libspotify thread. Post an event
// to be processed in the main program thread.
static void sess_cb_end_of_track(sp_session *sess)
{
  (void)sess;
  struct timeval tv = { 0, 250000 }; // 0.25 sec buffer.
  evtimer_add(g_session.stop_ev, &tv);
}

static void sess_cb_search_complete_cb(sp_search *res, void *data)
{
  (void)data;

  log_append("Search result: %d/%d", sp_search_num_tracks(res), sp_search_total_tracks(res));
  ui_dirty(UI_SIDEBAR);
  ui_dirty(UI_TRACKLIST);
  ui_update_post();
}

static sp_session_callbacks g_callbacks = {
  .logged_in          = sess_cb_logged_in,
  .logged_out         = sess_cb_logged_out,
  .metadata_updated   = NULL,
  .connection_error   = sess_cb_connection_error,
  .message_to_user    = sess_cb_message_to_user,
  .notify_main_thread = sess_cb_notify,
  .music_delivery     = audio_cb_music_delivery,
  .play_token_lost    = sess_cb_play_token_lost,
  .log_message        = sess_cb_log,
  .end_of_track       = sess_cb_end_of_track
};

// Process Spotify events in main event loop.
void sess_event_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;

  session_t *sess = arg;

  int timeout = 0;
  while (!timeout)
    sp_session_process_events(sess->spotify, &timeout);

  // Schedule next event.
  struct timeval tv;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;
  evtimer_add(sess->spot_ev, &tv);
}

void sess_init(struct event_base *evbase)
{
  g_session.evbase  = evbase;
  g_session.spot_ev = evtimer_new(evbase, sess_event_cb, &g_session);
  g_session.stop_ev = evtimer_new(evbase, sess_stop_cb, &g_session);
  g_session.exiting = false;
  g_session.current_track = NULL;

  sp_session_config config = {
    .api_version          = SPOTIFY_API_VERSION,
    .cache_location       = "tmp",
    .settings_location    = "tmp",
    .application_key      = g_appkey,
    .application_key_size = g_appkey_size,
    .user_agent           = POLSPOT_USER_AGENT,
    .callbacks            = &g_callbacks,
    .userdata             = &g_session
  };

  sp_error err = sp_session_init(&config, &g_session.spotify);

  if (err != SP_ERROR_OK)
    panic("sp_session_init() failed: %s", sp_error_message(err));
}

void sess_cleanup()
{
  // If user is logged in then initiate logout. This will change the session
  // state to SESS_DISCONNECTING. Another call to this function will thus exit
  // the main loop without waiting for the logout process to finish, allowing
  // users to avoid waiting for timeouts by issuing the quit commant twice.
  if (g_session.state == SESS_ONLINE) {
    sess_disconnect();
    g_session.exiting = true;
  }
  else {
    free(g_session.username);
    g_session.username = 0;
    free(g_session.password);
    g_session.password = 0;

    // Free search results.
    for (sess_search_t *s = g_session.search; s;) {
      sp_search_release(s->res);
      sess_search_t *p = s;
      s = s->next;
      free(p);
    }

    g_session.current_track = NULL;
    event_free(g_session.spot_ev);

    // Exit main event loop.
    event_base_loopbreak(g_session.evbase);
  }
}

void sess_connect()
{
  assert(g_session.username && g_session.password);

  sess_disconnect();

  // Login with credentials set by sess_username/sess_password.
  sp_error err = sp_session_login(g_session.spotify, g_session.username, g_session.password);

  if (err != SP_ERROR_OK)
    panic("sp_session_login() failed: %s", sp_error_message(err));

  log_append("Connecting...");

  // Redraw status info.
  g_session.state = SESS_CONNECTING;
  ui_dirty(UI_FOOTER);
  ui_update_post();
}

void sess_disconnect()
{
  if (g_session.state == SESS_ONLINE) {
    sess_stop();

     sp_error err = sp_session_logout(g_session.spotify);

     if (err != SP_ERROR_OK)
       panic("sp_session_logout() failed: %s", sp_error_message(err));

    log_append("Disconnecting...");
    g_session.state = SESS_DISCONNECTING;

    // Return to splash screen.
    ui_show(UI_SET_SPLASH);
  }

  // Redraw status info.
  ui_dirty(UI_FOOTER);
  ui_update_post();
}

void sess_username(const char *username)
{
  free(g_session.username);
  g_session.username = strdup(username);

  // Prompt for password.
  footer_input(INPUT_PASSWORD);
}

void sess_password(const char *password)
{
  free(g_session.password);
  g_session.password = strdup(password);

  sess_connect();
}

void sess_search(const char *query)
{
  if (g_session.state != SESS_ONLINE) {
    log_append("Not connected");
    return;
  }

  log_append("Searching for: <%s>", query);
  sp_search *res = sp_search_create(g_session.spotify, query,
      0, 200, // tracks
      0,   0, // albums
      0,   0, // artists
      sess_cb_search_complete_cb, NULL);

  if (!res)
    panic("sp_search_create() failed");

  sess_search_t *prev = g_session.search;
  sess_search_t *search = malloc(sizeof(sess_search_t));
  search->res  = res;
  search->next = prev;
  g_session.search = search;
  ++g_session.search_len;

  sidebar_reset();
}

// Start playback.
void sess_play(sp_track *t)
{
  if (g_session.state != SESS_ONLINE) {
    log_append("Not connected");
    return;
  }

  if (!t || !sp_track_is_available(t) || !sp_track_is_loaded(t)) {
    log_append("Track not available");
    return;
  }

  sess_stop();

  g_session.current_track = t;

  log_append("Playing \"%s\"...", sp_track_name(t));

  sp_error err = sp_session_player_load(g_session.spotify, t);
  if (err != SP_ERROR_OK)
    panic("sp_session_player_load() failed: %s", sp_error_message(err));

  err = sp_session_player_play(g_session.spotify, true);
  if (err != SP_ERROR_OK)
    panic("sp_session_player_play() failed: %s", sp_error_message(err));

  g_session.playing = true;

  // Redraw track info.
  ui_dirty(UI_TRACKINFO);
  ui_update_post();
}

// Stop playback.
void sess_stop()
{
  if (g_session.state != SESS_ONLINE || !g_session.current_track || !sp_track_is_loaded(g_session.current_track))
    return;

  sp_session_player_unload(g_session.spotify);

  g_session.current_track = NULL;
  g_session.playing = false;
  g_session.paused = false;

  log_append("Stopped");

  // Redraw track info.
  ui_dirty(UI_TRACKINFO);
  ui_update_post();
}

static void sess_stop_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;
  (void)arg;

  sess_stop();
}

// Toggle playback pause.
void sess_pause()
{
  if (g_session.state != SESS_ONLINE || !g_session.playing || !g_session.current_track || !sp_track_is_loaded(g_session.current_track))
    return;

  g_session.paused = !g_session.paused;
  sp_error err = sp_session_player_play(g_session.spotify, !g_session.paused);
  if (err != SP_ERROR_OK)
    panic("sp_session_player_play() failed: %s", sp_error_message(err));


  if (g_session.paused)
    log_append("Paused");
  else
    log_append("Resuming");
}
