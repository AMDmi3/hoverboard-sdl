/*
 * Copyright (C) 2015 Dmitry Marakasov <amdmi3@amdmi3.ru>
 *
 * This file is part of hoverboard-sdl.
 *
 * hoverboard-sdl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hoverboard-sdl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hoverboard-sdl.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILECACHE_HH
#define TILECACHE_HH

#include <map>
#include <memory>
#include <string>

#include <SDL2pp/Surface.hh>
#include <SDL2pp/Renderer.hh>

class Tile;

class TileCache {
private:
	SDL2pp::Renderer& renderer_;

	typedef std::unique_ptr<Tile> TilePtr;
	typedef std::unique_ptr<SDL2pp::Surface> SurfacePtr;

	std::map<SDL2pp::Point, TilePtr> tiles_;

private:
	static std::string MakeTilePath(const SDL2pp::Point& coords);
	static SurfacePtr LoadTileData(const SDL2pp::Point& coords);
	TilePtr CreateTile(const SDL2pp::Point& coords, SurfacePtr surface);

public:
	TileCache(SDL2pp::Renderer& renderer);
	~TileCache();

	void Render(const SDL2pp::Rect& rect);
};

#endif // TILECACHE_HH
