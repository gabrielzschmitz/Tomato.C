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
}
