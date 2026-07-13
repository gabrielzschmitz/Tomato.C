# TODO.md

This file tracks the development of the **Tomato.C** rewrite — a modular,
extensible, terminal-based Pomodoro timer written in C.

---

## ✅ Completed

### 🎞️ Animations
- [X] ASCII animations with frame timing, color and UNICODE support
- [X] Animation variants: nerd-icons, emojis, ASCII
- [X] Animation API: print to position, center, toggle loop

### 🖼️ UI Library
- [X] Modular UI library for rendering and layout management

### 📊 Status Bar
- [X] Modular status bar with left, center (auto-centered), and right zones
- [X] Each module implemented as an independent function

### 🗃️ Logging
- [X] Log library for Pomodoro session data
- [X] Efficient read/write via UNIX sockets
- [X] Fix `tomato -t` to be fully functional to place on bar

### 🔊 Audio
- [X] Audio playback library using `miniaudio.h`
- [X] Supports volume control and looping

### 🔔 Notifications
- [X] Desktop notification support using `libnotify`

### 🔧 Core Logic
- [X] Pomodoro loop (work, short/long breaks)
- [X] Pause, resume, and skip functionality

### ⚙️ Configuration
- [X] Modular config system for durations, break types, and user settings
- [X] Move configuration from compiled to runtime paradigma
- [X] Refactor the configuration system to prioritize user-defined settings
  over hardcoded defaults, particularly for keybindings.
- [X] Add Status Bar modules at user config
- [X] Add installation and relative paths where needed

### 📝 Simple note taking with vim-motions
- [X] Add and remove tasks
- [X] Move through tasks
- [X] Toggle tasks on/off
- [X] Manage breaklines and large lines
- [X] Edit tasks [X] Add and remove subtasks (max depth 1) [X] Move notes up and down [X] Undo and redo edits
- [X] Add a page system

### 🛠️ User Preferences
- [X] Save/load current session state
    - [X] Transform logging to be less data consuming (maybe binary)
    - [X] Add more interesting data about sessions to GetPomodoroLog
- [X] Add error stack and UI error line

### 👋 User Interface
- [X] Mouse interaction
- [X] Welcome screen
- [X] Continue screen
- [X] ESC/q close current existing popup instead of closing app.
- [X] Runtime preferences system and page
- [X] Pause emoji turns into play emoji when session paused
- [X] Make CLI history more like runtime history
- [X] Add help menu

### 🧩 Extra Features
- [X] White noise playback for focus sessions
- [X] Work history log (git-style commits)

### 🧪 Testing
- [X] Unit tests for all core modules
- [X] Integration testing
- [X] E2E testing

---

## 🚧 In Progress

### 📚 Documentation
- [X] Inline documentation for all modules
- [X] Documentation to HTML pages
- [ ] Updated README with install, usage, and config details
- [ ] Changelog for version tracking

---

## 🗓️ Planned

### 🧪 Testing
- [ ] User testing and feedback collection
