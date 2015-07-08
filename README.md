# libSDL2pp project template

This is a template for libSDL2pp-using projects. It contains:

* libSDL2pp submodule
* GPLv3 license
* Readme file
* Simple libSDL2pp program
* CMakeLists.txt to build the project
* Travis CI config file

To use it, just clone this repository, squash all commits into
single one it if you want and replace a number of ```fixme```
placeholders with names approproate for your prokect:

* CMakeLists.txt:
  * Project name
  * Variable names for headers and sources
  * Main target name
* main.cc:
  * Project name in license header (4 times)
  * Main window title
* README.md:
  * Header
  * GitHub user and project names in travis icon
  * Project description
  * URL for your GitHub profile

To make sure you've changed everything:

```
grep -Ri fixme *
```

*Finally, remove everything up to the following line and have fun developing your libSDL2pp-using project!*

---

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
