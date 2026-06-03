# Contributing

Thanks for improving Terminal Tetris Pro. This project is small, but it should be treated like a real C codebase: readable changes, tests, and a clear separation between engine and terminal frontend.

## Development setup

You need a POSIX-like environment with a C11 compiler and `make`.

Recommended commands:

```bash
make
make test
make sanitize
```

For a full local check, run:

```bash
make check
```

## Code style

- Use C11.
- Keep warnings enabled: `-Wall -Wextra -Wpedantic`.
- Prefer explicit names over clever abbreviations.
- Keep engine code independent from terminal and filesystem APIs.
- Avoid global mutable state unless there is a strong reason.
- Keep functions small enough to test and reason about.

## Engine changes

For changes in `src/game.c` or `src/tetris.h`:

- add or update tests in `tests/test_game.c`;
- verify deterministic behavior when seeded;
- document any rule changes in README or docs;
- be careful with scoring, line clear, rotation, lock delay, and collision behavior.

## Terminal/frontend changes

For changes in `src/render.c`, `src/terminal.c`, or `src/main.c`:

- verify the game still exits cleanly and restores the terminal;
- avoid moving gameplay rules out of the engine;
- keep platform-specific behavior isolated in `src/terminal.c`;
- test in a real terminal where possible.

## Commit guidelines

Good commit messages are short and specific:

```text
Expand engine tests for hold lifecycle
Add architecture documentation
Refine terminal input parsing
```

Avoid mixing unrelated work in one commit. For example, do not combine rendering changes, scoring changes, and README rewrites unless they are part of the same behavior change.

## Pull request checklist

Before opening or merging a PR:

- [ ] `make test` passes.
- [ ] `make sanitize` passes where supported.
- [ ] New engine behavior has regression coverage.
- [ ] README/docs are updated when public behavior changes.
- [ ] The terminal is restored correctly after quitting or errors.

## Project direction

Near-term work should prioritize reliability and maintainability before large feature expansion:

1. keep CI green;
2. strengthen engine tests;
3. document architecture;
4. add configuration and leaderboard support;
5. add replay infrastructure;
6. add AI or graphical frontends only after the engine is easier to verify.
