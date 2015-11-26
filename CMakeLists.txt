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
ADD_SUBDIRECTORY(extlibs/libSDL2pp)

# datadir
ADD_DEFINITIONS(-DDATADIR="${PROJECT_SOURCE_DIR}/data")

# sources
SET(SOURCES
	main.cc
	game.cc
	tiles.cc
	tilecache.cc
)

SET(HEADERS
	game.hh
	tiles.hh
	tilecache.hh
)

# binary
INCLUDE_DIRECTORIES(SYSTEM ${SDL2PP_INCLUDE_DIRS})
ADD_EXECUTABLE(hoverboard ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(hoverboard ${SDL2PP_LIBRARIES})
