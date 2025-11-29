#include <time.h>

#define TB_IMPL
#include "termbox2.h"

#define FPS                  24
#define MS_PER_FRAME         1000 / FPS
#define MIN_TERMINAL_WIDTH   10
#define MIN_TERMINAL_HEIGHT  10
#define BASE_FONT_WIDTH      3
#define BASE_FONT_HEIGHT     5

/* types */

enum errors {
    ERR_DRAW_SYMBOL = -1337,
    ERR_TERMINAL_SIZE,
};

typedef struct {
    int x, y;
} Pos;

typedef struct {
    Pos center;
    int font_width, font_height;
    uintattr_t fg, bg;
    char time[9]; /* HH:MM:SS */
} State;

/* global vars */

const char
g_font[256][BASE_FONT_HEIGHT][BASE_FONT_WIDTH+1] = {
    ['0'] = { "###", "# #", "# #", "# #", "###" }, // 0
    ['1'] = { " # ", "## ", " # ", " # ", "###" }, // 1
    ['2'] = { "###", "  #", "###", "#  ", "###" }, // 2
    ['3'] = { "###", "  #", "## ", "  #", "###" }, // 3
    ['4'] = { "# #", "# #", "###", "  #", "  #" }, // 4
    ['5'] = { "###", "#  ", "###", "  #", "###" }, // 5
    ['6'] = { "###", "#  ", "###", "# #", "###" }, // 6
    ['7'] = { "###", "  #", "  #", "  #", "  #" }, // 7
    ['8'] = { "###", "# #", "###", "# #", "###" }, // 8
    ['9'] = { "###", "# #", "###", "  #", "###" }, // 9
    [':'] = { "   ", " # ", "   ", " # ", "   " }, // :
};

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

int
draw_symbol(int code, Pos pos, uintattr_t fg, uintattr_t bg)
{
    int dx, dy;

    if (code < 0 || code > 255)
        return g_last_errno = ERR_DRAW_SYMBOL;
    for (dy = 0; dy < g_state.font_height; dy++) {
        for (dx = 0; dx < g_state.font_width; dx++) {
            if (g_font[code][dy][dx] == '#') {
                tb_set_cell(pos.x+dx, pos.y+dy, ' ', fg, bg);
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

    symbols_count = strlen(g_state.time);
    step_x = g_state.font_width+1;
    total_width = step_x*strlen(g_state.time)-1;

    start_x = g_state.center.x-total_width/2;
    start_y = g_state.center.y-g_state.font_height/2;

    for (i = 0; i < symbols_count; ++i) {
        Pos pos;

        pos = (Pos){
            .x = start_x+i*step_x,
            .y = start_y,
        };

        if (draw_symbol(g_state.time[i],
                    pos, g_state.fg, g_state.bg) < 0)
            return g_last_errno;
    }

    /* sync internal buffer and terminal */
    tb_present();

    return 0;
}

void
update_sizes()
{
    g_state.center.x = tb_width() / 2;
    g_state.center.y = tb_height() / 2;
    g_state.font_width = 3;
    g_state.font_height = 5;
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
    g_state = (State){
        .fg = TB_BLUE,
        .bg = TB_BLUE,
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
