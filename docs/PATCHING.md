# Patching

Users are encouraged to create patches for Tomato.C. Each patchable feature has
its own subsection in this file.

## Animations

### Animation Slots

| Slot                          | Default file                            | Default name       |
| ----------------------------- | --------------------------------------- | ------------------ |
| `animations.mainmenu`         | `$DATADIR/sprites/mainmenu.asc`         | `mainmenu`         |
| `animations.worktime`         | `$DATADIR/sprites/worktime.asc`         | `worktime`         |
| `animations.shortpause`       | `$DATADIR/sprites/shortpause.asc`       | `shortpause`       |
| `animations.longpause`        | `$DATADIR/sprites/longpause.asc`        | `longpause`        |
| `animations.notes`            | `$DATADIR/sprites/notes.asc`            | `notes`            |
| `animations.notes_transition` | `$DATADIR/sprites/notes_transition.asc` | `notes_transition` |
| `animations.help`             | `$DATADIR/sprites/help.asc`             | `help`             |
| `animations.continue`         | `$DATADIR/sprites/continue.asc`         | `continue`         |

### Config

```toml
[animations.worktime]
name = "worktime"                              # [animation.NAME] in .asc
filename = "~/.config/tomato/sprites/work.asc" # sprite file
default_frame = 0                              # 0-based static frame
```

Path expansion: `$DATADIR`, `$HOME`, `$XDG_CONFIG_HOME`, `$XDG_DATA_HOME`, `~`.
Non-absolute paths resolve relative to config file dir.

### `.asc` Format

```text
[animation.worktime]
icon-type = "nerd-icons"        # nerd-icons | emojis | ascii
default-frame-duration = "1.0s"
loop = true
default-color = 7

[[frame]]
frame-duration = "0.5s"         # optional override
[[content]]
\c{2}text \u{ea71}
\c{1.0}fg.bg
\s \{4}
[[/content]]
[[/frame]]
```

| Escape    | Meaning                                  |
| --------- | ---------------------------------------- |
| `\c{N}`   | Set foreground color `N`                 |
| `\c{F.B}` | Set foreground `F`, background `B`       |
| `\u{HEX}` | Unicode codepoint (hex), e.g. `\u{ea71}` |
| `\s`      | One transparent blank cell               |
| `\{N}`    | `N` transparent blank cells              |
