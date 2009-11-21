#ifndef POLSPOT_UI_H
#define POLSPOT_UI_H

#define _XOPEN_SOURCE_EXTENDED // wchar_t in ncurses.
#include <wchar.h>
#include <ncurses.h>
#include <stdbool.h>
#include <event2/event.h>

#define DSFY_MAX(a, b) ((a) > (b) ? (a) : (b))
#define DSFY_MIN(a, b) ((a) < (b) ? (a) : (b))

#define KEY_ESC 0x1b
#define UI_COLORS 8

enum {
  UI_STYLE_NORMAL = 0,
  UI_STYLE_DIM,
  UI_STYLE_NA
};

typedef enum ui_flags {
  UI_FLAG_OFFSCREEN = 1 << 0,
  UI_FLAG_FOCUS     = 1 << 1,
  UI_FLAG_DIRTY     = 1 << 2
} ui_flags_t;

typedef enum ui_elem {
  UI_SPLASH = 0,
  UI_SIDEBAR,
  UI_TRACKLIST,
  UI_LOG,
  UI_HELP,
  UI_TRACKINFO,
  UI_FOOTER,
  UI_END
} ui_elem_t;

typedef enum ui_set {
  UI_SET_NONE,
  UI_SET_SPLASH,
  UI_SET_BROWSER,
  UI_SET_LOG,
  UI_SET_HELP,
  UI_SET_END
} ui_set_t;

struct ui;
typedef void (*ui_draw_cb_t)(struct ui*);
typedef int  (*ui_keypress_cb_t)(wint_t, bool);

typedef struct ui {
  WINDOW       *win;
  ui_flags_t    flags;
  ui_set_t      set;

  unsigned int  width;
  unsigned int  height;
  unsigned int  fixed_width;
  unsigned int  fixed_height;

  ui_draw_cb_t  draw_cb;
  ui_keypress_cb_t keypress_cb;
} ui_t;

typedef struct screen {
  bool      stdscr_initialized;
  short     colors[UI_COLORS][3];
  ui_t      ui_elements[UI_END];
  ui_elem_t ui_focus;

  struct event *stdin_ev;
  struct event *update_ev;
  struct event *redraw_ev;
  struct event *quit_ev;
} screen_t;

void stdscr_init();
void stdscr_cleanup();

void ui_init(struct event_base *evbase);
void ui_cleanup();

void ui_balance();
void ui_update(bool redraw);
void ui_show(ui_set_t show);
void ui_dirty(ui_elem_t dirty);
void ui_focus(ui_elem_t focus);
ui_elem_t ui_focused();

void ui_input_cb(evutil_socket_t sock, short event, void *arg);
void ui_update_post();
void ui_update_cb(evutil_socket_t sock, short event, void *arg);
void ui_redraw_post();
void ui_redraw_cb(evutil_socket_t sock, short event, void *arg);
void ui_quit_post();
void ui_quit_cb(evutil_socket_t sock, short event, void *arg);

#endif
