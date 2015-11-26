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

#include "map.hh"

#include <sstream>

static bool operator<(SDL2pp::Point a, SDL2pp::Point b) {
	if (a.x < b.x)
		return true;
	if (a.x == b.x)
		return a.y < b.y;
	return false;
}

std::string Map::MakeTilePath(SDL2pp::Point tile) {
	std::stringstream filename;
	filename << DATADIR << "/" << tile.x << "/" << tile.y << ".png";
	return filename.str();
}

Map::Map(SDL2pp::Renderer& renderer) : renderer_(renderer) {
}

Map::~Map() {
}

void Map::Render(SDL2pp::Rect rect) {
	static const int tilesize = 512;

	SDL2pp::Point start_tile(floordiv(rect.x, tilesize), floordiv(rect.y, tilesize));
	SDL2pp::Point end_tile(floordiv(rect.GetX2(), tilesize), floordiv(rect.GetY2(), tilesize));

	SDL2pp::Point offset = start_tile * tilesize - SDL2pp::Point(rect.x, rect.y);

	SDL2pp::Point tile;
	for (tile.x = start_tile.x; tile.x <= end_tile.x; tile.x++) {
		for (tile.y = start_tile.y; tile.y <= end_tile.y; tile.y++) {
			if (absent_tiles_.find(tile) != absent_tiles_.end())
				continue;
			auto tileiter = tiles_.find(tile);
			if (tileiter == tiles_.end()) {
				try {
					tileiter = tiles_.insert(std::make_pair(tile, SDL2pp::Texture(renderer_, MakeTilePath(tile)))).first;
				} catch (...) {
					absent_tiles_.insert(tile);
					continue;
				}
			}

			renderer_.Copy(tileiter->second, SDL2pp::Rect(0, 0, tilesize, tilesize), (tile - start_tile) * tilesize + offset);
		}
	}
}
