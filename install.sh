#!/bin/sh
#/**
# * @file install.sh
# * @brief Tomato.C Installer / Uninstaller.
# *
# * Detects the operating system, installs build dependencies, compiles
# * the binary with the correct data path, and installs it system-wide.
# * Also supports removing installed files via --uninstall.
# * Supports Debian/Ubuntu, Fedora/RHEL, Arch/CachyOS, Alpine, openSUSE,
# * and macOS (Homebrew).
# *
# * Usage: ./install.sh [--prefix=<dir>] [--destdir=<dir>]
# *        ./install.sh --uninstall [--prefix=<dir>] [--destdir=<dir>]
# *
# * Options:
# *   --prefix=<dir>   Installation prefix (default: /usr/local)
# *   --destdir=<dir>  Staging directory for packaged installs
# *   --uninstall      Remove installed files (binary, data, desktop entry)
# *   --help|-h        Show this help
# *
# * Exit codes: 0 = success, nonzero = failure
# */

set -u

#/**
# * ---------------------------------------------------------------------------
# * Global state
# * ---------------------------------------------------------------------------
# */

PREFIX="/usr/local"
DESTDIR=""
UNINSTALL=0
OS_ID=""; OS_NAME=""; PKG_MANAGER=""

#/**
# * ---------------------------------------------------------------------------
# * Color support
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Initialise ANSI colour variables if output is a TTY and
# *        NO_COLOR is not set.
# */
init_colors() {
  RED=''; GREEN=''; YELLOW=''; MAGENTA=''; CYAN=''; BOLD=''; RESET=''
  if [ -t 1 ] && [ "${NO_COLOR:-}" = "" ]; then
    if command -v tput >/dev/null 2>&1 && tput colors 2>/dev/null | grep -q '[0-9]'; then
      RED=$(tput setaf 1)
      GREEN=$(tput setaf 2)
      YELLOW=$(tput setaf 3)
      MAGENTA=$(tput setaf 5)
      CYAN=$(tput setaf 6)
      BOLD=$(tput bold)
      RESET=$(tput sgr0)
    fi
  fi
}

#/**
# * ---------------------------------------------------------------------------
# * Argument parsing
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Print usage information and exit.
# */
usage() {
  echo ""
  echo "${BOLD}Tomato.C Installer${RESET}"
  echo ""
  echo "Usage: $0 [--prefix=<dir>] [--destdir=<dir>]"
  echo "       $0 --uninstall [--prefix=<dir>] [--destdir=<dir>]"
  echo ""
  echo "  --prefix=<dir>   Installation prefix (default: /usr/local)"
  echo "  --destdir=<dir>  Staging directory for packaged installs"
  echo "  --uninstall      Remove installed files (binary, data, desktop entry)"
  echo "  --help|-h        Show this help"
  echo ""
  exit 0
}

#/**
# * @brief Parse command-line arguments and set global PREFIX / DESTDIR.
# * @param $@  Arguments from the command line.
# */
parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      --prefix=*)
        PREFIX="${1#*=}"
        ;;
      --destdir=*)
        DESTDIR="${1#*=}"
        ;;
      --uninstall)
        UNINSTALL=1
        ;;
      --help|-h)
        usage
        ;;
      *)
        echo "${RED} Unknown option: $1${RESET}" >&2
        echo " Try $0 --help for available options." >&2
        exit 1
        ;;
    esac
    shift
  done

  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo " ${BOLD}Tomato.C Installer${RESET}"
  echo "${MAGENTA}═══════════════════════════════════════════════════════════════${RESET}"
  echo ""
  echo " ${BOLD}Arguments:${RESET}"
  echo "   Prefix : ${PREFIX}"
  echo "   Destdir: ${DESTDIR:-"(none)"}"
  echo ""
}

#/**
# * ---------------------------------------------------------------------------
# * OS detection
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Detect the operating system and distribution.
# *
# * Uses /etc/os-release on Linux, sw_vers on macOS.
# * @return Sets global: OS_ID, OS_NAME, PKG_MANAGER.  Exits on failure.
# */
detect_os() {
  echo " ${CYAN}[DETECT]${RESET} Identifying operating system..."

  if [ "$(uname -s)" = "Darwin" ]; then
    OS_ID="macos"
    OS_NAME="macOS"
    PKG_MANAGER="brew"
  elif [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_ID="$ID"
    OS_NAME="$NAME"
    case "$ID" in
      debian|ubuntu)        PKG_MANAGER="apt"    ;;
      rhel|fedora|centos)   PKG_MANAGER="dnf"    ;;
      arch|manjaro|cachyos) PKG_MANAGER="pacman" ;;
      void)                 PKG_MANAGER="xbps"   ;;
      alpine)               PKG_MANAGER="apk"    ;;
      opensuse*|suse)       PKG_MANAGER="zypper" ;;
      *)
        echo " ${RED}[ERR]${RESET}  Unsupported distribution: ${BOLD}$ID${RESET}" >&2
        echo "     Install dependencies manually, then run ./build.sh" >&2
        exit 1
        ;;
    esac
  else
    echo " ${RED}[ERR]${RESET}  Cannot detect operating system" >&2
    exit 1
  fi

  echo " ${GREEN}[OK]${RESET}  ${BOLD}$OS_NAME${RESET} (${OS_ID}) — ${CYAN}${PKG_MANAGER}${RESET}"
  echo ""
}

