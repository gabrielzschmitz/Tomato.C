#         .             .              .
#         |             |              |           .
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
#  ,|
#  `'
# default.nix
{pkgs ? import <nixpkgs> {}}: let
  repository-url = "https://github.com/gabrielzschmitz/Tomato.C";
in
  pkgs.stdenv.mkDerivation {
    name = "tomato";

    src = pkgs.fetchgit {
      url = repository-url;
      rev = "2bf6e116cbdaa611930340a7778b5bf85071ab62";
      sha256 = "H+zfRECLki3t2h2DqFMheoTd3bXhz4xY1XaeEZonwMY=";
    };

    installPhase = ''
      mkdir -p $out/bin && cp tomato tomatonoise $out/bin/

      ln -s $(which notify-send) $out/bin/
      ln -s $(which mpv) $out/bin/
    '';

    buildInputs = with pkgs; [
      gcc
      which
      gnumake
      ncurses
      pkgconfig
    ];

    propagatedBuildInputs = with pkgs; [
      libnotify
      mpv
    ];

    meta = with pkgs.lib; {
      description = "A pomodoro timer written in pure C.";
      homepage = repository-url;
      license = licenses.gpl3Plus;
      maintainers = with maintainers; [luisnquin];
    };
  }
