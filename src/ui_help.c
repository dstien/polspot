#include "ui_help.h"

#define HELP_HEIGHT 18
#define HELP_WIDTH  48

void help_init(ui_t *ui)
{
  ui->win          = newwin(0, 0, 0, 0);
  ui->flags        = 0;
  ui->set          = UI_SET_HELP;
  ui->fixed_width  = 0;
  ui->fixed_height = 0;
  ui->draw_cb      = help_draw;
  ui->keypress_cb  = help_keypress;
}

void help_draw(ui_t *ui)
{
  // TODO: Print arguments.
  int line = DSFY_MAX((ui->height - HELP_HEIGHT) / 2, 0);
  int x = DSFY_MAX((ui->width - HELP_WIDTH) / 2, 0);

  mvwprintw(ui->win,   line, x, "Key        Command       Description");
  mvwchgat(ui->win,    line, x, -1, A_BOLD, UI_STYLE_DIM, NULL);

  mvwprintw(ui->win, ++line, x, ":                        Open command input");
  mvwprintw(ui->win, ++line, x, "^D                       Cancel input");
  ++line;
  mvwprintw(ui->win, ++line, x, "^E         c/connect     (Re)connect to Spotify");
  mvwprintw(ui->win, ++line, x, "^W         d/disconnect  Disconnect from Spotify");
  mvwprintw(ui->win, ++line, x, "/          s/search      Search");
  ++line;
  mvwprintw(ui->win, ++line, x, "Enter                    Play highlighted track");
  mvwprintw(ui->win, ++line, x, "Backspace  t/stop        Stop playback");
  mvwprintw(ui->win, ++line, x, "Space      p/pause       Toggle playback pause");
  ++line;
  mvwprintw(ui->win, ++line, x, "|          l/log         Display log");
  mvwprintw(ui->win, ++line, x, "F1         ?/h/help      Display this text");
  mvwprintw(ui->win, ++line, x, "Esc        m/main        Return to main screen");
  ++line;
  mvwprintw(ui->win, ++line, x, "^L         r/redraw      Force screen redraw");
  mvwprintw(ui->win, ++line, x, "^Q         q/quit        Quit");

#ifdef LIBSPOTIFY
  // Try our best to comply with libspotify TOS 10.2.
  line += 2;
  x += 3;
  mvwprintw(ui->win, ++line, x, "This product uses SPOTIFY® CORE but is not endorsed,");
  --x;
  mvwchgat(ui->win, line, x, 54, A_BOLD, UI_STYLE_DIM, NULL);
  mvwprintw(ui->win, ++line, x, "certified or otherwise approved in any way by Spotify.");
  mvwchgat(ui->win, line, x, 54, A_BOLD, UI_STYLE_DIM, NULL);
  x -= 2;
  mvwprintw(ui->win, ++line, x, "Spotify is the registered trade mark of the Spotify Group.");
  mvwchgat(ui->win, line, x, 58, A_BOLD, UI_STYLE_DIM, NULL);
#endif

}

int help_keypress(wint_t ch, bool code)
{
  (void)code;

  // TODO: Scrolling...
  switch (ch) {
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_ESC:
    case 'D' - '@':
      ui_show(UI_SET_BROWSER);
      return 0;

    default:
      return ch;
  }
}
