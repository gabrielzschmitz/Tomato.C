#/**
# * @file flake.nix
# * @brief Nix flake for Tomato.C.
# *
# * Exposes GCC, TinyCC, and Zig packages, a development shell, and compiler
# * checks. Dependency-free: only nixpkgs is an input (no flake-utils).
# *
# * Usage:
# *   nix build .#        — build the GCC package (result/bin/tomato)
# *   nix build .#tcc     — build with TinyCC
# *   nix build .#zig     — build with Zig's C compiler
# *   nix run   .#        — run the binary
# *   nix develop         — enter the development shell
# *   nix flake check     — build and test all three compiler variants
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
        system: pkgs:
        let
          tomato = pkgs.callPackage ./default.nix { };
          withCompiler =
            {
              name,
              compiler,
              package,
            }:
            tomato.overrideAttrs (old: {
              pname = "tomato-c-${name}";
              nativeBuildInputs = old.nativeBuildInputs ++ [ package ];
              buildPhase = ''
                runHook preBuild
                make -C build PREFIX="$out" DATAPREFIX="$out/share/tomato" CC="${compiler}"
                runHook postBuild
              '';
            });
        in
        {
          default = tomato;
          tomato = tomato;
          tcc = withCompiler {
            name = "tcc";
            compiler = "tcc";
            package = pkgs.tinycc;
          };
          zig = withCompiler {
            name = "zig";
            compiler = "zig cc";
            package = pkgs.zig;
          };
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
        system: pkgs:
        let
          compilerCheck =
            {
              name,
              compiler,
              compilerPackage,
              tomatoPackage,
            }:
            pkgs.runCommand "tomato-c-${name}-check"
              {
                nativeBuildInputs = [
                  tomatoPackage
                  compilerPackage
                  pkgs.coreutils
                  pkgs.findutils
                  pkgs.gawk
                  pkgs.gnugrep
                  pkgs.gnumake
                  pkgs.gnused
                  pkgs.pkg-config
                ];
                buildInputs = with pkgs; [
                  ncurses
                  libnotify
                  libpulseaudio
                  alsa-lib
                  glib
                  gdk-pixbuf
                ];
              }
              ''
                set -eu
                mkdir -p "$TMPDIR/source" "$TMPDIR/home" "$TMPDIR/config/tomato" \
                  "$TMPDIR/state/tomato" "$TMPDIR/runtime"
                export HOME="$TMPDIR/home"
                export XDG_CONFIG_HOME="$TMPDIR/config"
                export XDG_STATE_HOME="$TMPDIR/state"
                export TOMATO_CONFIG_DIR="$TMPDIR/config/tomato"
                export TOMATO_STATE_DIR="$TMPDIR/state/tomato"
                export TOMATO_RUNTIME_DIR="$TMPDIR/runtime"
                chmod 700 "$TOMATO_RUNTIME_DIR"

                # Smoke-test the packaged CLI; this also checks ELF startup.
                tomato --version > "$TMPDIR/version.txt"
                grep -q "^tomato " "$TMPDIR/version.txt"
                if tomato bogusarg > "$TMPDIR/bogusarg.txt" 2>&1; then
                  echo "unknown argument must exit nonzero" >&2
                  exit 1
                fi
                grep -iq "usage" "$TMPDIR/bogusarg.txt"

                cp -R ${self}/. "$TMPDIR/source/"
                chmod -R u+w "$TMPDIR/source"
                cd "$TMPDIR/source"
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

                CC="${compiler}" TEST_CC="${compiler}" ./run_tests.sh --quiet
                touch "$out"
              '';
        in
        {
          build = compilerCheck {
            name = "gcc";
            compiler = "gcc";
            compilerPackage = pkgs.gcc;
            tomatoPackage = self.packages.${system}.default;
          };
          tcc = compilerCheck {
            name = "tcc";
            compiler = "tcc";
            compilerPackage = pkgs.tinycc;
            tomatoPackage = self.packages.${system}.tcc;
          };
          zig = compilerCheck {
            name = "zig";
            compiler = "zig cc";
            compilerPackage = pkgs.zig;
            tomatoPackage = self.packages.${system}.zig;
          };
        }
      );
    };
}
