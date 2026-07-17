# Changelog

All notable changes to this project will be documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
