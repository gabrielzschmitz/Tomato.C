#         .             .              .
#         |             |              |           .
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
#  ,|
#  `'
# default.nix
{pkgs ? import <nixpkgs> {}}:
pkgs.stdenv.mkDerivation {
  name = "tomato";

  src = pkgs.fetchFromGitHub {
    owner = "gabrielzschmitz";
    repo = "Tomato.C";
    rev = "61aa8a3263b602176fef374fc331429dce405d52";
    sha256 = "0752qgydqmgn8if77r6rx1v72l4zn76yv9f127v3b8nl24165ggv";
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
    homepage = "https://github.com/gabrielzschmitz/Tomato.C";
    license = licenses.gpl3Plus;
    maintainers = with maintainers; [luisnquin];
  };
}
