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

{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  inputsFrom = [ (pkgs.callPackage ./default.nix { }) ];
  packages = with pkgs; [
    # Build essentials
    gcc
    gnumake
    pkg-config
    ncurses
    libnotify
    glib
    gdk-pixbuf
    # Shell utilities used by build.sh / run_tests.sh
    coreutils
    gnused
    gnugrep
    gawk
    diffutils
    findutils
    which
    # Optional development tooling referenced in the README
    bear
    clang-tools
    doxygen
    tinycc
    zig
    # Runtime / debugging helpers
    git
    dbus
    dunst
    valgrind
    gdb
  ];

  shellHook = ''
    TOMATO_DEV_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
    export TOMATO_DEV_ROOT

    export TOMATO_CONFIG_DIR="$TOMATO_DEV_ROOT/.tmp/config/tomato"
    export TOMATO_STATE_DIR="$TOMATO_DEV_ROOT/.tmp/state/tomato"
    export TOMATO_RUNTIME_DIR="$TOMATO_DEV_ROOT/.tmp/runtime"

    mkdir -p "$TOMATO_CONFIG_DIR" "$TOMATO_STATE_DIR" "$TOMATO_RUNTIME_DIR"
    chmod 700 "$TOMATO_RUNTIME_DIR"

    # Runtime shared libraries
    export LD_LIBRARY_PATH="${
      pkgs.lib.makeLibraryPath [
        pkgs.ncurses
        pkgs.libnotify
        pkgs.libpulseaudio
        pkgs.alsa-lib
        pkgs.glib
        pkgs.gdk-pixbuf
      ]
    }:''${LD_LIBRARY_PATH:-}"

    echo "Tomato.C dev files: $TOMATO_DEV_ROOT/.tmp"
  '';
}
