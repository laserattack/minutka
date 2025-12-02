#include <time.h>
#include <ctype.h>

char
*argv0;

#include "config.h" /* termbox included here */
#include "font.h"
#include "arg.h"

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
    int w, h;
    uintattr_t fg, bg;
    void* data;
} Font;

typedef struct {
    char mode;
    Font font;
    Pos center;
    time_t curtime, starttime, endtime;
} State;

/* help funcs */

void
die(const char *errstr, ...)
{
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(1);
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
    int w, h;

    w = tb_width();
    h = tb_height();

    if (w < MIN_TERMINAL_WIDTH || h < MIN_TERMINAL_HEIGHT)
        return g_last_errno = ERR_TERMINAL_SIZE;

    return 0;
}

/* Position is the upper-left corner of the symbol's bounding box */
int
draw_symbol(int code, Pos *pos, Font *font)
{
    int dx, dy;

    if (code < 0 || code > 255)
        return g_last_errno = ERR_DRAW_SYMBOL;
    for (dy = 0; dy < font->h; dy++) {
        for (dx = 0; dx < font->w; dx++) {
            switch (font->w) {
            case SMALL_FONT_WIDTH: {
                    SmallFontChar *fontdata;

                    fontdata = (SmallFontChar *)font->data;
                    if (fontdata[code][dy][dx] == '#')
                        tb_set_cell(pos->x+dx, pos->y+dy, ' ',
                                font->fg, font->bg);
                    break;
                }
            case LARGE_FONT_WIDTH: {
                    LargeFontChar *fontdata;

                    fontdata = (LargeFontChar *)font->data;
                    if (fontdata[code][dy][dx] == '#')
                        tb_set_cell(pos->x+dx, pos->y+dy, ' ',
                                font->fg, font->bg);
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
    int i, textw, stepx, startx, starty, symcount;
    struct tm *local;
    char time[9];

    /* clear internal buffer */
    tb_clear();

    switch (g_state->mode) {
    case 'c':
        local = localtime(&g_state->curtime);
        strftime(time, sizeof(time), "%H:%M:%S", local);
        break;
    case 't':
        int secs, mins, hours;

        secs = difftime(g_state->endtime, g_state->curtime);
        secs = (secs<0)? 0 : secs;
        mins = secs/60;
        hours = mins/60;
        snprintf(time, sizeof(time),
                "%02d:%02d:%02d", hours%100, mins%60, secs%60);
        break;
    default:
        die("[ERROR] unknown mode");
    }

    symcount = sizeof(time)-1;
    stepx = g_state->font.w+1;
    textw = stepx*symcount-1;

    startx = g_state->center.x-textw/2;
    starty = g_state->center.y-g_state->font.h/2;

    for (i = 0; i < symcount; ++i) {
        Pos pos;

        pos = (Pos){ .x = startx+i*stepx, .y = starty };
        if (draw_symbol(time[i], &pos, &g_state->font) < 0)
            return g_last_errno;
    }

    /* sync internal buffer and terminal */
    tb_present();

    return 0;
}

void
update_sizes()
{
    int w, h;

    w = tb_width();
    h = tb_height();
    g_state->center = (Pos){ .x = w/2, .y = h/2 };

    if (w < FONT_CHANGE_WIDTH)
        g_state->font = (Font){
            .data = (void *)g_font_small,
            .w = SMALL_FONT_WIDTH,
            .h = SMALL_FONT_HEIGHT,
            .fg = g_state->font.fg,
            .bg = g_state->font.bg,
        };
    else
        g_state->font = (Font){
            .data = (void *)g_font_large,
            .w = LARGE_FONT_WIDTH,
            .h = LARGE_FONT_HEIGHT,
            .fg = g_state->font.fg,
            .bg = g_state->font.bg,
        };
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
main_loop()
{
    tb_init();
    update_sizes();
    while (1) {
        g_state->curtime = time(NULL);
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
        die("[ERROR] draw symbol error\n");
        break;
    case ERR_TERMINAL_SIZE:
        die("[ERROR] bad terminal size\n");
        break;
    }
}

void
usage() {
    die("[INFO] usage: %s [-h] [[-c] | [-t sec]]\n", argv0);
}

int
main(int argc, char *argv[])
{
    int startmode, timertime;

    //  TODO: -e for exit when 00:00:00 reached in timer mode
    ARGBEGIN {
    case 'h':
        usage();
        break;
    case 'c':
        if (startmode) {
            printf("[ERROR] its not possible to run in"
                    " more than one mode\n");
            usage();
        }
        startmode = ARGC();
        break;
    case 't':
        char *c, *time;

        if (startmode) {
            printf("[ERROR] its not possible to run in"
                    " more than one mode\n");
            usage();
        }
        time = ARGF();
        startmode = ARGC();
        if (time == NULL) {
            printf("[ERROR] required argument after flag '%c'\n", ARGC());
            usage();
        }
        if (strlen(time) > 9) {
            printf("[ERROR] arg after flag '%c'"
                    " too big (>9 symbols)\n", ARGC());
            usage();
        }
        for (c = time; *c; c++) {
            if (!isdigit(*c)) {
                printf("[ERROR] arg after flag '%c' must be an integer"
                        ", but got '%s'\n", ARGC(), time);
                usage();
            }
        }
        timertime = atoi(time);
        break;
    default:
        printf("[ERROR] unknown flag '%c'\n", ARGC());
        usage();
        break;
    } ARGEND;

    if (argc != 0) {
        printf("[ERROR] invalid args count: %d\n", argc);
        usage();
    }

    if (!startmode) {
        printf("[ERROR] start mode is not specified\n");
        usage();
    }
    
    printf("[INFO] starting in '%c' mode...\n", startmode);

    /* init start state */
    if (!(g_state = (State *)malloc(sizeof(State))))
        die("[ERROR] init state allocation error\n");
    g_state->mode = startmode;
    g_state->starttime = time(NULL);
    g_state->endtime = time(NULL) + timertime;
    g_state->font = (Font){ .fg = TEXT_COLOR, .bg = TEXT_COLOR };

    main_loop();

    if (g_state) free(g_state);
    printf("[INFO] cleanup done\n");

    print_error();

    return 0;
}
