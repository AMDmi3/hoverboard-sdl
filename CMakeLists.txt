PROJECT(hoverboard-sdl)

# meta
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	MESSAGE("")
	MESSAGE("*** Building for inplace run")
	MESSAGE("*** Specify CMAKE_INSTALL_PREFIX if you want to install")
	MESSAGE("")

	SET(SYSTEMWIDE 0)
ELSE(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	MESSAGE("")
	MESSAGE("*** Building for installation into ${CMAKE_INSTALL_PREFIX}")
	MESSAGE("")

	SET(SYSTEMWIDE 1)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# options
IF(SYSTEMWIDE)
	SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE STRING "Where to install binaries")
	SET(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Root directory for data files")
	SET(DATADIR "${DATAROOTDIR}/hoverboard" CACHE STRING "Where to install data files")
	SET(ICONDIR "${DATAROOTDIR}/pixmaps" CACHE STRING "Where to install icon")
	SET(DESKTOPDIR "${DATAROOTDIR}/applications" CACHE STRING "Where to install .desktop file")
ENDIF(SYSTEMWIDE)

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
IF(SYSTEMWIDE)
	ADD_DEFINITIONS(-DDATADIR="${DATADIR}")
ELSE(SYSTEMWIDE)
	ADD_DEFINITIONS(-DDATADIR="${PROJECT_SOURCE_DIR}/data")
ENDIF(SYSTEMWIDE)

# sources
SET(SOURCES
	src/coins.cc
	src/game.cc
	src/main.cc
	src/tilecache.cc
	src/tiles.cc
)

SET(HEADERS
	src/collision.hh
	src/game.hh
	src/passability.hh
	src/tilecache.hh
	src/tiles.hh
)

# binary
INCLUDE_DIRECTORIES(SYSTEM ${SDL2PP_INCLUDE_DIRS})
ADD_EXECUTABLE(hoverboard ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(hoverboard ${SDL2PP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# installation
IF(SYSTEMWIDE)
	CONFIGURE_FILE(misc/hoverboard.desktop.in misc/hoverboard.desktop)

	INSTALL(TARGETS hoverboard RUNTIME DESTINATION ${BINDIR})
	INSTALL(DIRECTORY data/ DESTINATION ${DATADIR})
	INSTALL(FILES misc/xkcd.ico DESTINATION ${ICONDIR} RENAME hoverboard.ico)
	INSTALL(FILES ${PROJECT_BINARY_DIR}/misc/hoverboard.desktop DESTINATION ${DESKTOPDIR})
ENDIF(SYSTEMWIDE)
