#include "render.h"

#include <stdbool.h>
#include <stdio.h>

static const char *COLORS[TG_PIECE_COUNT] = {
    "\033[38;5;51m",
    "\033[38;5;226m",
    "\033[38;5;201m",
    "\033[38;5;46m",
    "\033[38;5;196m",
    "\033[38;5;27m",
    "\033[38;5;214m"
};

static void cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

static void clear_line(void) {
    printf("\033[2K");
}

static bool piece_has_cell(TgPiece p, int x, int y) {
    const TgPoint *cells = tg_piece_cells(p.type, p.rotation);
    for (int i = 0; i < 4; ++i) {
        if (p.x + cells[i].x == x && p.y + cells[i].y == y) {
            return true;
        }
    }
    return false;
}

static void draw_cell(int value, bool ghost) {
    if (ghost) {
        printf("\033[2m░░\033[0m");
    } else if (value > 0) {
        int idx = value - 1;
        if (idx >= 0 && idx < TG_PIECE_COUNT) {
            printf("%s██\033[0m", COLORS[idx]);
        } else {
            printf("██");
        }
    } else {
        printf("  ");
    }
}

static void draw_mini_piece(int row, int col, TgPieceType type) {
    for (int y = 0; y < 4; ++y) {
        cursor(row + y, col);
        printf("        ");
    }

    if (type == TG_NO_PIECE) {
        cursor(row + 1, col);
        printf("--");
        return;
    }

    const TgPoint *cells = tg_piece_cells(type, 0);
    for (int i = 0; i < 4; ++i) {
        cursor(row + cells[i].y, col + cells[i].x * 2);
        printf("%s██\033[0m", COLORS[type]);
    }
}

static void draw_panel_line(int *row, int col, const char *text) {
    cursor(*row, col);
    clear_line();
    printf("%s", text);
    (*row)++;
}

void render_game(const TgGame *g, unsigned best_score) {
    const int top = 2;
    const int left = 4;
    const int board_left = left + 14;
    const int right_panel = board_left + TG_COLS * 2 + 5;
    const int visible_start = TG_HIDDEN_ROWS;
    TgPiece ghost = tg_ghost_piece(g);

    cursor(1, left);
    clear_line();
    printf("\033[1mTerminal Tetris Pro / 俄罗斯方块\033[0m");

    cursor(top, board_left - 1);
    printf("┌");
    for (int x = 0; x < TG_COLS * 2; ++x) {
        printf("─");
    }
    printf("┐");

    for (int y = visible_start; y < TG_ROWS; ++y) {
        int screen_y = top + 1 + (y - visible_start);
        cursor(screen_y, board_left - 1);
        printf("│");
        for (int x = 0; x < TG_COLS; ++x) {
            bool active = piece_has_cell(g->current, x, y);
            bool ghost_cell = piece_has_cell(ghost, x, y) && !active && g->status != TG_GAME_OVER;
            if (active && g->status != TG_GAME_OVER) {
                draw_cell((int)g->current.type + 1, false);
            } else if (g->board[y][x] != 0) {
                draw_cell(g->board[y][x], false);
            } else if (ghost_cell) {
                draw_cell(0, true);
            } else {
                draw_cell(0, false);
            }
        }
        printf("│");
    }

    cursor(top + TG_VISIBLE_ROWS + 1, board_left - 1);
    printf("└");
    for (int x = 0; x < TG_COLS * 2; ++x) {
        printf("─");
    }
    printf("┘");

    int row = top + 1;
    cursor(row++, left); clear_line(); printf("\033[1mHOLD\033[0m");
    draw_mini_piece(row, left, g->hold);

    row += 6;
    cursor(row++, left); clear_line(); printf("\033[1mSTATS\033[0m");
    cursor(row++, left); clear_line(); printf("Score  %u", g->score);
    cursor(row++, left); clear_line(); printf("Best   %u", best_score);
    cursor(row++, left); clear_line(); printf("Level  %u", g->level);
    cursor(row++, left); clear_line(); printf("Lines  %u", g->lines);
    cursor(row++, left); clear_line(); printf("Combo  %u", g->combo < 0 ? 0u : (unsigned)g->combo);
    cursor(row++, left); clear_line(); printf("B2B    %s", g->back_to_back ? "yes" : "no");

    row = top + 1;
    cursor(row++, right_panel); clear_line(); printf("\033[1mNEXT\033[0m");
    for (int i = 0; i < TG_NEXT_COUNT; ++i) {
        draw_mini_piece(row, right_panel, g->next[i]);
        row += 4;
    }

    row = top + 1 + TG_VISIBLE_ROWS - 8;
    draw_panel_line(&row, right_panel, "\033[1mCONTROLS\033[0m");
    draw_panel_line(&row, right_panel, "A/D or ←/→  move");
    draw_panel_line(&row, right_panel, "S or ↓      soft drop");
    draw_panel_line(&row, right_panel, "W/X or ↑    rotate");
    draw_panel_line(&row, right_panel, "Z           rotate back");
    draw_panel_line(&row, right_panel, "Space       hard drop");
    draw_panel_line(&row, right_panel, "C           hold");
    draw_panel_line(&row, right_panel, "P/Q/R       pause/quit/restart");

    cursor(top + TG_VISIBLE_ROWS + 3, left);
    clear_line();
    if (g->last_clear.kind != TG_CLEAR_NONE) {
        printf("Last: %s  +%u", tg_clear_name(g->last_clear.kind), g->last_clear.score);
    } else {
        printf(" ");
    }

    if (g->status == TG_PAUSED) {
        cursor(top + 9, board_left + 4);
        printf("\033[1;7m PAUSED \033[0m");
    } else if (g->status == TG_GAME_OVER) {
        cursor(top + 8, board_left + 3);
        printf("\033[1;7m GAME OVER \033[0m");
        cursor(top + 10, board_left + 2);
        printf("R restart   Q quit");
    }

    fflush(stdout);
}