#/**
# * ---------------------------------------------------------------------------
# * Dependencies
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Print the package list required to build Tomato.C on the
# *        detected OS.
# *
# * @return Prints a space-separated list of packages to stdout.
# */
check_dependencies() {
  case "$PKG_MANAGER" in
    apt)    echo "build-essential pkg-config libncursesw5-dev libnotify-dev"     ;;
    dnf)    echo "gcc make pkgconfig ncurses-devel libnotify-devel"              ;;
    pacman) echo "gcc make pkg-config ncurses libnotify"                         ;;
    apk)    echo "gcc make pkgconfig musl-dev ncurses-dev libnotify-dev"         ;;
    xbps)   echo "gcc make pkg-config ncurses libnotify"                         ;;
    zypper) echo "gcc make pkg-config ncurses-devel libnotify-devel"             ;;
    brew)   echo "ncurses"                                                       ;;
  esac
}

#/**
# * @brief Install build dependencies using the system package manager.
# *
# * Uses sudo on Linux. On macOS, ensures Xcode Command Line Tools and
# * Homebrew are present before installing.
# */
install_dependencies() {
  echo " ${CYAN}[DEPS]${RESET} Installing build dependencies..."

  local packages
  packages=$(check_dependencies)
  echo "   Packages: ${BOLD}$packages${RESET}"

  case "$PKG_MANAGER" in
    apt)
      sudo apt update -qq && sudo apt install -y $packages
      ;;
    dnf)
      sudo dnf install -y $packages
      ;;
    pacman)
      sudo pacman -S --needed --noconfirm $packages
      ;;
    apk)
      sudo apk add $packages
      ;;
    xbps)
      sudo xbps-install -S $packages
      ;;
    zypper)
      sudo zypper install -y $packages
      ;;
    brew)
      if ! xcode-select -p >/dev/null 2>&1; then
        echo "   Xcode Command Line Tools not found."
        xcode-select --install
        echo "   Please re-run install.sh after the installation completes."
        exit 1
      fi
      if ! command -v brew >/dev/null 2>&1; then
        echo " ${RED}[ERR]${RESET}  Homebrew is required. Install from https://brew.sh" >&2
        exit 1
      fi
      brew install $packages
      ;;
  esac

  if [ $? -eq 0 ]; then
    echo " ${GREEN}[OK]${RESET}  Dependencies installed"
  else
    echo " ${RED}[ERR]${RESET}  Failed to install dependencies" >&2
    exit 1
  fi
  echo ""
}

#/**
# * ---------------------------------------------------------------------------
# * Build
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Compile Tomato.C via build.sh with the install DATAPREFIX.
# *
# * DATAPREFIX is set so the binary embeds the absolute installed data
# * directory path rather than the default ./resources.
# */
build_project() {
  echo " ${CYAN}[BUILD]${RESET} Compiling Tomato.C..."

  export DATAPREFIX="${DESTDIR}${PREFIX}/share/tomato"
  if ! ./build.sh; then
    echo " ${RED}[ERR]${RESET}  Build failed" >&2
    exit 1
  fi

  echo " ${GREEN}[OK]${RESET}  Build successful"
  echo ""
}

