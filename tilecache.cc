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
#include <set>

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

TileCache::TileCache(SDL2pp::Renderer& renderer) : renderer_(renderer), cache_size_(128) {
}

TileCache::~TileCache() {
}

void TileCache::SetCacheSize(size_t cache_size) {
	cache_size_ = cache_size;
}

void TileCache::Render(const SDL2pp::Rect& rect) {
	SDL2pp::Point start_tile = Tile::TileForPoint(SDL2pp::Point(rect.x, rect.y));
	SDL2pp::Point end_tile = Tile::TileForPoint(SDL2pp::Point(rect.GetX2(), rect.GetY2()));

	std::set<SDL2pp::Point> seen_tiles;

	// render all seen tiles
	SDL2pp::Point tilecoord;
	for (tilecoord.x = start_tile.x; tilecoord.x <= end_tile.x; tilecoord.x++) {
		for (tilecoord.y = start_tile.y; tilecoord.y <= end_tile.y; tilecoord.y++) {
			auto tileiter = tiles_.find(tilecoord);
			if (tileiter == tiles_.end())
				tileiter = tiles_.insert(std::make_pair(tilecoord, CreateTile(tilecoord, LoadTileData(tilecoord)))).first;

			seen_tiles.insert(tilecoord);
			tileiter->second->Render(renderer_, rect);
		}
	}

	// cleanup extra tiles
	if (tiles_.size() > cache_size_) {
		// sort all known tiles by distance from viewer
		std::map<int, SDL2pp::Point> tiles_for_removal;
		for (auto& tile : tiles_) {
			if (seen_tiles.find(tilecoord) != seen_tiles.end())
				continue;

			SDL2pp::Point delta = Tile::TileForPoint(SDL2pp::Point(rect.x + rect.w/2, rect.y + rect.h/2)) - tile.first;
			int distance = std::abs(delta.x) + std::abs(delta.y);
			tiles_for_removal.insert(std::make_pair(distance, tile.first));
		}

		// remove furthest
		for (auto iter = tiles_for_removal.rbegin(); iter != tiles_for_removal.rend() && tiles_.size() > cache_size_; iter++)
			tiles_.erase(iter->second);
	}
}
