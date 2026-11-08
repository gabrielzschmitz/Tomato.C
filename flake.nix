#/**
# * @file flake.nix
# * @brief Nix flake for Tomato.C.
# *
# * Exposes the Tomato.C package, a development shell, and a build smoke
# * check.  Dependency-free: only nixpkgs is an input (no flake-utils).
# *
# * Usage:
# *   nix build .#        — build the package (result/bin/tomato)
# *   nix run   .#        — run the binary
# *   nix develop         — enter the development shell
# *   nix flake check     — build + smoke test
# */

{
  description = "Tomato.C — a modular, extensible terminal-based Pomodoro timer in pure C";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (
        system: pkgs: {
          default = pkgs.callPackage ./default.nix { };
          tomato = pkgs.callPackage ./default.nix { };
        }
      );

      devShells = forAllSystems (
        system: pkgs: {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
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
          };
        }
      );

      checks = forAllSystems (
        system: pkgs: {
          build =
            pkgs.runCommand "tomato-c-build-check" { nativeBuildInputs = [ self.packages.${system}.default ]; }
              ''
                # Running with an unknown argument prints usage and exits 0,
                # so this smoke-tests the binary without a TTY.
                tomato bogusarg 2>&1 | grep -q "usage"
                touch "$out"
              '';
        }
      );
    };
}
