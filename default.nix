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
      rev = "77aed3d216331f4e332a67417eeddb029644416d";
      sha256 = "HoUe5ooQD4mTLUxFEA1JnXKNDDCGJVh1wrRuy/e184c=";
    };

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
