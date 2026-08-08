# Changelog

All notable changes to this project will be documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-08-08

### Added

- **Automated installation script** — Added `install.sh` to detect the
  operating system and package manager, install build dependencies, and set up
  Tomato.C system-wide with the appropriate data prefix. Supports APT, DNF,
  Pacman, APK, Zypper, Homebrew, and XBPS.

- **Uninstallation support** — `install.sh --uninstall` now removes the
  installed binary, shared application data, and Linux desktop entry while
  preserving user configuration files.

- **Void Linux support** — Added installation support for Void Linux through
  the XBPS package manager.

- **Dedicated Pomodoro state machine** — Added `pomodoro.c` and `pomodoro.h` to
  centralize Pomodoro initialization, time ticking, step transitions, and
  duration calculations.

- **Expanded test coverage** — Added unit and integration tests across
  animation, audio, drawing, input, notifications, logging, configuration, UI,
  initialization, history, Tomato, and Pomodoro modules.

- **Out-of-memory tests** — Added allocation-failure tests for animation and
  bar modules, with platform-aware malloc wrapping.

### Fixed

- **Animation frame buffer overflows** — Added buffer bounds checks and
  explicit length handling during frame deserialization.

- **Configuration memory handling** — Fixed configuration string lifetime
  issues by using `strdup`, added `PATH_MAX` handling for paths, and validated
  numeric configuration values against their supported bounds.

- **Memory leaks and allocation errors** — Improved allocation and cleanup
  handling across sessions, logging, notifications, and other dynamic data
  paths.

- **UI rendering buffer safety** — Added bounds checking to `snprintf` loops
  used by UI rendering functions to prevent buffer overflows.

- **`FPS` macro safety** — Wrapped the `FPS` macro in parentheses to prevent
  unexpected expression evaluation.

- **Small terminal handling** — `DrawScreen` now returns `NO_ERROR` when the
  terminal is too small after displaying the appropriate error message,
  allowing the application to continue running gracefully.

- **Portable test binary discovery** — Replaced the non-portable `find
  -executable` test with `find -perm -111`, improving compatibility with POSIX
  environments such as macOS and Alpine.

- **macOS animation tests** — Replaced Linux-specific `/proc/self/fd` paths
  with portable `/dev/fd` paths for sprite deserialization tests.

- **Test script robustness** — Fixed unbound `MAGENTA` variables in `build.sh`,
  `install.sh`, and `run_tests.sh`, and corrected positional parameter
  expansion from `$10` to `${10}` in `run_tests.sh`.

### Changed

- **Pomodoro state management** — `update.c` now uses the centralized
  `TransitionPomodoroStep` state-machine function for work, short-pause, and
  long-pause transitions instead of maintaining duplicated transition logic.

- **Build system** — Added the new Pomodoro module to the build configuration.

- **Installation workflow** — Installation is now handled through `install.sh`,
  with automatic dependency detection and system-wide setup.

- **Test integration** — Integration tests now link against the real Pomodoro
  implementation instead of maintaining a separate copy of its logic.

- **Draw tests** — `unit/test_draw` now links against the real `draw.c` and
  `util.c` implementations instead of mirrored test logic.

- **CI test workflow** — Ubuntu and macOS test jobs now run `run_tests.sh` with
  `--verbose` and `--clean` to ensure clean builds and detailed test output.

- **Test platform handling** — Malloc wrapping is now enabled only on supported
  platforms, allowing out-of-memory tests to run on Linux without breaking
  other environments.

- **Code formatting** — Source files were reformatted to comply with the
  project's `.clang-format` rules and style guidelines.

- **Documentation** — README installation, uninstallation, contribution,
  development, and documentation instructions were updated and streamlined.

## [1.0.3] - 2026-07-19

### Fixed

- **Session duration now includes short and long breaks** — `HistSessionsForDay`
  and `GetPomodoroHistoryDay` compute duration via the pomodoro formula
  (`cycles × work + (cycles−1) × short + long`) instead of using
  `current_step_time` or `total_elapsed`, giving accurate planned session
  length.
