# FIXME_NAME

[![Build Status](https://travis-ci.org/FIXME_USER/FIXME_PROJECT.svg?branch=master)](https://travis-ci.org/FIXME_USER/FIXME_PROJECT)

FIXME_DESCRIPTION

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

* [FIXME_NAME](https://github.com/FIXME_USER) <FIXME_EMAIL>

## License

GPLv3 or later, see COPYING

The project also bundles third party software under its own licenses:

* extlibs/libSDL2pp (C++11 SDL2 wrapper library) - zlib license
