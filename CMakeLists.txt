cmake_minimum_required(VERSION 3.16)

# meta
project(hoverboard-sdl LANGUAGES CXX VERSION 0.8.0)

# options
option(SYSTEMWIDE "Build for systemwide installation" OFF)
option(STANDALONE "Build for creating standalone package" OFF)

if(SYSTEMWIDE)
	set(BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE STRING "Where to install binaries")
	set(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Root directory for data files")
	set(DATADIR "${DATAROOTDIR}/hoverboard" CACHE STRING "Where to install data files")
	set(DESKTOPDIR "${DATAROOTDIR}/applications" CACHE STRING "Where to install .desktop file")
	set(DOCSDIR "${DATAROOTDIR}/doc/hoverboard" CACHE STRING "Where to install documentation")
elseif(STANDALONE)
	set(BINDIR "." CACHE STRING "Where to install binaries")
	set(DATAROOTDIR "." CACHE STRING "Root directory for data files")
	set(DATADIR "data" CACHE STRING "Where to install data files")
	set(DESKTOPDIR "." CACHE STRING "Where to install .desktop file")
	set(DOCSDIR "." CACHE STRING "Where to install documentation")
endif()

# flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# depends
if(EXISTS ${PROJECT_SOURCE_DIR}/extlibs/libSDL2pp/CMakeLists.txt)
	set(SDL2PP_WITH_IMAGE TRUE)
	set(SDL2PP_WITH_TTF TRUE)
	add_subdirectory(extlibs/libSDL2pp)
else()
	find_package(SDL2pp)
endif()

find_package(Threads)

# definitions
if(SYSTEMWIDE OR STANDALONE)
	add_definitions(-DHOVERBOARD_DATADIR="${DATADIR}")
else()
	add_definitions(-DHOVERBOARD_DATADIR="${PROJECT_SOURCE_DIR}/data")
endif()

if(STANDALONE)
	add_definitions(-DSTANDALONE)
endif()

# sources
set(SOURCES
	src/coins.cc
	src/game.cc
	src/main.cc
	src/tilecache.cc
	src/tile.cc
	src/tile_obstacle.cc
	src/tile_visual.cc
)

set(HEADERS
	src/collision.hh
	src/game.hh
	src/tilecache.hh
	src/tile.hh
)

set(RCFILES
	misc/hoverboard.rc
)

# binary
add_executable(hoverboard ${SOURCES} ${HEADERS} ${RCFILES})
target_link_libraries(hoverboard SDL2pp::SDL2pp Threads::Threads)
set_target_properties(hoverboard PROPERTIES WIN32_EXECUTABLE ON)

# installation
if(SYSTEMWIDE OR STANDALONE)
	install(TARGETS hoverboard RUNTIME DESTINATION ${BINDIR})
	install(DIRECTORY data/ DESTINATION ${DATADIR})

	install(FILES README.md COPYING COPYING.DATA DESTINATION ${DOCSDIR})

	if(NOT WIN32)
		configure_file(misc/hoverboard.desktop.in misc/hoverboard.desktop)
		install(FILES ${PROJECT_BINARY_DIR}/misc/hoverboard.desktop DESTINATION ${DESKTOPDIR})
	endif()
endif()
