PROJECT(hoverboard-sdl)

# meta
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# options
OPTION(SYSTEMWIDE "Build for systemwide installation" OFF)
OPTION(STANDALONE "Build for creating standalone package" OFF)

IF(SYSTEMWIDE)
	SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE STRING "Where to install binaries")
	SET(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Root directory for data files")
	SET(DATADIR "${DATAROOTDIR}/hoverboard" CACHE STRING "Where to install data files")
	SET(DESKTOPDIR "${DATAROOTDIR}/applications" CACHE STRING "Where to install .desktop file")
ELSEIF(STANDALONE)
	SET(BINDIR "." CACHE STRING "Where to install binaries")
	SET(DATAROOTDIR "." CACHE STRING "Root directory for data files")
	SET(DATADIR "data" CACHE STRING "Where to install data files")
	SET(DESKTOPDIR "." CACHE STRING "Where to install .desktop file")
ENDIF(SYSTEMWIDE)

# flags
IF(NOT MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic")
ENDIF(NOT MSVC)

# depends
IF(NOT EXISTS ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp/CMakeLists.txt)
	MESSAGE(FATAL_ERROR "The source directory\n  ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp\ndoes not contain a CMakeLists.txt file.\nIt is likely that you forgot to run\n  git submodule init && git submodule update")
ENDIF(NOT EXISTS ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp/CMakeLists.txt)

SET(SDL2PP_WITH_IMAGE TRUE)
SET(SDL2PP_WITH_TTF TRUE)
ADD_SUBDIRECTORY(extlibs/libSDL2pp)

FIND_PACKAGE(Threads)

# definitions
IF(SYSTEMWIDE OR STANDALONE)
	ADD_DEFINITIONS(-DHOVERBOARD_DATADIR="${DATADIR}")
ELSE(SYSTEMWIDE OR STANDALONE)
	ADD_DEFINITIONS(-DHOVERBOARD_DATADIR="${PROJECT_SOURCE_DIR}/data")
ENDIF(SYSTEMWIDE OR STANDALONE)

IF(STANDALONE)
	ADD_DEFINITIONS(-DSTANDALONE)
ENDIF(STANDALONE)

# sources
SET(SOURCES
	src/coins.cc
	src/game.cc
	src/main.cc
	src/tilecache.cc
	src/tile.cc
	src/tile_obstacle.cc
	src/tile_visual.cc
)

SET(HEADERS
	src/collision.hh
	src/game.hh
	src/tilecache.hh
	src/tile.hh
)

SET(RCFILES
	misc/hoverboard.rc
)

# binary
INCLUDE_DIRECTORIES(SYSTEM ${SDL2PP_INCLUDE_DIRS})
ADD_EXECUTABLE(hoverboard ${SOURCES} ${HEADERS} ${RCFILES})
TARGET_LINK_LIBRARIES(hoverboard ${SDL2PP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# installation
IF(SYSTEMWIDE OR STANDALONE)
	INSTALL(TARGETS hoverboard RUNTIME DESTINATION ${BINDIR})
	INSTALL(DIRECTORY data/ DESTINATION ${DATADIR})

	IF(NOT WIN32)
		CONFIGURE_FILE(misc/hoverboard.desktop.in misc/hoverboard.desktop)
		INSTALL(FILES ${PROJECT_BINARY_DIR}/misc/hoverboard.desktop DESTINATION ${DESKTOPDIR})
	ENDIF(NOT WIN32)
ENDIF(SYSTEMWIDE OR STANDALONE)
