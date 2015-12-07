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
				loaded_tiles_.emplace_back(std::make_pair(current_tile, std::move(tile)));
				currently_loading_ = SDL2pp::NullOpt;
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

void TileCache::UpdateCache(const SDL2pp::Rect& rect) {
	std::set<SDL2pp::Point> seen_tiles;

	{
		std::lock_guard<std::mutex> lock(loader_queue_mutex_);

		// First, materialize all freshly loaded tiles
		for (auto& loaded : loaded_tiles_)
			tiles_.emplace(std::make_pair(loaded.first, std::move(loaded.second)));

		loaded_tiles_.clear();

		// Next add all missing tiles to the queue
		std::list<SDL2pp::Point> missing_tiles;

		ProcessTilesInRect(rect, [this, &missing_tiles, &seen_tiles](const SDL2pp::Point& tilecoord) {
				if (tiles_.find(tilecoord) == tiles_.end() && (!currently_loading_ || *currently_loading_ != tilecoord))
					missing_tiles.push_back(tilecoord);
				else
					seen_tiles.emplace(tilecoord);
			});

		// Update loader queue
		loader_queue_.swap(missing_tiles);
	}

	// Slap loader thread to start loading new tile if it was sleeping
	loader_queue_condvar_.notify_all();

	// Cleanup extra tiles
	if (tiles_.size() > cache_size_) {
		// Sort all known tiles by distance from viewer
		std::map<int, SDL2pp::Point> tiles_for_removal;
		for (auto& tile : tiles_) {
			if (seen_tiles.find(tile.first) != seen_tiles.end())
				continue;

			SDL2pp::Point delta = Tile::TileForPoint(SDL2pp::Point(rect.x + rect.w/2, rect.y + rect.h/2)) - tile.first;
			int distance = std::abs(delta.x) + std::abs(delta.y);
			tiles_for_removal.emplace(std::make_pair(distance, tile.first));
		}

		// Remove furthest
		for (auto iter = tiles_for_removal.rbegin(); iter != tiles_for_removal.rend() && tiles_.size() > cache_size_; iter++)
			tiles_.erase(iter->second);
	}
}

void TileCache::Render(const SDL2pp::Rect& rect) {
	SDL2pp::Point start_tile = Tile::TileForPoint(SDL2pp::Point(rect.x, rect.y));
	SDL2pp::Point end_tile = Tile::TileForPoint(SDL2pp::Point(rect.GetX2(), rect.GetY2()));

	// render all seen tiles
	SDL2pp::Point tilecoord;
	for (tilecoord.x = start_tile.x; tilecoord.x <= end_tile.x; tilecoord.x++) {
		for (tilecoord.y = start_tile.y; tilecoord.y <= end_tile.y; tilecoord.y++) {
			auto tileiter = tiles_.find(tilecoord);
			if (tileiter != tiles_.end()) {
				tileiter->second.Materialize(renderer_); // XXX
				tileiter->second.Render(renderer_, rect);
			}
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
