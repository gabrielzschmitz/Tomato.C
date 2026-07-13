# Tomato.C

<img align="right" width="192px" src="./resources/icons/tomato.svg" alt="Tomato.C Logo">

<a href="./LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-blue" alt="License"></a>
<a href="https://www.buymeacoffee.com/gabrielzschmitz" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 20px !important;width: 87px;" ></a>
<a href="https://github.com/gabrielzschmitz/Tomato.C"><img src="https://img.shields.io/github/stars/gabrielzschmitz/Tomato.C?style=social" alt="Give me a Star"></a>

**Tomato.C** is a modular, extensible, terminal-based Pomodoro timer written in
pure C. Its features dynamic UI, ASCII sprite animations and notifications. The
application integrates a robust productivity workflow featuring a built-in
notes system with vim-like motions and comprehensive session history logging.

---

## Quick Start

### 1. Clone the repository

```bash
git clone https://github.com/gabrielzschmitz/Tomato.C.git
cd Tomato.C
```

### 2. Build and run

```bash
./build.sh
./tomato
```

### 3. Install and run

```bash
sudo ./build.sh --install
tomato
```

---

## Usage

Pomodoro controls and navigation are handled via keyboard shortcuts and mouse
input. To get help with the keybinds press `?`/`F1`

<p align="center">
  <img src="./resources/demo.gif" alt="Tomato.C demonstration" style="width: 484px; border: 3px solid #e06e6e; padding: 0;">
</p>
<p align="center">
  <em>
  Tomato.C combines modern productivity features with a clean, retro-inspired
  terminal interface.
  </em>
</p>

---

## Contribute

Feel free to contribute to the project, the requirement is to follow
conventional commit and use the `.clang-format` to format the files!

---

## Documentation

Complete API and developer documentation is generated with **Doxygen**.
Generate the documentation locally:

```bash
cd docs
./generate.sh
$BROWSER output/html/index.html
```

---

## License

This project is licensed under the GNU General Public License v3.0. See the
[LICENSE](./LICENSE) file for details.
