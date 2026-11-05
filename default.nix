#/**
# * @file default.nix
# * @brief Nix package for Tomato.C.
# *
# * Builds the binary with the same Makefile as ./build.sh, embedding the
# * absolute Nix store data path via DATAPREFIX, and installs the binary,
# * data files (sprites, sounds, icons), desktop entry and sample config.
# *
# * Consumable both directly (`nix-build`, `callPackage ./default.nix`)
# * and through the flake (`nix build .#`).
# */

{ lib, stdenv, pkg-config, gnumake, ncurses, libnotify }:

stdenv.mkDerivation {
  pname = "tomato-c";
  version = "1.1.3"; # Bump on each release (matching the git tag).

  src = lib.cleanSource ./.;

  nativeBuildInputs = [ pkg-config gnumake ];
  buildInputs = [ ncurses libnotify ];

  buildPhase = ''
    runHook preBuild
    # PREFIX overrides the /usr/local include/lib leakage from config.mk;
    # DATAPREFIX embeds the installed data directory into the binary.
    make -C build PREFIX="$out" DATAPREFIX="$out/share/tomato"
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dm755 build/tomato "$out/bin/tomato"

    mkdir -p "$out/share/tomato/sprites" \
             "$out/share/tomato/sounds" \
             "$out/share/tomato/icons" \
             "$out/share/applications"

    install -m644 resources/sprites/*.asc        "$out/share/tomato/sprites/"
    install -m644 resources/sounds/*.mp3         "$out/share/tomato/sounds/"
    install -m644 resources/icons/tomato.png     "$out/share/tomato/icons/tomato.png"
    install -m644 resources/icons/tomato.svg     "$out/share/tomato/icons/tomato.svg"
    install -m644 sample_config.toml             "$out/share/tomato/sample_config.toml"

    install -m644 tomato.desktop "$out/share/applications/tomato.desktop"
    substituteInPlace "$out/share/applications/tomato.desktop" \
      --replace "Icon=tomato" "Icon=$out/share/tomato/icons/tomato.svg"

    runHook postInstall
  '';

  meta = with lib; {
    description = "A modular, extensible terminal-based Pomodoro timer in pure C";
    homepage = "https://github.com/gabrielzschmitz/Tomato.C";
    license = licenses.gpl3Only;
    mainProgram = "tomato";
    platforms = platforms.linux;
  };
}
