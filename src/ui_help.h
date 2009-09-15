/*
 * $Id: ui_help.h 329 2009-05-31 20:01:31Z dstien $
 *
 */

#ifndef DESPOTIFY_UI_HELP_H
#define DESPOTIFY_UI_HELP_H

#include "ui.h"

void help_init(ui_t *ui);
void help_draw(ui_t *ui);
int  help_keypress(wint_t ch, bool code);

#endif
