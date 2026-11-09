# Architecture

This document describes Tomato.C's process model, timer IPC, persistence
formats, and configuration merge rules. Public C APIs are documented in the
headers and generated Doxygen reference.

## Startup and main loop

`main()` loads configuration once, handles command-line modes, then initializes
the terminal and application state for the interactive UI. It starts a timer
server child process and waits for a readiness byte before entering the UI
loop.

Each interactive cycle performs these phases:

1. Set an input timeout: configured frame interval while animations run, or one
   second while animations are inactive.
2. Update timer, scene, status-bar, animation, and timer-log state.
3. Read input. The first read may wait for input or timeout; queued input drains
   without blocking.
4. Draw the screen when redraw is pending.

The loop uses curses input timeouts rather than a fixed idle polling delay.
Pomodoro time advances from elapsed monotonic time, so a delayed UI cycle can
catch up without losing timer ticks.

## Timer IPC

The timer server runs in a child process. It owns a Unix-domain stream socket at
`logging.timer_file` (default `tomato_timer.sock` in the runtime directory).
The parent passes a pipe descriptor to the child. The server writes `R` to that
pipe only after binding and listening; the UI waits for `R` before sending timer
updates.

The UI sends newline-terminated status strings through `SetTimerLog`. The
server stores the latest string and broadcasts updates to connected clients.
`tomato -t` is a client: it prints updates until the server disconnects.
`tomato -loop false -t` reads one update and exits. Identical UI status strings
are throttled to avoid redundant socket writes.

## Persistence formats

State paths default to `$XDG_STATE_HOME/tomato`, then
`$HOME/.local/state/tomato`. `TOMATO_STATE_DIR` overrides that location when it
contains an absolute path. The program falls back to a private per-user
`/tmp/tomato-<uid>` directory only when no home directory is available. TOML
logging paths can override individual files.

### `pomodoro.bin`

`pomodoro.bin` is a sequence of packed snapshots, normally appended. Abandoning
an unfinished session can rewrite the file to remove its snapshots. Each record
is 21 bytes. The file has no magic value, version header, or explicit byte-order
marker; fields use host byte order. Changing this layout requires a migration
or a new file format.

| Offset | Type | Field | Meaning |
| ---: | --- | --- | --- |
| 0 | `uint16_t` | `session_index` | Session identifier |
| 2 | `uint8_t` | `current_step` | `WORK_TIME=1`, `SHORT_PAUSE=2`, `LONG_PAUSE=3` |
| 3 | `uint8_t` | `current_cycle` | Current cycle, stored one-based |
| 4 | `uint8_t` | `total_cycles` | Cycles per session |
| 5 | `uint8_t` | `work_time` | Work duration in minutes |
| 6 | `uint8_t` | `short_pause_time` | Short break in minutes |
| 7 | `uint8_t` | `long_pause_time` | Long break in minutes |
| 8 | `uint32_t` | `total_elapsed` | Elapsed session seconds |
| 12 | `uint32_t` | `current_step_time` | Elapsed current-step seconds |
| 16 | `uint8_t` | `status` | `0` completed, `1` unfinished |
| 17 | `uint32_t` | `session_start_time` | Unix timestamp in seconds |

Records are snapshots, not one record per completed session. Resume reads the
last complete record and validates it; an invalid last record is not replaced
by an older snapshot. History queries group snapshots by session or calendar
date as needed.

### `notes.log` and `errors.log`

`notes.log` is newline-delimited text. Each row stores
`id|page_id|depth|parent_id|state|text`; a literal `|` in note text is escaped
as `\|`. Saves write a temporary file and rename it over the old file.

`errors.log` is append-only text. Each line contains severity, context, message,
and numeric error code.

## Configuration precedence and merge rules

Configuration is loaded once per process. `LoadConfig()` ignores later calls.
Precedence, from lowest to highest, is:

1. Built-in defaults.
2. `/etc/tomato/config.toml`.
3. User configuration selected by `TOMATO_CONFIG_DIR`,
   `XDG_CONFIG_HOME`, or `HOME`.

For scalar and array settings, later files override earlier values. Unspecified
values keep the value already loaded.

Keybindings merge by action, input mode, and scene. A TOML key list replaces
the key list for that matching group; it removes omitted default keys from that
group. Unrelated groups remain unchanged. New groups are added. Existing order
is retained where possible because the first matching binding wins during
dispatch.

Runtime preference edits can update the active application state. Saving them
writes the corresponding TOML values; it does not reload configuration from
disk.
