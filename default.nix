#         .             .              .
#         |             |              |           .
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
#  ,|
#  `'
# default.nix
{}: let
  pkgs = import <nixpkgs> {};
  lib = pkgs.lib;
in
  pkgs.stdenv.mkDerivation {
    name = "tomato";

    src = ./.;

    installPhase = ''
      mkdir -p $out/bin && cp tomato $out/bin/

      ln -s $(which notify-send) $out/bin/
      ln -s $(which mpv) $out/bin/
    '';

    buildInputs = with pkgs; [
      gcc
      which
      gnumake
      ncurses
    ];

    propagatedBuildInputs = with pkgs; [
      libnotify
      mpv
    ];

    meta = with lib; {
      description = "A pomodoro timer written in pure C.";
      homepage = "https://github.com/gabrielzschmitz/Tomato.C";
      license = licenses.mit;
      maintainers = with maintainers; [luisnquin];
    };
  }
