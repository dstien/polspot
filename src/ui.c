#include <sys/ioctl.h>

#include "commands.h"
#include "main.h"
#include "session.h"
#include "ui.h"
#include "ui_footer.h"
#include "ui_help.h"
#include "ui_log.h"
#include "ui_sidebar.h"
#include "ui_splash.h"
#include "ui_trackinfo.h"
#include "ui_tracklist.h"
#include "ui_trackprogress.h"

#define UI_FOREACH_START_END(uis, var, start, end) for (ui_t *var = &uis[start], *_i = (ui_t*)start; \
                                                   (long)_i != end; _i = (ui_t*)((long)_i + 1), \
                                                   var = &uis[(long)_i])
#define UI_FOREACH_START(uis, var, start)          UI_FOREACH_START_END(uis, var, start, UI_END)
#define UI_FOREACH(uis, var)                       UI_FOREACH_START_END(uis, var, 0,     UI_END)

screen_t g_screen;

extern session_t g_session;

// Ncurses initialization.
void stdscr_init()
{
  if (g_screen.stdscr_initialized)
    return;

  initscr();

  if (has_colors()
      && start_color() == OK
      && use_default_colors() == OK) {
    // Define some more comfortable colors if supported.
    if (can_change_color()) {
      // Backup current definitions.
      for (int i = 0; i < UI_COLORS; ++i) {
        color_content(i, &g_screen.colors[i][0], &g_screen.colors[i][1], &g_screen.colors[i][2]);
      }

      init_color(COLOR_BLACK, 256, 256, 256); // Dark gray
      init_color(COLOR_RED,   384,   0,   0); // Dark red
    }

    init_pair(UI_STYLE_NORMAL, -1, -1);
    init_pair(UI_STYLE_DIM, COLOR_BLACK, -1);
    init_pair(UI_STYLE_NA, COLOR_RED, -1);
  }

  raw();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);

  g_screen.stdscr_initialized = true;
}

// Ncurses cleanup.
void stdscr_cleanup()
{
  if (!g_screen.stdscr_initialized)
    return;

  // Reset redefined colors.
  if (can_change_color()) {
    for (int i = 0; i < UI_COLORS; ++i) {
      init_color(i, g_screen.colors[i][0], g_screen.colors[i][1], g_screen.colors[i][2]);
    }
  }

  noraw();
  endwin();

  g_screen.stdscr_initialized = false;
}

// Initialize UI elements.
void ui_init(struct event_base *evbase)
{
  g_screen.stdscr_initialized = false;
  g_screen.stdin_ev = event_new(evbase, /*STDIN_FILENO*/ 0, EV_READ | EV_PERSIST, ui_input_cb, NULL);
  event_add(g_screen.stdin_ev, NULL);

  g_screen.update_ev = evtimer_new(evbase, ui_update_cb, NULL);
  g_screen.redraw_ev = evtimer_new(evbase, ui_redraw_cb, NULL);
  g_screen.quit_ev   = evtimer_new(evbase, ui_quit_cb,   NULL);

  stdscr_init();

  splash_init(&g_screen.ui_elements[UI_SPLASH]);
  sidebar_init(&g_screen.ui_elements[UI_SIDEBAR]);
  tracklist_init(&g_screen.ui_elements[UI_TRACKLIST]);
  log_init(&g_screen.ui_elements[UI_LOG]);
  help_init(&g_screen.ui_elements[UI_HELP]);
  trackinfo_init(&g_screen.ui_elements[UI_TRACKINFO]);
  trackprogress_init(&g_screen.ui_elements[UI_TRACKPROGRESS]);
  footer_init(&g_screen.ui_elements[UI_FOOTER]);

  ui_show(UI_SET_BROWSER);
  ui_redraw_post();
}

// Cleanup UI elements.
void ui_cleanup()
{
  stdscr_cleanup();

  UI_FOREACH(g_screen.ui_elements, ui) {
    delwin(ui->win);
    ui->win = 0;
  }

  event_free(g_screen.stdin_ev);
  event_free(g_screen.update_ev);
  event_free(g_screen.redraw_ev);
  event_free(g_screen.quit_ev);
}

