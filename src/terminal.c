#define _POSIX_C_SOURCE 200809L

#include "terminal.h"

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios saved_termios;
static bool active = false;

static void handle_signal(int sig) {
    term_leave_game_mode();
    _exit(128 + sig);
}

bool term_enter_game_mode(void) {
    if (tcgetattr(STDIN_FILENO, &saved_termios) == -1) {
        perror("tcgetattr");
        return false;
    }

    struct termios raw = saved_termios;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= CS8;
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return false;
    }

    active = true;
    atexit(term_leave_game_mode);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    printf("\033[?1049h\033[?25l\033[2J\033[H");
    fflush(stdout);
    return true;
}

void term_leave_game_mode(void) {
    if (!active) {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
    printf("\033[0m\033[?25h\033[?1049l");
    fflush(stdout);
    active = false;
}

static bool input_ready(void) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    struct timeval tv = {0, 0};
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

static int read_byte(void) {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    return n == 1 ? c : -1;
}

TermInput term_read_input(void) {
    if (!input_ready()) {
        return IN_NONE;
    }

    int c = read_byte();
    if (c < 0) {
        return IN_NONE;
    }

    if (c == '\033') {
        int c1 = read_byte();
        int c2 = read_byte();
        if (c1 == '[') {
            switch (c2) {
                case 'A': return IN_ROTATE_CW;
                case 'B': return IN_DOWN;
                case 'C': return IN_RIGHT;
                case 'D': return IN_LEFT;
                default: return IN_NONE;
            }
        }
        return IN_NONE;
    }

    if (c == ' ') {
        return IN_HARD_DROP;
    }

    switch (tolower(c)) {
        case 'q': return IN_QUIT;
        case 'a': return IN_LEFT;
        case 'd': return IN_RIGHT;
        case 's': return IN_DOWN;
        case 'w':
        case 'x': return IN_ROTATE_CW;
        case 'z': return IN_ROTATE_CCW;
        case 'c': return IN_HOLD;
        case 'p': return IN_PAUSE;
        case 'r': return IN_RESTART;
        default: return IN_NONE;
    }
}

uint64_t term_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

void term_sleep_ms(unsigned ms) {
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000u);
    req.tv_nsec = (long)(ms % 1000u) * 1000000L;
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        ;
    }
}

bool term_size_too_small(int min_cols, int min_rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return false;
    }
    return ws.ws_col < min_cols || ws.ws_row < min_rows;
}
