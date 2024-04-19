#         .             .              .
#         |             |              |           .
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'
#  ,|
#  `'
# default.nix
{
  pkg-config,
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

  src = builtins.path {
    name = "Tomato.C";
    path = ./.;
  };

  preConfigure = ''
    substituteInPlace notify.c \
      --replace /usr/local/ $out/

    substituteInPlace tomato.desktop \
      --replace /usr/local/ $out/

    substituteInPlace util.c \
      --replace /usr/local/ $out/

      substituteInPlace config.mk \
      --replace '/usr/local' $out
  '';

  nativeBuildInputs = [
    pkg-config
    gnumake
    ncurses
    which
    gcc
  ];

  propagatedBuildInputs = [
    libnotify
    mpv
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp tomato tomatonoise $out/bin/

    # Runtime dependencies
    ln -s ${libnotify}/bin/notify-send $out/bin/
    ln -s ${mpv}/bin/mpv $out/bin/

    mkdir -p $out/share/applications/
    mkdir -p $out/share/tomato/

    cp $src/tomato.desktop $out/share/applications/
    cp -r $src/sounds/ $out/share/tomato/
    cp -r $src/icons/ $out/share/tomato/
  '';

  meta = with lib; {
    description = "A pomodoro timer written in pure C.";
    homepage = "https://github.com/gabrielzschmitz/Tomato.C";
    license = licenses.gpl3Plus;
    maintainers = with maintainers; [luisnquin CarlosCraveiro];
  };
}
