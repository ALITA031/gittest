#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    IN_NONE = 0,
    IN_QUIT,
    IN_LEFT,
    IN_RIGHT,
    IN_DOWN,
    IN_ROTATE_CW,
    IN_ROTATE_CCW,
    IN_HARD_DROP,
    IN_HOLD,
    IN_PAUSE,
    IN_RESTART
} TermInput;

bool term_enter_game_mode(void);
void term_leave_game_mode(void);
TermInput term_read_input(void);
uint64_t term_now_ms(void);
void term_sleep_ms(unsigned ms);
bool term_size_too_small(int min_cols, int min_rows);

#endif
