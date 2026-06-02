#include "tetris.h"

#include <stddef.h>
#include <string.h>

static const TgPoint SHAPES[TG_PIECE_COUNT][4][4] = {
    [TG_I] = {
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}}
    },
    [TG_O] = {
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}}
    },
    [TG_T] = {
        {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {1, 2}}
    },
    [TG_S] = {
        {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}}
    },
    [TG_Z] = {
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {0, 2}}
    },
    [TG_J] = {
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 0}, {1, 1}, {0, 2}, {1, 2}}
    },
    [TG_L] = {
        {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}}
    }
};

static const TgPoint KICKS_JLSTZ[4][4][5] = {
    [0][1] = {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
    [1][0] = {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
    [1][2] = {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
    [2][1] = {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
    [2][3] = {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    [3][2] = {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
    [3][0] = {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
    [0][3] = {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}
};

static const TgPoint KICKS_I[4][4][5] = {
    [0][1] = {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
    [1][0] = {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
    [1][2] = {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
    [2][1] = {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
    [2][3] = {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
    [3][2] = {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
    [3][0] = {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
    [0][3] = {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}
};

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    if (x == 0) {
        x = 0x9E3779B9u;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void shuffle_bag(TgGame *g) {
    for (int i = 0; i < TG_BAG_SIZE; ++i) {
        g->bag[i] = (TgPieceType)i;
    }
    for (int i = TG_BAG_SIZE - 1; i > 0; --i) {
        int j = (int)(xorshift32(&g->rng) % (uint32_t)(i + 1));
        TgPieceType tmp = g->bag[i];
        g->bag[i] = g->bag[j];
        g->bag[j] = tmp;
    }
    g->bag_index = 0;
}

static TgPieceType pop_bag(TgGame *g) {
    if (g->bag_index >= TG_BAG_SIZE) {
        shuffle_bag(g);
    }
    return g->bag[g->bag_index++];
}

static void refill_queue(TgGame *g) {
    for (int i = 0; i < TG_NEXT_COUNT; ++i) {
        if (g->next[i] == TG_NO_PIECE) {
            g->next[i] = pop_bag(g);
        }
    }
}

static TgPieceType pop_queue(TgGame *g) {
    TgPieceType type = g->next[0];
    for (int i = 0; i < TG_NEXT_COUNT - 1; ++i) {
        g->next[i] = g->next[i + 1];
    }
    g->next[TG_NEXT_COUNT - 1] = pop_bag(g);
    return type;
}

static TgPiece make_piece(TgPieceType type) {
    TgPiece p;
    p.type = type;
    p.rotation = 0;
    p.x = 3;
    p.y = 0;
    return p;
}

static void reset_piece_timers(TgGame *g) {
    g->fall_accumulator_ms = 0;
    g->lock_timer_ms = 0;
    g->lock_resets = 0;
    g->hold_used = false;
    g->last_action_was_rotate = false;
}

static int cell_is_occupied_or_wall(const TgGame *g, int x, int y) {
    if (x < 0 || x >= TG_COLS || y >= TG_ROWS) {
        return 1;
    }
    if (y < 0) {
        return 0;
    }
    return g->board[y][x] != 0;
}

static bool is_t_spin(const TgGame *g, TgPiece piece) {
    if (piece.type != TG_T || !g->last_action_was_rotate) {
        return false;
    }

    int cx = piece.x + 1;
    int cy = piece.y + 1;
    int occupied = 0;
    occupied += cell_is_occupied_or_wall(g, cx - 1, cy - 1);
    occupied += cell_is_occupied_or_wall(g, cx + 1, cy - 1);
    occupied += cell_is_occupied_or_wall(g, cx - 1, cy + 1);
    occupied += cell_is_occupied_or_wall(g, cx + 1, cy + 1);
    return occupied >= 3;
}

static TgClearKind classify_clear(unsigned lines, bool t_spin) {
    if (t_spin) {
        switch (lines) {
            case 0: return TG_CLEAR_T_SPIN_ZERO;
            case 1: return TG_CLEAR_T_SPIN_SINGLE;
            case 2: return TG_CLEAR_T_SPIN_DOUBLE;
            default: return TG_CLEAR_T_SPIN_TRIPLE;
        }
    }
    switch (lines) {
        case 0: return TG_CLEAR_NONE;
        case 1: return TG_CLEAR_SINGLE;
        case 2: return TG_CLEAR_DOUBLE;
        case 3: return TG_CLEAR_TRIPLE;
        default: return TG_CLEAR_TETRIS;
    }
}

static bool is_back_to_back_clear(TgClearKind kind) {
    return kind == TG_CLEAR_TETRIS ||
           kind == TG_CLEAR_T_SPIN_SINGLE ||
           kind == TG_CLEAR_T_SPIN_DOUBLE ||
           kind == TG_CLEAR_T_SPIN_TRIPLE;
}

static unsigned base_score(TgClearKind kind) {
    switch (kind) {
        case TG_CLEAR_SINGLE: return 100;
        case TG_CLEAR_DOUBLE: return 300;
        case TG_CLEAR_TRIPLE: return 500;
        case TG_CLEAR_TETRIS: return 800;
        case TG_CLEAR_T_SPIN_ZERO: return 400;
        case TG_CLEAR_T_SPIN_SINGLE: return 800;
        case TG_CLEAR_T_SPIN_DOUBLE: return 1200;
        case TG_CLEAR_T_SPIN_TRIPLE: return 1600;
        default: return 0;
    }
}

static void score_clear(TgGame *g, unsigned lines, bool t_spin) {
    TgClearKind kind = classify_clear(lines, t_spin);
    bool difficult = is_back_to_back_clear(kind);
    unsigned gained = base_score(kind) * g->level;

    if (difficult && g->back_to_back && gained > 0) {
        gained = gained * 3 / 2;
    }

    if (lines > 0) {
        g->combo += 1;
        if (g->combo > 0) {
            gained += (unsigned)g->combo * 50u * g->level;
        }
    } else {
        g->combo = -1;
    }

    if (difficult) {
        g->back_to_back = true;
    } else if (lines > 0) {
        g->back_to_back = false;
    }

    g->score += gained;
    g->lines += lines;
    g->level = g->lines / 10 + 1;
    g->last_clear.lines = lines;
    g->last_clear.score = gained;
    g->last_clear.level = g->level;
    g->last_clear.combo = (g->combo < 0) ? 0u : (unsigned)g->combo;
    g->last_clear.back_to_back = g->back_to_back;
    g->last_clear.kind = kind;
}

static void lock_piece(TgGame *g) {
    bool t_spin = is_t_spin(g, g->current);
    const TgPoint *cells = tg_piece_cells(g->current.type, g->current.rotation);

    for (int i = 0; i < 4; ++i) {
        int x = g->current.x + cells[i].x;
        int y = g->current.y + cells[i].y;
        if (y < TG_HIDDEN_ROWS) {
            g->status = TG_GAME_OVER;
        }
        if (x >= 0 && x < TG_COLS && y >= 0 && y < TG_ROWS) {
            g->board[y][x] = (uint8_t)g->current.type + 1u;
        }
    }

    unsigned cleared = tg_clear_full_lines(g);
    score_clear(g, cleared, t_spin);
    g->pieces_locked++;

    if (g->status != TG_GAME_OVER) {
        g->current = make_piece(pop_queue(g));
        reset_piece_timers(g);
        if (tg_collides(g, g->current)) {
            g->status = TG_GAME_OVER;
        }
    }
}

static void reset_lock_if_allowed(TgGame *g) {
    if (tg_is_grounded(g) && g->lock_resets < TG_MAX_LOCK_RESETS) {
        g->lock_timer_ms = 0;
        g->lock_resets++;
    }
}

const char *tg_piece_name(TgPieceType type) {
    static const char *names[] = {"I", "O", "T", "S", "Z", "J", "L"};
    if (type < 0 || type >= TG_PIECE_COUNT) {
        return "-";
    }
    return names[type];
}

const char *tg_clear_name(TgClearKind kind) {
    switch (kind) {
        case TG_CLEAR_SINGLE: return "Single";
        case TG_CLEAR_DOUBLE: return "Double";
        case TG_CLEAR_TRIPLE: return "Triple";
        case TG_CLEAR_TETRIS: return "Tetris";
        case TG_CLEAR_T_SPIN_ZERO: return "T-Spin";
        case TG_CLEAR_T_SPIN_SINGLE: return "T-Spin Single";
        case TG_CLEAR_T_SPIN_DOUBLE: return "T-Spin Double";
        case TG_CLEAR_T_SPIN_TRIPLE: return "T-Spin Triple";
        default: return "";
    }
}

const TgPoint *tg_piece_cells(TgPieceType type, int rotation) {
    return SHAPES[type][rotation & 3];
}

void tg_init(TgGame *g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed ? seed : 0xC0FFEEu;
    g->hold = TG_NO_PIECE;
    g->level = 1;
    g->combo = -1;
    g->status = TG_RUNNING;
    for (int i = 0; i < TG_NEXT_COUNT; ++i) {
        g->next[i] = TG_NO_PIECE;
    }
    shuffle_bag(g);
    refill_queue(g);
    g->current = make_piece(pop_queue(g));
    reset_piece_timers(g);
}

void tg_restart(TgGame *g, uint32_t seed) {
    tg_init(g, seed);
}

bool tg_collides(const TgGame *g, TgPiece piece) {
    const TgPoint *cells = tg_piece_cells(piece.type, piece.rotation);
    for (int i = 0; i < 4; ++i) {
        int x = piece.x + cells[i].x;
        int y = piece.y + cells[i].y;
        if (x < 0 || x >= TG_COLS || y >= TG_ROWS) {
            return true;
        }
        if (y >= 0 && g->board[y][x] != 0) {
            return true;
        }
    }
    return false;
}

bool tg_is_grounded(const TgGame *g) {
    TgPiece down = g->current;
    down.y++;
    return tg_collides(g, down);
}

TgPiece tg_ghost_piece(const TgGame *g) {
    TgPiece ghost = g->current;
    while (!tg_collides(g, ghost)) {
        ghost.y++;
    }
    ghost.y--;
    return ghost;
}

bool tg_move(TgGame *g, int dx, int dy) {
    if (g->status != TG_RUNNING) {
        return false;
    }
    TgPiece candidate = g->current;
    candidate.x += dx;
    candidate.y += dy;
    if (!tg_collides(g, candidate)) {
        g->current = candidate;
        g->last_action_was_rotate = false;
        if (dx != 0 || dy != 0) {
            reset_lock_if_allowed(g);
        }
        return true;
    }
    return false;
}

bool tg_rotate(TgGame *g, int direction) {
    if (g->status != TG_RUNNING || direction == 0) {
        return false;
    }

    int from = g->current.rotation & 3;
    int to = (from + (direction > 0 ? 1 : 3)) & 3;
    TgPiece rotated = g->current;
    rotated.rotation = to;

    const TgPoint (*kicks)[4][5] = (g->current.type == TG_I) ? KICKS_I : KICKS_JLSTZ;
    if (g->current.type == TG_O) {
        static const TgPoint no_kick[5] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
        for (int i = 0; i < 5; ++i) {
            TgPiece candidate = rotated;
            candidate.x += no_kick[i].x;
            candidate.y += no_kick[i].y;
            if (!tg_collides(g, candidate)) {
                g->current = candidate;
                g->last_action_was_rotate = true;
                reset_lock_if_allowed(g);
                return true;
            }
        }
        return false;
    }

    for (int i = 0; i < 5; ++i) {
        TgPiece candidate = rotated;
        candidate.x += kicks[from][to][i].x;
        candidate.y += kicks[from][to][i].y;
        if (!tg_collides(g, candidate)) {
            g->current = candidate;
            g->last_action_was_rotate = true;
            reset_lock_if_allowed(g);
            return true;
        }
    }
    return false;
}

bool tg_soft_drop(TgGame *g) {
    if (tg_move(g, 0, 1)) {
        g->score += 1;
        return true;
    }
    return false;
}

unsigned tg_hard_drop(TgGame *g) {
    if (g->status != TG_RUNNING) {
        return 0;
    }
    unsigned cells = 0;
    while (tg_move(g, 0, 1)) {
        cells++;
    }
    g->score += cells * 2u;
    lock_piece(g);
    return cells;
}

bool tg_hold(TgGame *g) {
    if (g->status != TG_RUNNING || g->hold_used) {
        return false;
    }

    TgPieceType old_current = g->current.type;
    if (g->hold == TG_NO_PIECE) {
        g->hold = old_current;
        g->current = make_piece(pop_queue(g));
    } else {
        TgPieceType swap = g->hold;
        g->hold = old_current;
        g->current = make_piece(swap);
    }
    g->hold_used = true;
    g->fall_accumulator_ms = 0;
    g->lock_timer_ms = 0;
    g->lock_resets = 0;
    g->last_action_was_rotate = false;

    if (tg_collides(g, g->current)) {
        g->status = TG_GAME_OVER;
    }
    return true;
}

void tg_toggle_pause(TgGame *g) {
    if (g->status == TG_RUNNING) {
        g->status = TG_PAUSED;
    } else if (g->status == TG_PAUSED) {
        g->status = TG_RUNNING;
    }
}

void tg_tick(TgGame *g, uint64_t elapsed_ms) {
    if (g->status != TG_RUNNING) {
        return;
    }

    if (tg_is_grounded(g)) {
        g->lock_timer_ms += elapsed_ms;
        if (g->lock_timer_ms >= TG_LOCK_DELAY_MS) {
            lock_piece(g);
            return;
        }
    } else {
        g->lock_timer_ms = 0;
        g->lock_resets = 0;
    }

    g->fall_accumulator_ms += elapsed_ms;
    unsigned interval = tg_gravity_interval_ms(g);
    while (g->fall_accumulator_ms >= interval && g->status == TG_RUNNING) {
        g->fall_accumulator_ms -= interval;
        if (!tg_move(g, 0, 1)) {
            if (g->lock_timer_ms >= TG_LOCK_DELAY_MS) {
                lock_piece(g);
            }
            break;
        }
    }
}

unsigned tg_clear_full_lines(TgGame *g) {
    unsigned cleared = 0;
    for (int y = TG_ROWS - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < TG_COLS; ++x) {
            if (g->board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            cleared++;
            for (int yy = y; yy > 0; --yy) {
                memcpy(g->board[yy], g->board[yy - 1], sizeof(g->board[yy]));
            }
            memset(g->board[0], 0, sizeof(g->board[0]));
            y++;
        }
    }
    return cleared;
}

unsigned tg_gravity_interval_ms(const TgGame *g) {
    if (g->level >= 20) {
        return 20;
    }
    static const unsigned intervals[] = {
        0, 800, 720, 630, 550, 470, 380, 300, 220, 160, 120,
        100, 90, 80, 70, 60, 50, 40, 35, 30
    };
    return intervals[g->level];
}
