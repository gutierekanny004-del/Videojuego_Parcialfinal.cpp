{ pkgs }: {
  deps = [
    pkgs.cmakeWithGui
    pkgs.cmake
    pkgs.gnumake
    pkgs.gcc
    pkgs.ncurses
    pkgs.ncurses.dev
  ];
}
