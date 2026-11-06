#/**
# * @file shell.nix
# * @brief Classic nix-shell (non-flake) development environment.
# *
# * Provides the same toolchain as the flake's devShell, usable with the
# * plain `nix-shell` command on systems that do not use flakes.
# *
# * Usage:
# *   nix-shell          # enter the development shell
# *   nix-shell --run "./build.sh && ./run_tests.sh"
# */

{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  inputsFrom = [ (pkgs.callPackage ./default.nix { }) ];
  packages = with pkgs; [
    # Build essentials
    gcc gnumake pkg-config ncurses libnotify
    # Shell utilities used by build.sh / run_tests.sh
    coreutils gnused gnugrep gawk diffutils findutils which
    # Optional development tooling referenced in the README
    bear clang-tools doxygen tinycc zig
    # Runtime / debugging helpers
    git dbus dunst valgrind gdb
  ];

  shellHook = ''
    TOMATO_DEV_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
    export TOMATO_DEV_ROOT
    export XDG_CONFIG_HOME="$TOMATO_DEV_ROOT/.tmp/config"
    export XDG_STATE_HOME="$TOMATO_DEV_ROOT/.tmp/state"
    export XDG_RUNTIME_DIR="$TOMATO_DEV_ROOT/.tmp/runtime"
    mkdir -p "$XDG_CONFIG_HOME/tomato" "$XDG_STATE_HOME" "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
    echo "Tomato.C dev files: $TOMATO_DEV_ROOT/.tmp"
  '';
}
