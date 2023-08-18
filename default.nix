#         .             .              .
#         |             |              |           .
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
#  ,|
#  `'
# default.nix
{
  pkgconfig,
  libnotify,
  gnumake,
  ncurses,
  stdenv,
  which,
  mpv,
  gcc,
  lib,
}:
stdenv.mkDerivation {
  name = "tomato";

  src = ./.;

  installPhase = ''
    mkdir -p $out/bin && cp tomato tomatonoise $out/bin/

    ln -s $(which notify-send) $out/bin/
    ln -s $(which mpv) $out/bin/
  '';

  buildInputs = [
    gcc
    which
    gnumake
    ncurses
    pkgconfig
  ];

  propagatedBuildInputs = [
    libnotify
    mpv
  ];

  meta = with lib; {
    description = "A pomodoro timer written in pure C.";
    homepage = "https://github.com/gabrielzschmitz/Tomato.C";
    license = licenses.gpl3Plus;
    maintainers = with maintainers; [luisnquin];
  };
}
