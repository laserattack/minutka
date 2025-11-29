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
    ERR_DRAW_DIGIT = -1337,
    ERR_TERMINAL_SIZE,
};

typedef struct {
    int x, y;
} Vec2;

typedef struct {
    Vec2 center;
    int font_width, font_height;
    uintattr_t fg, bg;
    char time[9]; /* HH:MM:SS */
} State;

/* global vars */

const char
g_digit_font[256][BASE_FONT_HEIGHT][BASE_FONT_WIDTH+1] = {
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
draw_digit(int digit, int x, int y, uintattr_t fg, uintattr_t bg)
{
    int dx, dy;

    if (digit < 0 || digit > 255)
        return g_last_errno = ERR_DRAW_DIGIT;
    for (dy = 0; dy < g_state.font_height; dy++) {
        for (dx = 0; dx < g_state.font_width; dx++) {
            if (g_digit_font[digit][dy][dx] == '#') {
                tb_set_cell(x+dx, y+dy, ' ', fg, bg);
            }
        }
    }

    return 0;
}

int
draw_screen()
{
    int i;

    /* clear internal buffer */
    tb_clear();
    set_current_time(g_state.time);

    for (i = 0; i < (int)sizeof(g_state.time); ++i) {
        int pos_x, pos_y, step_x;
        uintattr_t fg, bg;

        step_x = g_state.font_width+1;
        pos_x = g_state.center.x+i*step_x;
        pos_y = g_state.center.y;
        fg = g_state.fg;
        bg = g_state.bg;

        if (draw_digit(g_state.time[i], pos_x, pos_y, fg, bg) < 0)
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

    if (check_terminal() < 0)
        return g_last_errno;

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
        if (draw_screen() < 0) break;
        if (handle_event() <= 0) break;
    }
    tb_shutdown();
}

void
print_error()
{
    switch (g_last_errno) {
    case ERR_DRAW_DIGIT:
        printf("[ERROR] Draw digit error\n");
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
