# Author:  luftmensch-luftmensch
{
  description = "A pomodoro timer written in pure C";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs {
          inherit system;
        };

        tomato-c = pkgs.callPackage ./default.nix {};
      in rec {
        defaultPackage = tomato-c;
        defaultApp = flake-utils.lib.mkApp {
          drv = defaultPackage;
        };
        devShell = pkgs.mkShell {
          buildInputs = [tomato-c];
        };
      }
    );
}

