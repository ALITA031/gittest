# Architecture

Terminal Tetris Pro is intentionally split into a portable engine and a terminal frontend. This keeps gameplay behavior testable and makes future frontends possible.

## Module map

```text
src/game.c       Engine: board, pieces, queue, hold, gravity, lock delay, scoring
src/tetris.h     Public engine types and API
src/render.c     Terminal renderer using ANSI escape sequences
src/render.h     Renderer interface
src/terminal.c   Raw mode, non-blocking input, terminal size, sleeping, timing
src/terminal.h   Terminal/input interface
src/main.c       Application loop, high-score persistence, input dispatch
```

## Engine layer

`src/game.c` owns the rules. It should not call terminal, filesystem, or platform APIs. It should remain deterministic when initialized with the same seed.

Responsibilities:

- board representation;
- tetromino definitions;
- 7-bag queue;
- current, next, and hold pieces;
- collision detection;
- SRS-style rotation and wall kicks;
- soft drop and hard drop;
- gravity and lock delay;
- line clears;
- scoring state;
- game status.

The engine is exposed through `src/tetris.h`. Tests should prefer the public API unless a behavior requires direct inspection of the `TgGame` state.

## Terminal layer

`src/terminal.c` owns platform-specific terminal behavior:

- raw mode setup and restoration;
- non-blocking keyboard input;
- mapping escape sequences to `TermInput` values;
- monotonic time;
- frame sleeping;
- terminal size checks.

Any future SDL2, raylib, or web frontend should not reuse this layer. It should call the engine directly.

## Rendering layer

`src/render.c` translates engine state into terminal output. It is allowed to know about board dimensions, piece colors, ghost pieces, and side panels, but it should not change gameplay state.

Rendering should stay a pure view of `TgGame` as much as possible.

## Application layer

`src/main.c` wires the game together:

1. enter terminal game mode;
2. check terminal size;
3. load high score;
4. initialize the engine;
5. read inputs;
6. update the engine with elapsed time;
7. render the frame;
8. save high score on quit/restart paths.

This file can use terminal APIs and filesystem APIs. It should avoid embedding game rules that belong in `src/game.c`.

## Testing strategy

The core test target builds only:

```text
tests/test_game.c
src/game.c
```

That is intentional. Engine tests should not need a terminal. This keeps CI reliable and makes tests suitable for sanitizers.

Recommended regression-test areas:

- deterministic seeds;
- piece collision;
- line clearing;
- hold lifecycle;
- ghost piece behavior;
- scoring and combo/back-to-back behavior;
- pause and game-over transitions;
- lock delay;
- SRS wall kicks;
- future replay determinism.

## Design rules

- Keep gameplay state in `TgGame`.
- Keep terminal state out of `TgGame`.
- Prefer small public engine functions over hidden behavior in `main.c`.
- Add tests before changing scoring, collision, rotation, gravity, or lock delay.
- Avoid new dependencies unless they enable a clearly separate frontend or tool.
