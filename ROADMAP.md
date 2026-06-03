# Terminal Tetris Pro Roadmap

This roadmap turns the project from a playable terminal game into a maintainable, extensible open-source Tetris engine and terminal client.

## Product vision

Terminal Tetris Pro should stay small enough to compile with a plain C compiler, but strong enough to demonstrate real game-engine design:

- a deterministic and testable core engine;
- a terminal frontend that feels responsive on macOS and Linux;
- clear separation between engine, rendering, input, persistence, and future frontends;
- a development process with tests, sanitizers, CI, documentation, and reviewable milestones.

## Current foundation

The project already has the right base architecture:

- `src/game.c` contains the engine and does not depend on terminal APIs;
- `src/render.c` owns terminal drawing;
- `src/terminal.c` owns raw mode, input, and timing;
- `src/main.c` wires the loop, input dispatch, and high-score persistence;
- `tests/test_game.c` covers engine behavior;
- GitHub Actions builds with GCC and Clang.

## Phase 1: harden the engine

Goal: make the existing game safer to modify.

- Expand unit tests for line clears, hold, pause, ghost piece, deterministic seeds, scoring, and gravity.
- Add regression tests before changing rotation, scoring, or lock-delay behavior.
- Keep `src/game.c` independent from terminal code.
- Make all gameplay constants explicit and documented.
- Treat sanitizer failures as release blockers.

## Phase 2: improve player experience

Goal: make the terminal game feel more polished without changing its minimal dependency model.

- Add a start screen and game-over menu instead of immediate play/restart only.
- Add configurable controls through a small config file.
- Add adjustable DAS/ARR-style horizontal movement settings.
- Add optional color themes.
- Add a pause overlay and clearer last-clear feedback.
- Add a local leaderboard instead of a single high score.

## Phase 3: strengthen rules and scoring

Goal: move closer to modern guideline-style Tetris behavior while keeping the implementation understandable.

- Refine T-Spin Mini detection.
- Add Perfect Clear detection and scoring.
- Add configurable speed curves.
- Add garbage-line primitives for future battle/AI modes.
- Document intentional deviations from official games.

## Phase 4: make the engine reusable

Goal: allow other frontends to reuse the same engine.

- Split public engine API documentation into `docs/ENGINE.md`.
- Add snapshot/debug helpers for tests and replays.
- Add a replay event log format.
- Add deterministic replay tests.
- Keep rendering outside the engine.

## Phase 5: advanced modes

Goal: make the project interesting as a long-term open-source repository.

- Sprint mode: clear 40 lines as fast as possible.
- Ultra mode: score as much as possible in a fixed time.
- AI bot mode using board evaluation heuristics.
- Replay viewer.
- Optional SDL2/raylib frontend that reuses the engine.

## Quality bar

A change should not be merged unless it satisfies the following:

- `make test` passes;
- `make sanitize` passes on supported systems;
- new engine behavior has at least one regression test;
- public behavior is reflected in README or docs;
- large changes are split into reviewable commits.

## Near-term priorities

1. Keep CI green.
2. Expand test coverage around the engine.
3. Add documentation for architecture and contribution workflow.
4. Add leaderboard and configuration support.
5. Add replay infrastructure before adding AI or extra frontends.
