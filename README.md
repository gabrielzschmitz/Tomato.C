# Tomato.C

<img align="right" width="192px" src="./resources/icons/tomato.svg" alt="Tomato.C Logo">

<a href="./LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-blue" alt="License"></a>
<a href="https://www.buymeacoffee.com/gabrielzschmitz" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 20px !important;width: 87px;" ></a>
<a href="https://github.com/gabrielzschmitz/Tomato.C"><img src="https://img.shields.io/github/stars/gabrielzschmitz/Tomato.C?style=social" alt="Give me a Star"></a>

**Tomato.C** is a modular, extensible, terminal-based Pomodoro timer written in
pure C. Its features dynamic UI, ASCII sprite animations, desktop
notifications, built-in notes system with Vim-like motions and comprehensive
session history logging.

---

## Installation

Choose the installation method that best fits your needs. The **install
script** is the quickest option for Linux and macOS. **Building from source**
is ideal if you want to customize or contribute.

<details>
<summary>Quick install script</summary>

```bash
curl -sSL https://raw.githubusercontent.com/gabrielzschmitz/Tomato.C/main/install.sh | bash
```

</details>

<details>
<summary>Build from source</summary>

```bash
git clone https://github.com/gabrielzschmitz/Tomato.C.git
cd Tomato.C
./build.sh
# And/or
./install.sh
```

By default the build uses [gcc](https://gcc.gnu.org/). To use
[tcc](https://bellard.org/tcc) (Tiny C Compiler) or [zig](https://ziglang.org)
as an alternative, pass it on the command line:

```bash
CC=tcc ./build.sh
# And/or
CC=tcc ./install.sh
```

```bash
CC="zig cc" ./build.sh
# And/or
CC="zig cc" ./install.sh
```

Alternatively for a more permanent change, edit `CC = gcc` in `build/config.mk`.

</details>

<details>
<summary>macOS with Homebrew</summary>

Install Xcode Command Line Tools and [Homebrew](https://brew.sh). The quick
installer above detects Homebrew and installs `ncurses` automatically:

```bash
xcode-select --install
curl -sSL https://raw.githubusercontent.com/gabrielzschmitz/Tomato.C/main/install.sh | bash
```

To build without installing system-wide, install `ncurses` and build from a
checkout:

```bash
brew install ncurses
git clone https://github.com/gabrielzschmitz/Tomato.C.git
cd Tomato.C
./build.sh
```

</details>

<details>
<summary>Uninstall</summary>

From local repo:

```bash
sudo ./install.sh --uninstall
```

Via curl:

```bash
curl -sSL https://raw.githubusercontent.com/gabrielzschmitz/Tomato.C/main/install.sh | bash -s -- --uninstall
```

</details>

---

## Usage

Use keyboard shortcuts or mouse input to navigate Tomato.C. Press `?` or
`F1` at any time to view the available keybindings.

### Command-line reference

```text
tomato [--help] [--version]
tomato [-loop true|false] -t [nerd-icons|emojis|ascii|no-icons]
tomato -h [overview|today]
```

- `-t`: Stream timer status.
- `-loop false -t`: Print one status and exit.
- `-h [overview|today]`: Display session history.
- `--help` / `--version`: Display help or version information.

Timer and history output require their respective logging options to be enabled.

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

## Configuration

Tomato.C uses a TOML configuration file. A fully commented sample is created at
`~/.config/tomato/config.toml` when installed with `install.sh`.

For configuration paths, precedence, and persistence details, see
[Architecture](ARCHITECTURE.md).

### Troubleshooting

- **Notifications:** Ensure your desktop notification service is running.
- **Notification sound:** Check `notifications.sound` and
  `notifications.sound_volume`.
- **Missing glyphs:** Use a compatible Nerd Font or emoji font, or set
  `visual.icons = "ascii"`.
- **Small terminal:** Enlarge the terminal to meet the displayed minimum
  dimensions.

---

## Documentation

API documentation is generated with **Doxygen**. See
[Architecture](ARCHITECTURE.md) for design details and
[Contributing](CONTRIBUTE.md) for development guidelines.

<p align="center">
  <img src="./resources/docs.png" alt="Tomato.C documentation" style="width: 484px; border: 3px solid #e06e6e; padding: 0;">
</p>
<p align="center">
  <em>
  Doxygen-generated API documentation for Tomato.C.
  </em>
</p>

Generate locally:

```bash
cd docs
./generate.sh
$BROWSER output/html/index.html
```

---

## Contribute

See [CONTRIBUTE.md](CONTRIBUTE.md) for development setup, build instructions,
tests, and contribution guidelines.

---

## License

This project is licensed under the GNU General Public License v3.0. See the
[LICENSE](./LICENSE) file for details.
