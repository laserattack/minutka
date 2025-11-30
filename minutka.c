#include <time.h>

#include "fonts.h"
#include "config.h" /* termbox included here */

#define MS_PER_FRAME 1000 / FPS

/* types */

enum errors {
    ERR_DRAW_SYMBOL = -1337,
    ERR_TERMINAL_SIZE,
};

typedef struct {
    int x, y;
} Pos;

typedef struct {
    int width, height;
    uintattr_t fg, bg;
    void* data;
} Font;

typedef struct {
    Pos center;
    Font font;
    char time[9]; /* HH:MM:SS */
} State;

/* global vars */

State
g_state;

int
g_last_errno = 0;

/* funcs */

int
check_terminal()
{
    int width, height;

    width = tb_width();
    height = tb_height();
    if (width < MIN_TERMINAL_WIDTH || height < MIN_TERMINAL_HEIGHT)
        return g_last_errno = ERR_TERMINAL_SIZE;

    return 0;
}

void
set_current_time(char *buffer)
{
    time_t now;
    struct tm *local;

    now = time(NULL);
    local = localtime(&now);
    strftime(buffer, 9, "%H:%M:%S", local);
}

/* Position is the upper-left corner of the symbol's bounding box */
int
draw_symbol(int code, Pos pos, Font font)
{
    int dx, dy, font_h, font_w;

    font_h = font.height;
    font_w = font.width;

    if (code < 0 || code > 255)
        return g_last_errno = ERR_DRAW_SYMBOL;
    for (dy = 0; dy < font_h; dy++) {
        for (dx = 0; dx < font_w; dx++) {
            switch (font_w) {
            case SMALL_FONT_WIDTH: {
                    SmallFontChar *font_data = (SmallFontChar *)font.data;
                    if (font_data[code][dy][dx] == '#') {
                        tb_set_cell(pos.x+dx, pos.y+dy, ' ',
                                font.fg, font.bg);
                    }
                    break;
                }
            case LARGE_FONT_WIDTH: {
                    LargeFontChar *font_data = (LargeFontChar *)font.data;
                    if (font_data[code][dy][dx] == '#') {
                        tb_set_cell(pos.x+dx, pos.y+dy, ' ',
                                font.fg, font.bg);
                    }
                    break;
                }
            }
        }
    }

    return 0;
}

int
draw_screen()
{
    int i, step_x, start_x, start_y, symbols_count, total_width;

    /* clear internal buffer */
    tb_clear();

    symbols_count = sizeof(g_state.time)-1;
    step_x = g_state.font.width+1;
    total_width = step_x*symbols_count-1;

    start_x = g_state.center.x-total_width/2;
    start_y = g_state.center.y-g_state.font.height/2;

    for (i = 0; i < symbols_count; ++i) {
        Pos pos;

        pos = (Pos){
            .x = start_x+i*step_x,
            .y = start_y,
        };

        if (draw_symbol(g_state.time[i], pos, g_state.font) < 0)
            return g_last_errno;
    }

    /* sync internal buffer and terminal */
    tb_present();

    return 0;
}

void
update_sizes()
{
    int width, height;

    width = tb_width();
    height = tb_height();

    g_state.center.x = width/2;
    g_state.center.y = height/2;

    if (width < 110) {
        g_state.font.data = (void*)g_font_small;
        g_state.font.width = SMALL_FONT_WIDTH;
        g_state.font.height = SMALL_FONT_HEIGHT;
    } else {
        g_state.font.data = (void*)g_font_large;
        g_state.font.width = LARGE_FONT_WIDTH;
        g_state.font.height = LARGE_FONT_HEIGHT;
    }
}

int
handle_event()
{
    struct tb_event ev;
    tb_peek_event(&ev, MS_PER_FRAME);

    switch (ev.type) {
    case TB_EVENT_KEY:
        /* firstly check basic symbols */
        switch (ev.ch) {
        case 'q':
            return 0;
        }
        /* secondly check special keys */
        switch (ev.key) {
        case TB_KEY_ESC: /* FALLTHROUGH */
        case TB_KEY_CTRL_C:
            return 0;
        }
        break;
    case TB_EVENT_RESIZE:
        update_sizes();
        break;
    }

    return 1;
}

void
init_state()
{
    g_state.font = (Font){
        .fg = TEXT_COLOR,
        .bg = TEXT_COLOR,
    };
    set_current_time(g_state.time);
    update_sizes();
}

void
main_loop()
{
    tb_init();
    init_state();
    while (1) {
        set_current_time(g_state.time);
        if (draw_screen() < 0) break;
        if (handle_event() <= 0) break;
        if (check_terminal() < 0) break;
    }
    tb_shutdown();
}

void
print_error()
{
    switch (g_last_errno) {
    case ERR_DRAW_SYMBOL:
        printf("[ERROR] Draw symbol error\n");
        break;
    case ERR_TERMINAL_SIZE:
        printf("[ERROR] Bad terminal size\n");
        break;
    }
}

int
main()
{
    main_loop();
    print_error();
    printf("bye bye!\n");
    return 0;
}
