# TODO.md

This file tracks the development of the **Tomato.C** rewrite — a modular,
extensible, terminal-based Pomodoro timer written in C.

---

## ✅ Completed

### 🎞️ Animations
- [x] ASCII animations with frame timing, color and UNICODE support
- [x] Animation variants: nerd-icons, emojis, ASCII
- [x] Animation API: print to position, center, toggle loop

### 🖼️ UI Library
- [x] Modular UI library for rendering and layout management

### 📊 Status Bar
- [x] Modular status bar with left, center (auto-centered), and right zones
- [x] Each module implemented as an independent function

### 🗃️ Logging
- [x] Log library for Pomodoro session data
- [x] Efficient read/write via UNIX sockets

### 🔊 Audio
- [x] Audio playback library using `miniaudio.h`
- [x] Supports volume control and looping

### 🔔 Notifications
- [x] Desktop notification support using `libnotify`

### 🔧 Core Logic
- [x] Pomodoro loop (work, short/long breaks)
- [x] Pause, resume, and skip functionality

### ⚙️ Configuration
- [X] Modular config system for durations, break types, and user settings
- [X] Move configuration from compiled to runtime paradigma

### 📝 Simple note taking with vim-motions
- [X] Add and remove tasks
- [X] Move through tasks
- [X] Toggle tasks on/off
- [X] Manage breaklines and large lines
- [X] Edit tasks
- [X] Add and remove subtasks (max depth 1)
- [X] Move notes up and down
- [X] Undo and redo edits

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

---

## 🚧 In Progress

### 🧪 Testing
- [ ] Unit tests for all core modules
- [ ] Integration testing

---

### 📚 Documentation
- [X] Inline documentation for all modules
- [X] Documentation to HTML pages
- [ ] Updated README with install, usage, and config details
- [ ] Changelog for version tracking

## 🗓️ Planned

### 📝 Simple note taking with vim-motions
- [ ] Add a page system

### 🧪 Testing
- [ ] User testing and feedback collection
