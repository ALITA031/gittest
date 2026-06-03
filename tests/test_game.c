#include "../src/tetris.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_clear_lines(void) {
    TgGame g;
    tg_init(&g, 1234u);
    memset(g.board, 0, sizeof(g.board));

    for (int x = 0; x < TG_COLS; ++x) {
        g.board[TG_ROWS - 1][x] = 1;
        g.board[TG_ROWS - 2][x] = 2;
    }

    unsigned cleared = tg_clear_full_lines(&g);
    assert(cleared == 2);
    for (int x = 0; x < TG_COLS; ++x) {
        assert(g.board[TG_ROWS - 1][x] == 0);
        assert(g.board[TG_ROWS - 2][x] == 0);
    }
}

static void test_hold_once_per_piece(void) {
    TgGame g;
    tg_init(&g, 42u);

    TgPieceType first = g.current.type;
    assert(tg_hold(&g));
    assert(g.hold == first);
    assert(g.hold_used);
    assert(!tg_hold(&g));
}

static void test_hold_resets_after_lock(void) {
    TgGame g;
    tg_init(&g, 43u);

    assert(tg_hold(&g));
    assert(g.hold_used);
    tg_hard_drop(&g);
    assert(g.status == TG_RUNNING);
    assert(!g.hold_used);
    assert(tg_hold(&g));
}

static void test_wall_collision(void) {
    TgGame g;
    tg_init(&g, 99u);
    g.current.type = TG_I;
    g.current.rotation = 0;
    g.current.x = -2;
    g.current.y = TG_HIDDEN_ROWS;
    assert(tg_collides(&g, g.current));
}

static void test_hard_drop_locks_piece(void) {
    TgGame g;
    tg_init(&g, 7u);
    unsigned before = g.pieces_locked;
    unsigned dropped = tg_hard_drop(&g);
    assert(dropped > 0);
    assert(g.pieces_locked == before + 1);
}

static void test_ghost_piece_is_lowest_valid_position(void) {
    TgGame g;
    tg_init(&g, 100u);
    memset(g.board, 0, sizeof(g.board));
    g.current.type = TG_T;
    g.current.rotation = 0;
    g.current.x = 3;
    g.current.y = TG_HIDDEN_ROWS;

    TgPiece ghost = tg_ghost_piece(&g);
    assert(!tg_collides(&g, ghost));
    ghost.y++;
    assert(tg_collides(&g, ghost));
}

static void test_pause_blocks_tick_and_moves(void) {
    TgGame g;
    tg_init(&g, 101u);
    int y = g.current.y;

    tg_toggle_pause(&g);
    assert(g.status == TG_PAUSED);
    tg_tick(&g, 5000u);
    assert(g.current.y == y);
    assert(!tg_move(&g, 1, 0));

    tg_toggle_pause(&g);
    assert(g.status == TG_RUNNING);
}

static void test_single_line_clear_scores_and_updates_state(void) {
    TgGame g;
    tg_init(&g, 102u);
    memset(g.board, 0, sizeof(g.board));

    for (int x = 0; x < TG_COLS; ++x) {
        g.board[TG_ROWS - 1][x] = 1;
    }
    for (int x = 3; x <= 6; ++x) {
        g.board[TG_ROWS - 1][x] = 0;
    }

    g.current.type = TG_I;
    g.current.rotation = 0;
    g.current.x = 3;
    g.current.y = TG_HIDDEN_ROWS;
    g.score = 0;
    g.lines = 0;
    g.level = 1;
    g.combo = -1;
    g.back_to_back = false;

    unsigned dropped = tg_hard_drop(&g);
    assert(dropped > 0);
    assert(g.lines == 1);
    assert(g.level == 1);
    assert(g.last_clear.kind == TG_CLEAR_SINGLE);
    assert(g.last_clear.lines == 1);
    assert(g.last_clear.score >= 100);
    assert(g.score >= g.last_clear.score);
}

static void test_restart_is_deterministic_for_seed(void) {
    TgGame a;
    TgGame b;
    tg_init(&a, 777u);
    tg_init(&b, 777u);

    assert(a.current.type == b.current.type);
    for (int i = 0; i < TG_NEXT_COUNT; ++i) {
        assert(a.next[i] == b.next[i]);
    }

    tg_restart(&b, 778u);
    bool differs = a.current.type != b.current.type;
    for (int i = 0; i < TG_NEXT_COUNT; ++i) {
        differs = differs || (a.next[i] != b.next[i]);
    }
    assert(differs);
}

static void test_gravity_interval_decreases(void) {
    TgGame g;
    tg_init(&g, 88u);
    g.level = 1;
    unsigned slow = tg_gravity_interval_ms(&g);
    g.level = 10;
    unsigned fast = tg_gravity_interval_ms(&g);
    assert(fast < slow);
    g.level = 20;
    assert(tg_gravity_interval_ms(&g) == 20);
}

int main(void) {
    test_clear_lines();
    test_hold_once_per_piece();
    test_hold_resets_after_lock();
    test_wall_collision();
    test_hard_drop_locks_piece();
    test_ghost_piece_is_lowest_valid_position();
    test_pause_blocks_tick_and_moves();
    test_single_line_clear_scores_and_updates_state();
    test_restart_is_deterministic_for_seed();
    test_gravity_interval_decreases();
    puts("All game-engine tests passed.");
    return 0;
}
