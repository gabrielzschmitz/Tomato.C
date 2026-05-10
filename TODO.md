# TODO.md

This file tracks the development of the **Tomato.C** rewrite — a modular,
extensible, terminal-based Pomodoro timer written in C.

---

## ✅ Completed

### 🎞️ Animations
- [x] ASCII animations with frame timing and color support
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

### 👨🏻‍💻 UI Behavior
- [x] Auto-centering for all interface components

### ⚙️ Configuration
- [X] Modular config system for durations, break types, and user settings

---

## 🚧 In Progress

- [ ] Simple in-app note-taking
  - [X] Add and remove tasks
  - [X] Move through tasks
  - [X] Toggle tasks on/off
  - [X] Manage breaklines and large lines
  - [X] Edit tasks
  - [ ] Add and remove subtasks (max depth 1)
  - [ ] Undo and redo tasks

---

## 🗓️ Planned

### 👋 User Interface
- [ ] Welcome screen
- [ ] Mouse interaction

### 🛠️ User Preferences
- [ ] Save/load current session state
- [ ] Modular preferences system

### 🧩 Extra Features
- [ ] White noise playback for focus sessions
- [ ] Work history log (git-style commits)

### ⚙️ Configuration
- [ ] Move configuration from compile time to run time paradigma

---

## 🧪 Testing

- [ ] Unit tests for all core modules
- [ ] Integration testing
- [ ] User testing and feedback collection

---

## 📚 Documentation

- [ ] Updated README with install, usage, and config details
- [ ] Inline documentation for all modules
- [ ] Changelog for version tracking