// Resize and stack UI elements on available screen space. Dynamic-height
// elements are sized after static-height ones. Multiple dynamic-height
// elements are combined in horizontal sets.
void ui_balance()
{
  struct winsize ws;
  ioctl(0, TIOCGWINSZ, &ws);
  resizeterm(ws.ws_row, ws.ws_col);
  unsigned int scrwidth = ws.ws_col, scrheight = ws.ws_row;

  // Set size for fixed-height elements.
  UI_FOREACH(g_screen.ui_elements, ui) {
    if (ui->fixed_height) {
      ui->width = scrwidth;
      ui->height = DSFY_MIN(ui->fixed_height, scrheight);
      scrheight -= ui->height;
    }
  }

  // Set remaining height to dynamic-height elements.
  UI_FOREACH(g_screen.ui_elements, ui)
    if (!ui->fixed_height)
      ui->height = scrheight;

  // Balance widths in horizontal sets for dynamic-height elements.
  unsigned int setwidth[UI_SET_END];
  for (ui_set_t set = UI_SET_NONE + 1; set < UI_SET_END; ++set)
    setwidth[set] = scrwidth;

  // Set width for fixed-width elements.
  UI_FOREACH(g_screen.ui_elements, ui) {
    if (ui->set > UI_SET_NONE && ui->fixed_width) {
      ui->width = DSFY_MIN(ui->fixed_width, setwidth[ui->set]);
      setwidth[ui->set] -= ui->width;
    }
  }

  // Set remaining width to dynamic-width elements.
  UI_FOREACH(g_screen.ui_elements, ui) {
    if (ui->set > UI_SET_NONE && !ui->fixed_width)
      ui->width = setwidth[ui->set];
  }

  // Stack elements. Set all dynamic elements at same y-position, only one
  // horizontal set will be visible at a time.
  bool dyny_set = false;
  unsigned int scry = 0,
               dyny = 0,
               setx[UI_SET_END] = { };
  UI_FOREACH(g_screen.ui_elements, ui) {
    // Resize and mark dirty.
    wresize(ui->win, ui->height, ui->width);
    ui->flags |= UI_FLAG_DIRTY;

    // Move.
    if (ui->fixed_height) {
      mvwin(ui->win, scry, 0);
      scry += ui->height;
    }
    else {
      if (!dyny_set) {
        dyny = scry;
        scry += ui->height;
        dyny_set = true;
      }
      mvwin(ui->win, dyny, setx[ui->set]);
      setx[ui->set] += ui->width;
    }
  }

  // Full draw.
  ui_update(true);
}

// Update all UI elements that are visible and marked dirty. If the redraw
// argument is set the dirty flag is ignored and a full redraw is performed
// by flushing ncurses' internal cache.
void ui_update(bool redraw)
{
  if (redraw)
    redrawwin(stdscr);

  wnoutrefresh(stdscr);

  UI_FOREACH(g_screen.ui_elements, ui) {
    if (!(ui->flags & UI_FLAG_OFFSCREEN)
        && ((ui->flags & UI_FLAG_DIRTY) || redraw)
        && ui->height && ui->width) {
      ui->flags &= ~UI_FLAG_DIRTY;
      werase(ui->win);
      ui->draw_cb(ui);

      if (redraw)
        redrawwin(ui->win);

      wnoutrefresh(ui->win);
    }
  }

  doupdate();
}

// Show and focus a UI element set. Elements in current visible set will be disabled.
void ui_show(ui_set_t show)
{
  // Default to splash screen when disconnected.
  if (show == UI_SET_BROWSER && g_session.state != SESS_ONLINE)
    show = UI_SET_SPLASH;

  bool focused = false;

  for (ui_elem_t i = 0; i < UI_END; ++i) {
    if (g_screen.ui_elements[i].set == show) {
      // Focus only first element in set.
      if (!focused) {
        g_screen.ui_elements[i].flags |= UI_FLAG_FOCUS;
        g_screen.ui_focus = i;
        focused = true;
      }
      g_screen.ui_elements[i].flags |= UI_FLAG_DIRTY;
      g_screen.ui_elements[i].flags &= ~UI_FLAG_OFFSCREEN;
    }
    else if (g_screen.ui_elements[i].set != UI_SET_NONE) {
      g_screen.ui_elements[i].flags &= ~(UI_FLAG_FOCUS | UI_FLAG_DIRTY);
      g_screen.ui_elements[i].flags |= UI_FLAG_OFFSCREEN;
    }
  }

  ui_update_post(0);
}

