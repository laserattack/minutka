#ifndef CONFIG_H
#define CONFIG_H

#define TB_IMPL
#include "termbox2.h"

/*
TB_DEFAULT
TB_BLACK
TB_RED
TB_GREEN
TB_YELLOW
TB_BLUE
TB_MAGENTA
TB_CYAN
TB_WHITE
*/
#define TEXT_COLOR           TB_BLUE

/*
frames per second
*/
#define FPS                  1

/*
large font <-> small font change
*/
#define FONT_CHANGE_WIDTH    110

/*
program exit when min size reached
*/
#define MIN_TERMINAL_WIDTH   10
#define MIN_TERMINAL_HEIGHT  10

#endif
