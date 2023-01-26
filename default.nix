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
      rev = "dda8e0ffe0b8612691ed336962f33580fb3a6038";
      sha256 = "193xr63s4700bsamac0zjff0l6n8bcm60f546jfxqaafz126f9zv";
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
