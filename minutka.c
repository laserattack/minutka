#include <time.h>

char
*argv0;

#include "config.h" /* termbox included here */
#include "fonts.h"
#include "arg.h"

#define MS_PER_FRAME 1000 / FPS

#define ERR_FUNCTION(name, should_exit) \
void name(const char *errstr, ...) \
{ \
    va_list ap; \
    va_start(ap, errstr); \
    fprintf(stderr, "[ERROR] "); \
    vfprintf(stderr, errstr, ap); \
    va_end(ap); \
    if (should_exit) exit(1); \
}

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

/* help funcs */

ERR_FUNCTION(err, 0)
ERR_FUNCTION(die, 1)

void
info(const char *str, ...)
{
    va_list ap;

    va_start(ap, str);
    fprintf(stdout, "[INFO] ");
    vfprintf(stdout, str, ap);
    va_end(ap);
}

void
set_current_time(char *buf, size_t bufsize)
{
    time_t now;
    struct tm *local;

    now = time(NULL);
    local = localtime(&now);
    strftime(buf, bufsize, "%H:%M:%S", local);
}

/* global vars */

State
*g_state;

int
g_last_errno = 0;

/* main logic */

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

/* Position is the upper-left corner of the symbol's bounding box */
int
draw_symbol(int code, Pos pos, Font font)
{
    int dx, dy;

    if (code < 0 || code > 255)
        return g_last_errno = ERR_DRAW_SYMBOL;
    for (dy = 0; dy < font.height; dy++) {
        for (dx = 0; dx < font.width; dx++) {
            switch (font.width) {
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

    symbols_count = sizeof(g_state->time)-1;
    step_x = g_state->font.width+1;
    total_width = step_x*symbols_count-1;

    start_x = g_state->center.x-total_width/2;
    start_y = g_state->center.y-g_state->font.height/2;

    for (i = 0; i < symbols_count; ++i) {
        Pos pos;

        pos = (Pos){
            .x = start_x+i*step_x,
            .y = start_y,
        };

        if (draw_symbol(g_state->time[i], pos, g_state->font) < 0)
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

    g_state->center = (Pos){
        .x = width/2,
        .y = height/2,
    };

    if (width < 110) {
        g_state->font = (Font){
            .data = (void *)g_font_small,
            .width = SMALL_FONT_WIDTH,
            .height = SMALL_FONT_HEIGHT,
            .fg = g_state->font.fg,
            .bg = g_state->font.bg,
        };
    } else {
        g_state->font = (Font){
            .data = (void *)g_font_large,
            .width = LARGE_FONT_WIDTH,
            .height = LARGE_FONT_HEIGHT,
            .fg = g_state->font.fg,
            .bg = g_state->font.bg,
        };
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
    if (!(g_state = (State *)malloc(sizeof(State))))
        die("allocation error\n");
    g_state->font = (Font){
        .fg = TEXT_COLOR,
        .bg = TEXT_COLOR,
    };
    set_current_time(g_state->time, 9);
    update_sizes();
}

void
main_loop()
{
    tb_init();
    init_state();
    while (1) {
        set_current_time(g_state->time, 9);
        if (draw_screen() < 0) break;
        if (handle_event() <= 0) break;
        if (check_terminal() < 0) break;
    }
    tb_shutdown();

    /* cleanup */
    if (g_state) free(g_state);
    info("Cleanup done\n");
}

void
handle_error()
{
    switch (g_last_errno) {
    case ERR_DRAW_SYMBOL:
        die("Draw symbol error\n");
        break;
    case ERR_TERMINAL_SIZE:
        die("Bad terminal size\n");
        break;
    }
}

void
usage() {
    die("usage: %s [-h]\n", argv0);
}

int
main(int argc, char *argv[])
{
    ARGBEGIN {
    case 'h':
        usage();
        break;
    default:
        err("unknown flag '%c'\n", ARGC());
        usage();
        break;
    } ARGEND;

    main_loop();
    handle_error();

    info("bye bye!\n");
    return 0;
}
