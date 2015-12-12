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

#include <set>
#include <cassert>
#include <cmath>

#include "collision.hh"

TileCache::TileCache(SDL2pp::Renderer& renderer) : renderer_(renderer), cache_size_(64), finish_thread_(false) {
	loader_thread_ = std::thread([this](){
			std::unique_lock<std::mutex> lock(loader_queue_mutex_);
			while (true) {
				// wait on condvar until we should load something or must exit
				loader_queue_condvar_.wait(lock, [&](){ return !loader_queue_.empty() || finish_thread_; } );

				// finish the thread if requested
				if (finish_thread_)
					return;

				assert(!loader_queue_.empty());

				// take first tile from the queue and load its data
				// note that we load Surface and not a texture, so
				// we don't access OpenGL from non-main thread
				SDL2pp::Point current_tile = loader_queue_.front();
				currently_loading_ = current_tile;

				loader_queue_.pop_front();

				lock.unlock();

				Tile tile(current_tile);

				lock.lock();

				// save loaded tile into list so it's converted to
				// texture from main thread later
				loaded_tiles_.emplace(*currently_loading_, std::move(tile));
				currently_loading_ = SDL2pp::NullOpt;

				// wakeup main thread which may be waiting for us
				if (loader_queue_.empty())
					loader_queue_condvar_.notify_all();
			}
		});
}

TileCache::~TileCache() {
	// signal worked thread to exit and join
	{
		std::lock_guard<std::mutex> lock(loader_queue_mutex_);
		finish_thread_ = true;
	}
	loader_queue_condvar_.notify_all();
	loader_thread_.join();
}

void TileCache::SetCacheSize(size_t cache_size) {
	cache_size_ = cache_size;
}

void TileCache::PreloadTilesSync(const SDL2pp::Rect& rect) {
	ProcessTilesInRect(rect, [this](const SDL2pp::Point tilecoord) {
			if (tiles_.find(tilecoord) == tiles_.end())
				tiles_.emplace(tilecoord, Tile(tilecoord));
		});
}

void TileCache::UpdateCache(const SDL2pp::Rect& rect, int xprecache, int yprecache) {
	// we only have one upgrade candidate per frame, as
	// upgrading takes time and upgradeing multiple tiles
	// may cause lags
	SDL2pp::Optional<TileMap::iterator> upgrade_candidate;
	std::set<SDL2pp::Point> seen_tiles;

	{
		std::unique_lock<std::mutex> lock(loader_queue_mutex_);

		// clear queue, we'll for new one
		loader_queue_.clear();

		// flush downloader output
		for (auto& tile : loaded_tiles_) {
			tiles_.emplace(std::make_pair(tile.first, std::move(tile.second)));
			seen_tiles.insert(tile.first);
		}
		loaded_tiles_.clear();

		//
		// Synchronous phase. If we have anything to load here, it will
		// cause lags, but there's no other option to draw the consistent
		// image and to handle physics properly.
		//

		// first, if needed tile is currently loading, wait for this tile
		if (currently_loading_ && Tile::RectForCoords(*currently_loading_).Intersects(rect))
			loader_queue_condvar_.wait(lock, [&](){ return !currently_loading_; } );

		// next, forcibly load and upgrade all visible tiles
		ProcessTilesInRect(rect, [this, &seen_tiles](const SDL2pp::Point& tilecoord) {
				auto tile_iter = tiles_.find(tilecoord);
				if (tile_iter == tiles_.end())
					tile_iter = tiles_.emplace(tilecoord, Tile(tilecoord)).first;

				if (tile_iter->second.NeedsUpgrade())
					tile_iter->second.Upgrade(renderer_);
				seen_tiles.insert(tile_iter->first);
			});

		//
		// Async phase. Now we work with extended rectangle and form
		// a new queue for downloader and a candidate for upgrade
		//

		// make new queue
		ProcessTilesInRect(rect.GetExtension(xprecache, yprecache), [this, &upgrade_candidate](const SDL2pp::Point& tilecoord) {
				auto tile_iter = tiles_.find(tilecoord);
				if (tile_iter == tiles_.end()) {
					if (!currently_loading_ || *currently_loading_ != tilecoord)
						loader_queue_.emplace_back(tilecoord);
				} else {
					// XXX: use some more clever alg here? pick closest tile perhaps
					if (tile_iter->second.NeedsUpgrade())
						upgrade_candidate = tile_iter;
				}
			});
	}

	// upgrade single tile
	if (upgrade_candidate)
		(*upgrade_candidate)->second.Upgrade(renderer_);

	// ping loader to start crunching the new queue
	loader_queue_condvar_.notify_all();

	// sort lru info
	{
		// this would not needed if we've used some sort of associative container with
		// embedded linked list for LRU info; I'm too lazy to write one now
		std::list<SDL2pp::Point> new_lru;
		for (auto& tile : seen_tiles)
			new_lru.push_back(tile);
		for (auto& tile : lru_heavy_tiles_)
			if (seen_tiles.find(tile) == seen_tiles.end())
				new_lru.push_back(tile);

		lru_heavy_tiles_.swap(new_lru);
	}

	// finally, cleanup some old tiles
	while (tiles_.size() > cache_size_) {
		tiles_.erase(lru_heavy_tiles_.back());
		lru_heavy_tiles_.pop_back();
	}
}

void TileCache::Render(const SDL2pp::Rect& rect) {
	SDL2pp::Point start_tile = Tile::CoordsForPoint(SDL2pp::Point(rect.x, rect.y));
	SDL2pp::Point end_tile = Tile::CoordsForPoint(SDL2pp::Point(rect.GetX2(), rect.GetY2()));

	// render all seen tiles
	SDL2pp::Point tilecoord;
	for (tilecoord.x = start_tile.x; tilecoord.x <= end_tile.x; tilecoord.x++) {
		for (tilecoord.y = start_tile.y; tilecoord.y <= end_tile.y; tilecoord.y++) {
			auto tileiter = tiles_.find(tilecoord);
			if (tileiter != tiles_.end())
				tileiter->second.Render(renderer_, rect);
		}
	}
}

void TileCache::UpdateCollisions(CollisionInfo& collisions, const SDL2pp::Rect& rect, int distance) {
	ProcessTilesInRect(rect.GetExtension(distance), [&](const SDL2pp::Point& tilecoord) {
			auto tile = tiles_.find(tilecoord);
			if (tile == tiles_.end()) // while we can skip not loaded tiles for rendering, we can't for physics
				tile = tiles_.emplace(tilecoord, Tile(tilecoord)).first; // so load needed tile synchronously

			tile->second.CheckLeftCollision(collisions, SDL2pp::Rect(rect.x - distance, rect.y, distance, rect.h));
			tile->second.CheckRightCollision(collisions, SDL2pp::Rect(rect.x + rect.w, rect.y, distance, rect.h));
			tile->second.CheckTopCollision(collisions, SDL2pp::Rect(rect.x, rect.y - distance, rect.w, distance));
			tile->second.CheckBottomCollision(collisions, SDL2pp::Rect(rect.x, rect.y + rect.h, rect.w, distance));
		});
}