- **History overview (`-h`) work time now equals full session time** —
  `createPomodoroHistoryStats` counts total session duration (work + pauses)
  instead of work-only, matching the per-day display.
- **TUI history statistics dialog no longer overcounts sessions and focus
  time** — `CreateHistoryStatsDialog` deduplicates by `session_index` and uses
  the formula for focus time instead of summing `current_step_time / 60` across
  every per-minute record (which inflated counts by 145× per session).
- **Day history dialog end time now reflects real recorded end** —
  `HistSessionsForDay` returns end times via
  `session_start_time + total_elapsed` (the actual wall-clock end), not
  `start_time + formula_duration`, so skipped sessions display the correct
  end time.
- **`localtime_r` replaces non-reentrant `localtime`** in history display
  functions to avoid shared-data races.

### Changed

- **`HistSessionsForDay` signature** — New `time_t* endTimes` parameter for
  returning real session end times; callers may pass `NULL`.
- **Row numbering in history dialogs** — Sessions are now numbered sequentially
  per day (1, 2, 3…) instead of showing the global `session_index`.

### Added

- **Comprehensive unit tests for `HistSessionsForDay`** — 8 new tests covering
  skipped sessions (endTime == startTime), multi-record deduplication,
  formula-based duration, end-time from `total_elapsed`, uncompleted sessions
  (duration 0), same-day multiple sessions, and day filtering.

## [1.0.2] - 2026-07-18

### Added

- **Default static frame support** — Optional `/Df` suffix on sprite header
  lines selects a static frame for non-animated rendering. All `.asc` files
  updated with appropriate default frames per scene.

- **Multi-format icon switching** — All three icon types (nerd-icons, emojis,
  ascii) are parsed at init into a 2D array
  `icon_animations[MAX_ICON_TYPES][MAX_ANIMATIONS]`, with `app->animations` as
  a pointer to the active set. Icon changes in the preferences menu now take
  effect immediately via LEFT/RIGHT/SPACE, not just after closing the sub-dialog.

### Fixed

- **Notes scene not rendering when `!ANIMATIONS`** — `renderNotesScene` was
  skipped in the non-animated branch; notes page indicator is now rendered with
  an animation reference.
- **Page transitions stuck when `!ANIMATIONS`** — `UpdateNotes` early-returned
  before completing page transitions, leaving `transitioning` permanently set.

## [1.0.1] - 2026-07-17

### Fixed

- **Config paths broken when running outside the project directory** —
  Notification `audio_path` and noise `sound_path` values read from TOML
  config files were stored as dangling pointers after `toml_free()`
  invalidated the parser's string table, causing `InitApp()` to read freed
  memory and fail with `ErrorCode 8`.  Fixed by `strdup`'ing (via
  `expandDatadir`) every path read from TOML before the parser is freed.

### Added

- **`$DATADIR` token expansion for configuration paths** — TOML config
  values for `audio_path`, `sound_path`, and logging file paths now
  support the `$DATADIR` token, which is expanded at load time to the
  compile-time `DATADIR` path (e.g. `/usr/local/share/tomato`).  This
  makes configuration files portable regardless of installation prefix
  or working directory.

### Changed

- **`sample_config.toml`** — All hardcoded `audio_path` and `sound_path`
  values replaced with `$DATADIR/sounds/...` tokens, preventing path
  breakage when the sample config is used as a user config.

## [1.0.0] - 2026-07-17

### Added

#### Core Pomodoro Timer
- Work / short break / long break cycle with fully configurable durations
- Pause, resume, skip, and reset functionality
- Cycle tracking with configurable pomodoros per cycle (1–8)
- Auto-start prompts after each completed step
- Unfinished session detection and continuation prompt

#### Terminal User Interface
- Dynamic multi-panel layout using ncursesw
- ASCII sprite animation engine (`Rollfilm`) with frame-based playback, timing,
  color, and looping
