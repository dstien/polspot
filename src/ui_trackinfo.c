#include "session.h"
#include "ui_trackinfo.h"

extern session_t g_session;

void trackinfo_init(ui_t *ui)
{
  ui->win          = newwin(0, 0, 0, 0);
  ui->flags        = 0;
  ui->set          = UI_SET_NONE;
  ui->fixed_width  = 0;
  ui->fixed_height = 2;
  ui->draw_cb      = trackinfo_draw;
  ui->keypress_cb  = 0;
}

void trackinfo_draw(ui_t *ui)
{
  sp_track *trk = g_session.current_track;

  if (trk) {
    wbkgd(ui->win, ' '); // Reset background.

    sp_album *alb = sp_track_album(trk);
    sp_artist *art = sp_track_artist(trk, 0); // TODO: Get all artists.

    wchar_t line1[ui->width], line2[ui->width];
    swprintf(line1, sizeof(line1), L"%s - %s", sp_artist_name(art), sp_track_name(trk));
    swprintf(line2, sizeof(line2), L"%s (%d)", sp_album_name(alb), sp_album_year(alb));
    mvwaddnwstr(ui->win, 0, DSFY_MAX(((signed)ui->width - (signed)wcslen(line1)) / 2, 0), line1, ui->width);
    mvwaddnwstr(ui->win, 1, DSFY_MAX(((signed)ui->width - (signed)wcslen(line2)) / 2, 0), line2, ui->width);
  }
  else {
    wbkgd(ui->win, '.'); // Grayed out background.
  }
}
