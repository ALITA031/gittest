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

static void test_gravity_interval_decreases(void) {
    TgGame g;
    tg_init(&g, 88u);
    g.level = 1;
    unsigned slow = tg_gravity_interval_ms(&g);
    g.level = 10;
    unsigned fast = tg_gravity_interval_ms(&g);
    assert(fast < slow);
}

int main(void) {
    test_clear_lines();
    test_hold_once_per_piece();
    test_wall_collision();
    test_hard_drop_locks_piece();
    test_gravity_interval_decreases();
    puts("All game-engine tests passed.");
    return 0;
}
