# hoverboard-sdl

[![Build Status](https://travis-ci.org/AMDmi3/hoverboard-sdl.svg?branch=master)](https://travis-ci.org/AMDmi3/hoverboard-sdl)

Desktop version of [xkcd 1608 "Hoverboard"](https://xkcd.com/1608/) game.

## Building

Dependencies:

* [CMake](http://www.cmake.org/)
* [SDL2](http://libsdl.org/)
* [SDL2_image](https://www.libsdl.org/projects/SDL_image/)

The project also uses libSDL2pp, C++11 bindings library for SDL2.
It's included into git repository as a submodule, so if you've
obtained source through git, don't forget to run ```git submodule
init && git submodule update```.

To build the project, run:

```
cmake . && make
```

## Author

* [AMDmi3](https://github.com/AMDmi3) <amdmi3@amdmi3.ru>

## License

* Code: GPLv3 or later, see COPYING
* Data: Creative Commons Attribution-NonCommercial 2.5 License, see [kxcd.com](https://xkcd.com/license.html).

The project also bundles third party software under its own licenses:

* extlibs/libSDL2pp (C++11 SDL2 wrapper library) - zlib license
