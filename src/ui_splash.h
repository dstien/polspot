/*
 * $Id: ui_splash.h 329 2009-05-31 20:01:31Z dstien $
 *
 */

#ifndef DESPOTIFY_UI_SPLASH_H
#define DESPOTIFY_UI_SPLASH_H

#include "ui.h"

void splash_init(ui_t *ui);
void splash_draw(ui_t *ui);
int  splash_keypress(wint_t ch, bool code);

#endif
