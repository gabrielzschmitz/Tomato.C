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
      rev = "159aee0b483e0c0b3c8ec3ce3b2df380ea662757";
      sha256 = "GestQkQ+j1gm0cfzrm1JoR9zXFR9lm+hpBYAYteDKzI=";
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
