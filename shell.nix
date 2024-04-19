# Use it with 'arrterian.nix-env-selector'
{pkgs ? import <nixpkgs> {}}:
with pkgs;
  mkShell {
    buildInputs = [
      pkg-config
      gnumake
      ncurses
      which
      gcc

      cppcheck
      clang
    ];

    propagatedBuildInputs = [
      libnotify
      mpv
    ];
  }
