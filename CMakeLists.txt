PROJECT(hoverboard-sdl)

# meta
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")

# depends
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp/CMakeLists.txt)
	MESSAGE(FATAL_ERROR "The source directory\n  ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp\ndoes not contain a CMakeLists.txt file.\nIt is likely that you forgot to run\n  git submodule init && git submodule update")
ENDIF(NOT EXISTS ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp/CMakeLists.txt)

SET(SDL2PP_WITH_IMAGE TRUE)
SET(SDL2PP_WITH_TTF TRUE)
ADD_SUBDIRECTORY(extlibs/libSDL2pp)

FIND_PACKAGE(Threads)

# datadir
ADD_DEFINITIONS(-DDATADIR="${PROJECT_SOURCE_DIR}/data")

# sources
SET(SOURCES
	coins.cc
	game.cc
	main.cc
	tilecache.cc
	tiles.cc
)

SET(HEADERS
	collision.hh
	game.hh
	passability.hh
	tilecache.hh
	tiles.hh
)

# binary
INCLUDE_DIRECTORIES(SYSTEM ${SDL2PP_INCLUDE_DIRS})
ADD_EXECUTABLE(hoverboard ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(hoverboard ${SDL2PP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
