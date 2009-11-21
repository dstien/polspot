#include "session.h"
#include "ui_log.h"
#include "ui_tracklist.h"

extern session_t g_session;

// Cursor position.
static int g_pos = 0;

// Available height for list (needed for PGDN/PGUP keys).
static int g_availy = 10;

// Current search result.
static struct sp_search *g_res = 0;

void tracklist_init(ui_t *ui)
{
  ui->win          = newwin(0, 0, 0, 0);
  ui->flags        = 0;
  ui->set          = UI_SET_BROWSER;
  ui->fixed_width  = 0;
  ui->fixed_height = 0;
  ui->draw_cb      = tracklist_draw;
  ui->keypress_cb  = tracklist_keypress;
}

// Print tracks in search result.
void tracklist_draw(ui_t *ui)
{
  int i = 0, line = 0;
  g_availy = ui->height - 2;

  // Title/artist columns width.
  int slen = (ui->width - 4 - 6 - 1) / 2;

  mvwprintw(ui->win, 0, 0, "%3s %-*.*s %-*.*sLength", "#",
      slen, slen, "Title", slen, slen, "Artist");
  mvwchgat(ui->win, 0, 0, -1, A_BOLD, UI_STYLE_DIM, NULL);

  if (!g_res || !sp_search_is_loaded(g_res))
    return;

  if (sp_search_num_tracks(g_res)) {
    // Scroll offset.
    i = DSFY_MAX(DSFY_MIN(DSFY_MAX(g_pos - (g_availy / 2), 0),
        sp_search_num_tracks(g_res) - g_availy), 0);

    sp_track *t = sp_search_track(g_res, i + line);
    for (; t && line < g_availy; t = sp_search_track(g_res, i + line)) {
      // Concat list of artists.
      wchar_t art[slen];
      int len = 0;

      for (int ai = 0; ai < sp_track_num_artists(t) && len < slen; ++ai) {
        sp_artist *a = sp_track_artist(t, 0);
        len += swprintf(art + len, slen - len, L"%s%s", ai ? "/" : "", sp_artist_name(a));
      }

      wchar_t str[ui->width];
      swprintf(str, sizeof(str), L"%3d %-*.*s %-*.*ls %2d:%02d",
          i + line + 1, slen, slen, sp_track_name(t), slen, slen, art,
          sp_track_duration(t) / 60000, sp_track_duration(t) % 60000 / 1000);
      mvwaddnwstr(ui->win, line + 1, 0, str, ui->width);

      if (i + line == g_pos)
        mvwchgat(ui->win, line + 1, 0, -1,
            (ui->flags & UI_FLAG_FOCUS ? A_REVERSE : A_BOLD),
            (sp_track_is_available(t) ? UI_STYLE_NORMAL : UI_STYLE_NA), NULL);

      ++line;
    }
  }

  // Additional info at bottom.
  mvwprintw(ui->win, ui->height - 1, 0, "Query: <%s> Hits: %d/%d",
      sp_search_query(g_res),
      sp_search_num_tracks(g_res),
      sp_search_total_tracks(g_res));
  mvwchgat(ui->win, ui->height - 1, 0, -1, A_BOLD, UI_STYLE_DIM, NULL);
}

int tracklist_keypress(wint_t ch, bool code)
{
  (void)code;

  // Change focus back to sidebar.
  if (ch == KEY_LEFT || ch == 'h') {
    ui_focus(UI_SIDEBAR);
    return 0;
  }

  // Nothing left to do if we've got an empty list.
  if (!g_res || !sp_search_is_loaded(g_res) || !sp_search_num_tracks(g_res))
    return ch;

  //struct track *t;

  switch (ch) {
    case KEY_ENTER:
    case '\n':
    case '\r':
      sess_play(sp_search_track(g_res, g_pos));
      break;

    case KEY_UP:
    case 'k':
      g_pos = DSFY_MAX(g_pos - 1, 0);
      break;

    case KEY_DOWN:
    case 'j':
      g_pos = DSFY_MIN(g_pos + 1, sp_search_num_tracks(g_res) - 1);
      break;

    case KEY_HOME:
      g_pos = 0;
      break;

    case KEY_END:
      g_pos = sp_search_num_tracks(g_res) - 1;
      break;

    case KEY_PPAGE:
      g_pos = DSFY_MAX(g_pos - g_availy, 0);
      break;

    case KEY_NPAGE:
      g_pos = DSFY_MIN(g_pos + g_availy, sp_search_num_tracks(g_res) - 1);
      break;

    default:
      return ch;
  }

  ui_dirty(UI_TRACKLIST);
  ui_update_post();

  return 0;
}

// Set search result for displaying.
void tracklist_set(int pos, bool focus)
{
  // Find search.
  sess_search_t *s = g_session.search;
  for (int i = 0; s && i != pos; ++i, s = s->next);

  if (g_res != s->res) {
    g_res = s->res;
    g_pos = 0;
  }

  if (focus)
    ui_focus(UI_TRACKLIST);
  else
    ui_dirty(UI_TRACKLIST);

  ui_update_post();
}