- Welcome carousel with multi-slide introduction
- Configurable status bar with left, center, and right zones
- Status bar modules: `InputMode`, `RealTime`, `Scene`, `CurrentStatus`,
  `LineColumn`, `Streak`, `Date`, `Weekday`, `TerminalSize`, `Icons`
- Mouse interaction support (click regions, scroll wheel)
- Error line display with auto-clearing timeout
- Screen-size validation
- Three icon sets: Nerd Fonts (default), Emojis, and ASCII — switchable at
  runtime or in config

#### Notes / Task Editor
- Vim-like modal editing: NORMAL, INSERT, and VISUAL modes
- Gap buffer data structure for O(1) text insertion and deletion
- Hierarchical notes and tasks with configurable nesting depth (0–3)
- Task toggling (`[ ]` / `[x]`)
- Add, delete, and reorder notes
- Promote and demote notes (change nesting level)
- Move mode for drag-and-drop reordering
- Undo and redo for all note operations
- Pagination with animated page transitions
- Text wrapping for long lines
- Automatic save and load on exit

#### Desktop Notifications
- System notifications via libnotify (Linux)
- Configurable notifications per stage: work, short break, long break, end of
  cycle
- Sound notifications alongside desktop alerts

#### Audio / White Noise
- White noise and ambient sound playback for focus sessions
- Per-track and master volume control
- Track types: Rain, Fire, Wind, Thunder
- Play and stop per track with persistence
- Notification sounds per pomodoro stage
- Vendored miniaudio.h — no system audio library dependency

#### Session History
- Binary log of all pomodoro sessions (28 bytes per record)
- GitHub-style activity contribution graph
- Day-by-day session detail view
- Statistics view with current and longest streaks
- Unix domain socket IPC for live timer status (`tomato -t`)
- CLI history overview (`tomato -h`)

#### Configuration System
- TOML-based configuration via vendored toml-c.h parser
- System config: `/etc/tomato/config.toml`
- User config: `~/.config/tomato/config.toml`
- Runtime preferences dialog — change settings without editing files
- Fully commented sample configuration

#### Keybinding System
- Fully configurable keybindings via TOML
- Mode-specific bindings: DEFAULT, NORMAL, INSERT, VISUAL
- Scene-aware dispatch with different bindings per scene
- Keybinding merging: system config + user config + built-in defaults

#### Error Handling
- Structured error type system with severity levels: DEBUG, INFO, WARNING,
  ERROR, CRITICAL
- Error stack with auto-clearing by timeout
- Critical error handling with freeze and quit confirmation

#### Build System
- POSIX make build with shared config.mk for compiler and linker flags
- Build script (`build.sh`) with debug, install, and output directory options
- Security hardening: `-fstack-protector-strong`, `-fPIE`,
  `-D_FORTIFY_SOURCE=2`, `-Wl,-z,relro,-z,now`
- Platform detection: Linux and macOS (Intel + ARM)

#### Testing
- 10 unit test suites: gap buffer, history, config, keybind, notes, util,
  error, log, anim, bar
- 5 integration test suites: pomodoro workflow, pomodoro timer, end-to-end,
  keybind resolution, keybind merge
- Test runner with filtering, color output, ASan, UBSan, and coverage support
- CI via GitHub Actions on Ubuntu, macOS Intel, and macOS ARM

#### Documentation
- Doxygen-generated API documentation
- Comprehensive README with quick start, usage, and contribution guide
- Fully commented sample configuration
- Development roadmap (TODO.md)

#### Cross-Platform Support
- Linux (primary)
- macOS (Intel and Apple Silicon)
- WSL (Windows Subsystem for Linux) compatibility mode

### Security

- Symlink-safe file writes via `O_NOFOLLOW` in `FOpenNoFollow`
- Stack canary (`-fstack-protector-strong`) and FORTIFY_SOURCE enabled for all
  builds
- Position-independent executable (`-fPIE`)
- RELRO and bind-now linking on Linux
- Non-executable stack on Linux
