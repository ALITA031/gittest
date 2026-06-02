#define _POSIX_C_SOURCE 200809L

#include "render.h"
#include "terminal.h"
#include "tetris.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static uint32_t make_seed(void) {
    uint64_t t = (uint64_t)time(NULL);
    uint64_t p = (uint64_t)getpid();
    uint64_t mixed = t ^ (p << 16) ^ 0x9E3779B97F4A7C15ULL;
    mixed ^= mixed >> 33;
    mixed *= 0xff51afd7ed558ccdULL;
    mixed ^= mixed >> 33;
    return (uint32_t)mixed ? (uint32_t)mixed : 0xC0FFEEu;
}

static int mkdir_if_needed(const char *path) {
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static int join_path(char *out, size_t size, const char *base, const char *leaf) {
    size_t base_len = strlen(base);
    size_t leaf_len = strlen(leaf);
    if (base_len + 1u + leaf_len + 1u > size) {
        return -1;
    }
    memcpy(out, base, base_len);
    out[base_len] = '/';
    memcpy(out + base_len + 1u, leaf, leaf_len + 1u);
    return 0;
}

static void highscore_path(char *buffer, size_t size) {
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    char dir[1024];
    char local[1024];
    char share[1024];

    if (xdg && xdg[0] != '\0') {
        mkdir_if_needed(xdg);
        if (join_path(dir, sizeof(dir), xdg, "terminal-tetris-pro") == 0) {
            mkdir_if_needed(dir);
            if (join_path(buffer, size, dir, "highscore") == 0) {
                return;
            }
        }
    }

    if (home && home[0] != '\0') {
        if (join_path(local, sizeof(local), home, ".local") == 0) {
            mkdir_if_needed(local);
            if (join_path(share, sizeof(share), local, "share") == 0) {
                mkdir_if_needed(share);
                if (join_path(dir, sizeof(dir), share, "terminal-tetris-pro") == 0) {
                    mkdir_if_needed(dir);
                    if (join_path(buffer, size, dir, "highscore") == 0) {
                        return;
                    }
                }
            }
        }
    }

    if (size > 0) {
        const char *fallback = ".terminal-tetris-pro-highscore";
        size_t n = strlen(fallback);
        if (n >= size) {
            n = size - 1u;
        }
        memcpy(buffer, fallback, n);
        buffer[n] = '\0';
    }
}

static unsigned load_highscore(void) {
    char path[512];
    highscore_path(path, sizeof(path));
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }
    unsigned score = 0;
    if (fscanf(fp, "%u", &score) != 1) {
        score = 0;
    }
    fclose(fp);
    return score;
}

static void save_highscore(unsigned score) {
    char path[512];
    highscore_path(path, sizeof(path));
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return;
    }
    fprintf(fp, "%u\n", score);
    fclose(fp);
}

static void apply_input(TgGame *game, TermInput input, unsigned *best_score) {
    switch (input) {
        case IN_QUIT:
            if (game->score > *best_score) {
                *best_score = game->score;
                save_highscore(*best_score);
            }
            term_leave_game_mode();
            exit(EXIT_SUCCESS);
        case IN_LEFT:
            tg_move(game, -1, 0);
            break;
        case IN_RIGHT:
            tg_move(game, 1, 0);
            break;
        case IN_DOWN:
            tg_soft_drop(game);
            break;
        case IN_ROTATE_CW:
            tg_rotate(game, 1);
            break;
        case IN_ROTATE_CCW:
            tg_rotate(game, -1);
            break;
        case IN_HARD_DROP:
            tg_hard_drop(game);
            break;
        case IN_HOLD:
            tg_hold(game);
            break;
        case IN_PAUSE:
            tg_toggle_pause(game);
            break;
        case IN_RESTART:
            if (game->score > *best_score) {
                *best_score = game->score;
                save_highscore(*best_score);
            }
            tg_restart(game, make_seed());
            break;
        case IN_NONE:
        default:
            break;
    }
}

int main(void) {
    if (!term_enter_game_mode()) {
        return EXIT_FAILURE;
    }

    if (term_size_too_small(82, 26)) {
        term_leave_game_mode();
        fprintf(stderr, "Terminal is too small. Please use at least 82 columns x 26 rows.\n");
        return EXIT_FAILURE;
    }

    unsigned best_score = load_highscore();
    TgGame game;
    tg_init(&game, make_seed());

    uint64_t last = term_now_ms();
    while (true) {
        TermInput input;
        while ((input = term_read_input()) != IN_NONE) {
            apply_input(&game, input, &best_score);
        }

        uint64_t now = term_now_ms();
        uint64_t elapsed = now >= last ? now - last : 0;
        last = now;

        tg_tick(&game, elapsed);
        if (game.score > best_score) {
            best_score = game.score;
        }

        render_game(&game, best_score);
        term_sleep_ms(16);
    }
}