// Mark UI element as dirty. Will cause redraw of the element at next ui_update().
void ui_dirty(ui_elem_t dirty)
{
  g_screen.ui_elements[dirty].flags |= UI_FLAG_DIRTY;
}

// Move focus to given UI element. Focused element receives keypresses and
// focus may affect how highlighted content is drawn.
void ui_focus(ui_elem_t focus)
{
  // Don't focus offscreen UI elements or elements that can't handle keypresses.
  if ((g_screen.ui_elements[focus].flags & UI_FLAG_OFFSCREEN)
      || !g_screen.ui_elements[focus].keypress_cb)
    return;

  for (ui_elem_t i = 0; i < UI_END; ++i) {
    if (i == focus) {
      g_screen.ui_elements[i].flags |= (UI_FLAG_FOCUS | UI_FLAG_DIRTY);
      g_screen.ui_focus = focus;
    }
    else if (g_screen.ui_elements[i].flags & UI_FLAG_FOCUS) {
      g_screen.ui_elements[i].flags &= ~UI_FLAG_FOCUS;
      g_screen.ui_elements[i].flags |= UI_FLAG_DIRTY;
    }
  }

  ui_update_post(0);
}

// Return currently focused UI element.
ui_elem_t ui_focused()
{
  return g_screen.ui_focus;
}

// Handle keyboard bindings and redirect keypresses to focused UI element.
void ui_input_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;
  (void)arg;

  wint_t ch;
  int type;

  while ((type = get_wch(&ch)) != ERR) {
    // Very special keys.
    switch (ch) {
      // Quit program.
      case 'Q' - '@':
        ui_quit_post();
        continue;

      // Force screen redraw.
      case 'L' - '@':
        ui_redraw_post();
        continue;
    }

    // Forward regular keys to focused element.
    ch = g_screen.ui_elements[g_screen.ui_focus].keypress_cb(ch, type == KEY_CODE_YES);

    if (!ch)
      continue;

    // Check remaining keys. Text input will override most of these.
    switch (ch) {
      case '?':
      case 'h':
      case KEY_F(1):
        ui_show(UI_SET_HELP);
        continue;

      case ':':
        footer_input(INPUT_COMMAND);
        continue;

      case '/':
        footer_input(INPUT_SEARCH);
        continue;

      case 'H' - '@':
      case 127:
      case KEY_BACKSPACE:
        cmd_cb_stop();
        continue;

      case ' ':
        cmd_cb_pause();
        continue;

      case '|':
        ui_show(UI_SET_LOG);
        continue;

      case 'E' - '@':
        cmd_cb_connect();
        continue;

      case 'W' - '@':
        cmd_cb_disconnect();
        continue;
    }
  }
}

// Queue update event to apply on-screen changes.
// Delay in milliseconds.
void ui_update_post(int delay)
{
  struct timeval tv = { 0, delay * 1000 };
  evtimer_add(g_screen.update_ev, &tv);
}

// Callback from event handler.
void ui_update_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;
  (void)arg;

  ui_update(false);
}

// Queue redraw event to force full screen redraw.
void ui_redraw_post()
{
  struct timeval tv = { 0, 0 };
  evtimer_add(g_screen.redraw_ev, &tv);
}

// Callback from event handler.
void ui_redraw_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;
  (void)arg;

  ui_balance();
}

// Callback from SIGWINCH handler. Post a delayed redraw request to prevent
// excessive redrawing during continuous window resizing.
void ui_winch_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;
  (void)arg;

  struct timeval tv = { 0, 500000 };
  evtimer_add(g_screen.redraw_ev, &tv);
}

// Queue application quit event.
void ui_quit_post()
{
  struct timeval tv = { 0, 0 };
  evtimer_add(g_screen.quit_ev, &tv);
}

// Callback from event handler.
void ui_quit_cb(evutil_socket_t sock, short event, void *arg)
{
  (void)sock;
  (void)event;
  (void)arg;

  sess_cleanup();
}
