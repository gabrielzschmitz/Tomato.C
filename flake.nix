{
  description = "A pomodoro timer written in pure C";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, flake-utils, ...}:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        tomato-c = (with pkgs; stdenv.mkDerivation {
          pname = "tomato-c";
          version = "1.0";
          src = fetchFromGitHub {
            owner = "gabrielzschmitz";
            repo = "Tomato.C";
            rev = "61aa8a3263b602176fef374fc331429dce405d52";
            sha256 = "0752qgydqmgn8if77r6rx1v72l4zn76yv9f127v3b8nl24165ggv";
          };

          nativeBuildInputs = with pkgs; [
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

          installPhase = ''
            mkdir -p $out/bin && cp tomato tomatonoise $out/bin/

            ln -s $(which notify-send) $out/bin/
            ln -s $(which mpv) $out/bin/
          '';
          
        });
      in rec {
        defaultPackage = tomato-c;
        defaultApp = flake-utils.lib.mkApp {
          drv = defaultPackage;
        };
        devShell = pkgs.mkShell {
          buildInputs = [ tomato-c ];
        };
      }
    );
}
