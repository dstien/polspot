#ifndef POLSPOT_UI_SIDEBAR_H
#define POLSPOT_UI_SIDEBAR_H

#include "ui.h"

void sidebar_init(ui_t *ui);
void sidebar_draw(ui_t *ui);
int  sidebar_keypress(wint_t ch, bool code);

void sidebar_reset();

#endif
