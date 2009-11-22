#include "audio.h"
#include "session.h"
#include "ui_trackprogress.h"

extern session_t g_session;

void trackprogress_init(ui_t *ui)
{
  ui->win          = newwin(0, 0, 0, 0);
  ui->flags        = 0;
  ui->set          = UI_SET_NONE;
  ui->fixed_width  = 0;
  ui->fixed_height = 1;
  ui->draw_cb      = trackprogress_draw;
  ui->keypress_cb  = 0;
}

void trackprogress_draw(ui_t *ui)
{
  sp_track *trk = g_session.current_track;

  if (trk) {
    wbkgd(ui->win, ' '); // Reset background.

    int len = sp_track_duration(trk);
    int pos = audio_position();

    mvwprintw(ui->win, 0, 0, "%2d:%02d ", pos / 60000, pos % 60000 / 1000);

    if (ui->width > 28) {
      int wdt = ui->width - 10;
      wchar_t bar[wdt];
      bar[0]       = L'\u2590'; // '▐'
      bar[wdt - 1] = L'\u258C'; // '▌'

      int cur = (pos * (wdt - 2)) / len;

      for (int i = 1; i < wdt - 1; ++i)
        if (i <= cur)
          bar[i] =  L'\u2588'; // '█'
        else
          bar[i] =  L'\u2591'; // '░'

      mvwaddnwstr(ui->win, 0, 5, bar, len);
    }

    if (ui->width > 10)
      mvwprintw(ui->win, 0, ui->width - 5, "%2d:%02d", len / 60000, len % 60000 / 1000);

    if (!g_session.paused) {
      ui_dirty(UI_TRACKPROGRESS);
      ui_update_post(500);
    }
  }
  else {
    wbkgd(ui->win, '.'); // Grayed out background.
  }
}