#/**
# * ---------------------------------------------------------------------------
# * Install
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Install the binary, data files, and desktop entry to the
# *        system prefix.
# *
# * All file operations use sudo for privilege escalation.
# */
install_files() {
  local install_dir="${DESTDIR}${PREFIX}"

  echo " ${CYAN}[INSTALL]${RESET} Installing to ${BOLD}${install_dir}${RESET}..."

  # Binary
  sudo mkdir -p "${install_dir}/bin" || exit 1
  sudo cp tomato "${install_dir}/bin/tomato" || exit 1
  sudo chmod 755 "${install_dir}/bin/tomato" || exit 1
  echo "   ${GREEN}[OK]${RESET}  Binary → ${install_dir}/bin/tomato"

  # Data files
  local data_dir="${install_dir}/share/tomato"
  sudo mkdir -p "$data_dir/sprites" "$data_dir/sounds" "$data_dir/icons" || exit 1

  sudo cp resources/sprites/*.asc "$data_dir/sprites/" 2>/dev/null
  sudo cp resources/sounds/*.mp3 "$data_dir/sounds/" 2>/dev/null
  sudo cp resources/icons/* "$data_dir/icons/" 2>/dev/null

  sudo chmod 644 "$data_dir/sprites/"* 2>/dev/null
  sudo chmod 644 "$data_dir/sounds/"* 2>/dev/null
  sudo chmod 644 "$data_dir/icons/"* 2>/dev/null
  echo "   ${GREEN}[OK]${RESET}  Data   → ${data_dir}"

  # Desktop file (Linux only)
  if [ "$(uname -s)" = "Linux" ]; then
    sudo mkdir -p "${install_dir}/share/applications" || exit 1
    sudo cp tomato.desktop "${install_dir}/share/applications/tomato.desktop" || exit 1
    sudo sed -i "s|Icon=.*|Icon=${data_dir}/icons/tomato.svg|" \
      "${install_dir}/share/applications/tomato.desktop" || exit 1
    sudo chmod 644 "${install_dir}/share/applications/tomato.desktop" || exit 1
    echo "   ${GREEN}[OK]${RESET}  Desktop → ${install_dir}/share/applications/tomato.desktop"
  fi

  # Sample config
  local config_dir="${HOME}/.config/tomato"
  if [ ! -f "${config_dir}/config.toml" ]; then
    mkdir -p "${config_dir}"
    cp sample_config.toml "${config_dir}/config.toml"
    echo "   ${GREEN}[OK]${RESET}  Config → ${config_dir}/config.toml"
  else
    echo "   ${YELLOW}[WARN]${RESET}  Config exists at ${config_dir}/config.toml — skipping"
  fi

  echo ""
  echo " ${GREEN}${BOLD}Installation complete!${RESET}"
  echo " Run ${BOLD}tomato${RESET} to start the Pomodoro timer."
}

#/**
# * ---------------------------------------------------------------------------
# * Uninstall
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Remove installed files from the system prefix.
# *
# * Deletes the binary, the entire share/tomato data directory,
# * and the desktop entry (Linux only).  The user config at
# * ~/.config/tomato is kept.
# */
uninstall_files() {
  local install_dir="${DESTDIR}${PREFIX}"

  echo " ${CYAN}[UNINSTALL]${RESET} Removing from ${BOLD}${install_dir}${RESET}..."

  sudo rm -f "${install_dir}/bin/tomato"
  echo "   ${GREEN}[OK]${RESET}  Removed ${install_dir}/bin/tomato"

  sudo rm -rf "${install_dir}/share/tomato"
  echo "   ${GREEN}[OK]${RESET}  Removed ${install_dir}/share/tomato"

  if [ "$(uname -s)" = "Linux" ]; then
    sudo rm -f "${install_dir}/share/applications/tomato.desktop"
    echo "   ${GREEN}[OK]${RESET}  Removed ${install_dir}/share/applications/tomato.desktop"
  fi

  echo ""
  echo " ${GREEN}${BOLD}Uninstall complete.${RESET}"
  echo " User config at ~/.config/tomato was kept."
}

#/**
# * ---------------------------------------------------------------------------
# * Main
# * ---------------------------------------------------------------------------
# */

#/**
# * @brief Orchestrate the full install or uninstall workflow.
# *
# * When --uninstall is given, only removes installed files.
# * Otherwise, initialises colours, parses arguments, detects the OS,
# * installs build dependencies, compiles the project, then installs
# * system-wide.
# *
# * @param $@  Arguments forwarded from the shell.
# */
main() {
  # If run via curl pipe (no local clone), clone the repo first
  if [ ! -f "build.sh" ]; then
    echo "Source not found — cloning repository..."
    if ! command -v git >/dev/null 2>&1; then
      echo "${RED}[ERR]${RESET}  git is required. Install git and try again." >&2
      exit 1
    fi
    git clone --depth=1 https://github.com/gabrielzschmitz/Tomato.C.git /tmp/tomato-install
    cd /tmp/tomato-install
    exec ./install.sh "$@"
  fi

  init_colors
  parse_args "$@"

  if [ "$UNINSTALL" -eq 1 ]; then
    uninstall_files
    exit 0
  fi

  detect_os
  install_dependencies
  build_project
  install_files
}

main "$@"
