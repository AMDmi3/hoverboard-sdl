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

#include "tilecache.hh"

#include <sys/types.h>
#include <sys/stat.h>

#include <sstream>

#include <SDL2pp/Surface.hh>

#include "tiles.hh"

std::string TileCache::MakeTilePath(const SDL2pp::Point& coords) {
	std::stringstream filename;
	filename << DATADIR << "/" << coords.x << "/" << coords.y << ".png";
	return filename.str();
}

TileCache::SurfacePtr TileCache::LoadTileData(const SDL2pp::Point& coords) {
	std::string path = MakeTilePath(coords);

	struct stat st;
	if (stat(path.c_str(), &st) == 0)
		return SurfacePtr(new SDL2pp::Surface(path));

	return SurfacePtr();
}

TileCache::TilePtr TileCache::CreateTile(const SDL2pp::Point& coords, SurfacePtr surface) {
	if (surface)
		return TilePtr(new TextureTile(coords, SDL2pp::Texture(renderer_, *surface)));
	else
		return TilePtr(new EmptyTile(coords));
}

TileCache::TileCache(SDL2pp::Renderer& renderer) : renderer_(renderer) {
}

TileCache::~TileCache() {
}

void TileCache::Render(const SDL2pp::Rect& rect) {
	SDL2pp::Point start_tile = Tile::TileForPoint(SDL2pp::Point(rect.x, rect.y));
	SDL2pp::Point end_tile = Tile::TileForPoint(SDL2pp::Point(rect.GetX2(), rect.GetY2()));

	SDL2pp::Point tile;
	for (tile.x = start_tile.x; tile.x <= end_tile.x; tile.x++) {
		for (tile.y = start_tile.y; tile.y <= end_tile.y; tile.y++) {
			auto tileiter = tiles_.find(tile);
			if (tileiter == tiles_.end())
				tileiter = tiles_.insert(std::make_pair(tile, CreateTile(tile, LoadTileData(tile)))).first;

			tileiter->second->Render(renderer_, rect);
		}
	}
}
