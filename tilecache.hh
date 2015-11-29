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
#include <condition_variable>
#include <mutex>
#include <list>
#include <thread>

#include <SDL2pp/Surface.hh>
#include <SDL2pp/Renderer.hh>

#include "tiles.hh"

class Tile;

class TileCache {
private:
	typedef std::unique_ptr<Tile> TilePtr;
	typedef std::unique_ptr<SDL2pp::Surface> SurfacePtr;
	typedef std::map<SDL2pp::Point, TilePtr> TileMap;

private:
	SDL2pp::Renderer& renderer_;

	TileMap tiles_;
	size_t cache_size_;

	// background loader
	std::thread loader_thread_;
	std::list<SDL2pp::Point> loader_queue_;
	std::list<std::pair<SDL2pp::Point, SurfacePtr>> loaded_list_;
	SDL2pp::Optional<SDL2pp::Point> currently_loading_;

	std::mutex loader_queue_mutex_;
	std::condition_variable loader_queue_condvar_;

	bool finish_thread_;

private:
	static std::string MakeTilePath(const SDL2pp::Point& coords);
	static SurfacePtr LoadTileData(const SDL2pp::Point& coords);
	TileMap::iterator CreateTile(const SDL2pp::Point& coords, SurfacePtr surface);

	template<class T>
	void ProcessTilesInRect(const SDL2pp::Rect& rect, T processor) {
		SDL2pp::Point start_tile = Tile::TileForPoint(SDL2pp::Point(rect.x, rect.y));
		SDL2pp::Point end_tile = Tile::TileForPoint(SDL2pp::Point(rect.GetX2(), rect.GetY2()));

		SDL2pp::Point tilecoord;
		for (tilecoord.x = start_tile.x; tilecoord.x <= end_tile.x; tilecoord.x++)
			for (tilecoord.y = start_tile.y; tilecoord.y <= end_tile.y; tilecoord.y++)
				processor(tilecoord);
	}

public:
	TileCache(SDL2pp::Renderer& renderer);
	~TileCache();

	void SetCacheSize(size_t cache_size);

	void PreloadTilesSync(const SDL2pp::Rect& rect);
	void UpdateCache(const SDL2pp::Rect& rect);
	void Render(const SDL2pp::Rect& rect);
};

#endif // TILECACHE_HH
