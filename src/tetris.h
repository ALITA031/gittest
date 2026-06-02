#ifndef TETRIS_H
#define TETRIS_H

#include <stdbool.h>
#include <stdint.h>

#define TG_COLS 10
#define TG_VISIBLE_ROWS 20
#define TG_HIDDEN_ROWS 2
#define TG_ROWS (TG_VISIBLE_ROWS + TG_HIDDEN_ROWS)
#define TG_NEXT_COUNT 5
#define TG_BAG_SIZE 7
#define TG_LOCK_DELAY_MS 500
#define TG_MAX_LOCK_RESETS 15

typedef enum {
    TG_I = 0,
    TG_O,
    TG_T,
    TG_S,
    TG_Z,
    TG_J,
    TG_L,
    TG_PIECE_COUNT,
    TG_NO_PIECE = -1
} TgPieceType;

typedef enum {
    TG_RUNNING = 0,
    TG_PAUSED,
    TG_GAME_OVER
} TgStatus;

typedef enum {
    TG_CLEAR_NONE = 0,
    TG_CLEAR_SINGLE,
    TG_CLEAR_DOUBLE,
    TG_CLEAR_TRIPLE,
    TG_CLEAR_TETRIS,
    TG_CLEAR_T_SPIN_ZERO,
    TG_CLEAR_T_SPIN_SINGLE,
    TG_CLEAR_T_SPIN_DOUBLE,
    TG_CLEAR_T_SPIN_TRIPLE
} TgClearKind;

typedef struct {
    int x;
    int y;
} TgPoint;

typedef struct {
    TgPieceType type;
    int rotation;
    int x;
    int y;
} TgPiece;

typedef struct {
    unsigned lines;
    unsigned score;
    unsigned level;
    unsigned combo;
    bool back_to_back;
    TgClearKind kind;
} TgClearResult;

typedef struct {
    uint8_t board[TG_ROWS][TG_COLS];
    TgPiece current;
    TgPieceType hold;
    TgPieceType next[TG_NEXT_COUNT];

    TgPieceType bag[TG_BAG_SIZE];
    int bag_index;
    uint32_t rng;

    unsigned score;
    unsigned lines;
    unsigned level;
    unsigned pieces_locked;
    int combo;
    bool back_to_back;
    bool hold_used;

    uint64_t fall_accumulator_ms;
    uint64_t lock_timer_ms;
    unsigned lock_resets;
    bool last_action_was_rotate;

    TgStatus status;
    TgClearResult last_clear;
} TgGame;

const char *tg_piece_name(TgPieceType type);
const char *tg_clear_name(TgClearKind kind);
const TgPoint *tg_piece_cells(TgPieceType type, int rotation);

void tg_init(TgGame *g, uint32_t seed);
void tg_restart(TgGame *g, uint32_t seed);

bool tg_is_grounded(const TgGame *g);
bool tg_collides(const TgGame *g, TgPiece piece);
TgPiece tg_ghost_piece(const TgGame *g);

bool tg_move(TgGame *g, int dx, int dy);
bool tg_rotate(TgGame *g, int direction);
bool tg_soft_drop(TgGame *g);
unsigned tg_hard_drop(TgGame *g);
bool tg_hold(TgGame *g);
void tg_toggle_pause(TgGame *g);
void tg_tick(TgGame *g, uint64_t elapsed_ms);

unsigned tg_clear_full_lines(TgGame *g);
unsigned tg_gravity_interval_ms(const TgGame *g);

#endif
