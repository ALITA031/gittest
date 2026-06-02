#define _POSIX_C_SOURCE 200809L

/*
 * Terminal Tetris in pure C using ANSI escape sequences + POSIX termios.
 * No external graphics libraries required.
 *
 * Controls:
 *   A/D or ←/→ : move left/right
 *   S or ↓     : soft drop
 *   W/X or ↑   : rotate clockwise
 *   Z          : rotate counter-clockwise
 *   Space      : hard drop
 *   P          : pause/resume
 *   R          : restart after game over
 *   Q          : quit
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define BOARD_W 10
#define BOARD_H 20
#define PIECE_TYPES 7
#define BAG_SIZE 7
#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int type;
    int rot;
    int x;
    int y;
} Piece;

typedef struct {
    int cells[BOARD_H][BOARD_W];
    Piece current;
    Piece next;
    int bag[BAG_SIZE];
    int bag_index;
    unsigned score;
    unsigned lines;
    unsigned level;
    bool game_over;
    bool paused;
} Game;

typedef enum {
    KEY_NONE = 0,
    KEY_QUIT,
    KEY_LEFT_MOVE,
    KEY_RIGHT_MOVE,
    KEY_SOFT_DROP,
    KEY_ROTATE_CW,
    KEY_ROTATE_CCW,
    KEY_HARD_DROP,
    KEY_PAUSE,
    KEY_RESTART
} InputKey;

static const Point SHAPES[PIECE_TYPES][4][4] = {
    // I
    {
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}}
    },
    // O
    {
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}}
    },
    // T
    {
        {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {1, 2}}
    },
    // S
    {
        {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}}
    },
    // Z
    {
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {0, 2}}
    },
    // J
    {
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 0}, {1, 1}, {0, 2}, {1, 2}}
    },
    // L
    {
        {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}}
    }
};

static const char *PIECE_NAMES[PIECE_TYPES] = {"I", "O", "T", "S", "Z", "J", "L"};
static const char *PIECE_COLORS[PIECE_TYPES] = {
    "\033[36m", // I cyan
    "\033[33m", // O yellow
    "\033[35m", // T magenta
    "\033[32m", // S green
    "\033[31m", // Z red
    "\033[34m", // J blue
    "\033[37m"  // L white
};

static struct termios original_termios;
static bool terminal_restored = false;

static void restore_terminal(void) {
    if (!terminal_restored) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
        printf("\033[?25h\033[0m\033[?1049l");
        fflush(stdout);
        terminal_restored = true;
    }
}

static void handle_signal(int sig) {
    restore_terminal();
    _exit(128 + sig);
}

static void enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    atexit(restore_terminal);

    struct termios raw = original_termios;
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= CS8;
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    printf("\033[?1049h\033[?25l\033[2J");
    fflush(stdout);
}

static uint64_t monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static void sleep_ms(unsigned ms) {
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000);
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        ;
    }
}

static bool input_available(void) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    struct timeval tv = {0, 0};
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

static int read_byte(void) {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1) {
        return c;
    }
    return -1;
}

static InputKey read_key(void) {
    if (!input_available()) {
        return KEY_NONE;
    }

    int c = read_byte();
    if (c == -1) {
        return KEY_NONE;
    }

    if (c == '\033') {
        int c1 = read_byte();
        int c2 = read_byte();
        if (c1 == '[') {
            switch (c2) {
                case 'A': return KEY_ROTATE_CW;   // Up
                case 'B': return KEY_SOFT_DROP;   // Down
                case 'C': return KEY_RIGHT_MOVE;  // Right
                case 'D': return KEY_LEFT_MOVE;   // Left
                default: break;
            }
        }
        return KEY_NONE;
    }

    if (c == ' ') {
        return KEY_HARD_DROP;
    }

    int lower = tolower(c);
    switch (lower) {
        case 'q': return KEY_QUIT;
        case 'a': return KEY_LEFT_MOVE;
        case 'd': return KEY_RIGHT_MOVE;
        case 's': return KEY_SOFT_DROP;
        case 'w':
        case 'x': return KEY_ROTATE_CW;
        case 'z': return KEY_ROTATE_CCW;
        case 'p': return KEY_PAUSE;
        case 'r': return KEY_RESTART;
        default: return KEY_NONE;
    }
}

static bool terminal_too_small(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        return false;
    }
    return ws.ws_row < 24 || ws.ws_col < 62;
}

static void shuffle_bag(Game *g) {
    for (int i = 0; i < BAG_SIZE; ++i) {
        g->bag[i] = i;
    }
    for (int i = BAG_SIZE - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = g->bag[i];
        g->bag[i] = g->bag[j];
        g->bag[j] = tmp;
    }
    g->bag_index = 0;
}

static int next_type(Game *g) {
    if (g->bag_index >= BAG_SIZE) {
        shuffle_bag(g);
    }
    return g->bag[g->bag_index++];
}

static Piece make_piece(int type) {
    Piece p = {type, 0, BOARD_W / 2 - 2, -1};
    return p;
}

static bool collides(const Game *g, Piece p) {
    const Point *shape = SHAPES[p.type][p.rot];
    for (int i = 0; i < 4; ++i) {
        int x = p.x + shape[i].x;
        int y = p.y + shape[i].y;
        if (x < 0 || x >= BOARD_W || y >= BOARD_H) {
            return true;
        }
        if (y >= 0 && g->cells[y][x] != 0) {
            return true;
        }
    }
    return false;
}

static bool try_move(Game *g, int dx, int dy) {
    Piece moved = g->current;
    moved.x += dx;
    moved.y += dy;
    if (!collides(g, moved)) {
        g->current = moved;
        return true;
    }
    return false;
}

static bool try_rotate(Game *g, int dir) {
    Piece rotated = g->current;
    rotated.rot = (rotated.rot + dir + 4) % 4;

    static const int kicks[] = {0, -1, 1, -2, 2};
    for (int i = 0; i < ARRAY_LEN(kicks); ++i) {
        Piece candidate = rotated;
        candidate.x += kicks[i];
        if (!collides(g, candidate)) {
            g->current = candidate;
            return true;
        }
    }
    return false;
}

static void lock_piece(Game *g) {
    const Point *shape = SHAPES[g->current.type][g->current.rot];
    for (int i = 0; i < 4; ++i) {
        int x = g->current.x + shape[i].x;
        int y = g->current.y + shape[i].y;
        if (y < 0) {
            g->game_over = true;
            continue;
        }
        if (x >= 0 && x < BOARD_W && y >= 0 && y < BOARD_H) {
            g->cells[y][x] = g->current.type + 1;
        }
    }
}

static unsigned clear_lines(Game *g) {
    unsigned cleared = 0;
    for (int y = BOARD_H - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < BOARD_W; ++x) {
            if (g->cells[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            ++cleared;
            for (int yy = y; yy > 0; --yy) {
                memcpy(g->cells[yy], g->cells[yy - 1], sizeof(g->cells[yy]));
            }
            memset(g->cells[0], 0, sizeof(g->cells[0]));
            ++y;
        }
    }
    return cleared;
}

static void add_score(Game *g, unsigned cleared, bool hard_drop, unsigned drop_cells) {
    static const unsigned scores[] = {0, 100, 300, 500, 800};
    if (cleared < ARRAY_LEN(scores)) {
        g->score += scores[cleared] * g->level;
    }
    if (hard_drop) {
        g->score += drop_cells * 2;
    }
    g->lines += cleared;
    g->level = g->lines / 10 + 1;
}

static void spawn_piece(Game *g) {
    g->current = g->next;
    g->next = make_piece(next_type(g));
    if (collides(g, g->current)) {
        g->game_over = true;
    }
}

static void init_game(Game *g) {
    memset(g, 0, sizeof(*g));
    g->score = 0;
    g->lines = 0;
    g->level = 1;
    g->game_over = false;
    g->paused = false;
    shuffle_bag(g);
    g->current = make_piece(next_type(g));
    g->next = make_piece(next_type(g));
}

static unsigned gravity_interval_ms(const Game *g) {
    unsigned base = 720;
    unsigned decay = (g->level > 1) ? (g->level - 1) * 55 : 0;
    return decay >= 620 ? 100 : base - decay;
}

static Piece ghost_piece(const Game *g) {
    Piece ghost = g->current;
    while (!collides(g, ghost)) {
        ghost.y++;
    }
    ghost.y--;
    return ghost;
}

static bool piece_occupies(Piece p, int x, int y) {
    const Point *shape = SHAPES[p.type][p.rot];
    for (int i = 0; i < 4; ++i) {
        if (p.x + shape[i].x == x && p.y + shape[i].y == y) {
            return true;
        }
    }
    return false;
}

static void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

static void print_cell(int type, bool ghost) {
    if (ghost) {
        printf("\033[2m::\033[0m");
    } else if (type > 0) {
        printf("%s[]\033[0m", PIECE_COLORS[type - 1]);
    } else {
        printf("  ");
    }
}

static void draw_preview(int top, int left, Piece p) {
    for (int y = 0; y < 4; ++y) {
        move_cursor(top + y, left);
        printf("        ");
    }

    const Point *shape = SHAPES[p.type][0];
    for (int i = 0; i < 4; ++i) {
        move_cursor(top + shape[i].y, left + shape[i].x * 2);
        printf("%s[]\033[0m", PIECE_COLORS[p.type]);
    }
}

static void draw_game(const Game *g) {
    const int top = 2;
    const int left = 3;
    const int side = left + BOARD_W * 2 + 7;
    Piece ghost = ghost_piece(g);

    printf("\033[H");
    printf("\033[0m");
    printf("\033[2K");
    move_cursor(1, left);
    printf("\033[1mTerminal Tetris / 俄罗斯方块\033[0m");

    for (int y = -1; y <= BOARD_H; ++y) {
        move_cursor(top + y + 1, left - 2);
        if (y == BOARD_H) {
            printf("+");
            for (int x = 0; x < BOARD_W * 2; ++x) {
                printf("-");
            }
            printf("+");
            continue;
        }
        printf("|");
        if (y >= 0) {
            for (int x = 0; x < BOARD_W; ++x) {
                bool active = piece_occupies(g->current, x, y);
                bool ghost_here = piece_occupies(ghost, x, y) && !active;
                if (active) {
                    print_cell(g->current.type + 1, false);
                } else if (g->cells[y][x] != 0) {
                    print_cell(g->cells[y][x], false);
                } else if (ghost_here) {
                    print_cell(0, true);
                } else {
                    print_cell(0, false);
                }
            }
        } else {
            for (int x = 0; x < BOARD_W; ++x) {
                print_cell(0, false);
            }
        }
        printf("|");
    }

    move_cursor(top + 1, side);
    printf("Next: %s        ", PIECE_NAMES[g->next.type]);
    draw_preview(top + 3, side, g->next);

    move_cursor(top + 9, side);
    printf("Score: %-10u", g->score);
    move_cursor(top + 10, side);
    printf("Lines: %-10u", g->lines);
    move_cursor(top + 11, side);
    printf("Level: %-10u", g->level);

    move_cursor(top + 13, side);
    printf("Controls");
    move_cursor(top + 14, side);
    printf("A/D or arrows: move   ");
    move_cursor(top + 15, side);
    printf("S/down: soft drop     ");
    move_cursor(top + 16, side);
    printf("W/X/up: rotate        ");
    move_cursor(top + 17, side);
    printf("Z: rotate back        ");
    move_cursor(top + 18, side);
    printf("Space: hard drop      ");
    move_cursor(top + 19, side);
    printf("P: pause  Q: quit     ");

    if (g->paused) {
        move_cursor(top + 9, left + 4);
        printf("\033[1m PAUSED \033[0m");
    }

    if (g->game_over) {
        move_cursor(top + 8, left + 2);
        printf("\033[1m GAME OVER \033[0m");
        move_cursor(top + 10, left + 1);
        printf(" R restart ");
        move_cursor(top + 11, left + 1);
        printf(" Q quit    ");
    }

    fflush(stdout);
}

static void settle_current_piece(Game *g, bool hard_drop, unsigned drop_cells) {
    lock_piece(g);
    if (!g->game_over) {
        unsigned cleared = clear_lines(g);
        add_score(g, cleared, hard_drop, drop_cells);
        spawn_piece(g);
    }
}

static void hard_drop(Game *g) {
    unsigned cells = 0;
    while (try_move(g, 0, 1)) {
        ++cells;
    }
    settle_current_piece(g, true, cells);
}

static void handle_key(Game *g, InputKey key, uint64_t *last_fall) {
    if (key == KEY_NONE) {
        return;
    }
    if (key == KEY_QUIT) {
        restore_terminal();
        exit(EXIT_SUCCESS);
    }
    if (g->game_over) {
        if (key == KEY_RESTART) {
            init_game(g);
            *last_fall = monotonic_ms();
        }
        return;
    }
    if (key == KEY_PAUSE) {
        g->paused = !g->paused;
        *last_fall = monotonic_ms();
        return;
    }
    if (g->paused) {
        return;
    }

    switch (key) {
        case KEY_LEFT_MOVE:
            try_move(g, -1, 0);
            break;
        case KEY_RIGHT_MOVE:
            try_move(g, 1, 0);
            break;
        case KEY_SOFT_DROP:
            if (try_move(g, 0, 1)) {
                g->score += 1;
            }
            break;
        case KEY_ROTATE_CW:
            try_rotate(g, 1);
            break;
        case KEY_ROTATE_CCW:
            try_rotate(g, -1);
            break;
        case KEY_HARD_DROP:
            hard_drop(g);
            *last_fall = monotonic_ms();
            break;
        default:
            break;
    }
}

int main(void) {
    srand((unsigned)time(NULL));
    enable_raw_mode();

    if (terminal_too_small()) {
        restore_terminal();
        fprintf(stderr, "Terminal is too small. Please use at least 62x24.\n");
        return EXIT_FAILURE;
    }

    Game game;
    init_game(&game);
    uint64_t last_fall = monotonic_ms();

    while (true) {
        InputKey key;
        while ((key = read_key()) != KEY_NONE) {
            handle_key(&game, key, &last_fall);
        }

        uint64_t now = monotonic_ms();
        if (!game.game_over && !game.paused && now - last_fall >= gravity_interval_ms(&game)) {
            if (!try_move(&game, 0, 1)) {
                settle_current_piece(&game, false, 0);
            }
            last_fall = now;
        }

        draw_game(&game);
        sleep_ms(16);
    }
}
