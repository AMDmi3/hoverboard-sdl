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

#include "tile.hh"

#include "collision.hh"

Tile::ObstacleData::~ObstacleData() {
}

Tile::NoObstacle::~NoObstacle() {
}

void Tile::NoObstacle::CheckLeftCollision(CollisionInfo&, const SDL2pp::Rect&, const SDL2pp::Point&) const {
}

void Tile::NoObstacle::CheckRightCollision(CollisionInfo&, const SDL2pp::Rect&, const SDL2pp::Point&) const {
}

void Tile::NoObstacle::CheckTopCollision(CollisionInfo&, const SDL2pp::Rect&, const SDL2pp::Point&) const {
}

void Tile::NoObstacle::CheckBottomCollision(CollisionInfo&, const SDL2pp::Rect&, const SDL2pp::Point&) const {
}

Tile::SolidObstacle::SolidObstacle() {
}

Tile::SolidObstacle::~SolidObstacle() {
}

void Tile::SolidObstacle::CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	coll.AddLeftCollision(localrect.GetBottomRight() + offset);
}

void Tile::SolidObstacle::CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	coll.AddRightCollision(localrect.GetBottomLeft() + offset);
}

void Tile::SolidObstacle::CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	coll.AddTopCollision(localrect.GetY2() + offset.y);
}

void Tile::SolidObstacle::CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	coll.AddBottomCollision(localrect.y + offset.y);
}

Tile::ObstacleMap::ObstacleMap(Map&& map) : map_(std::move(map)) {
}

Tile::ObstacleMap::~ObstacleMap() {
}

void Tile::ObstacleMap::CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	// This bit is interesting from the performance perspective
	// here we need to get rightmost (and if there are multiple rightmost,
	// bottommost of them) point for which obstacle mask is true
	//
	// We can either:
	// - scan whole localrectangle from top to bottom and left to right and
	//   return the best matching point
	// - scan from the right, the from the bottom, and return the first
	//   matching point
	//
	// The second way is, in theory, really unfriendly to cache prefetching
	// (however modern CPUs may have optimization for backward scan as well),
	// but it lets us stop the scan early in some cases (player close to a
	// wall). The first way is cache-friendly, though it always scans the
	// while localrectangle.
	//
	// There are other considerations, such as that we usually only need to
	// scan a thin (in horizontal dilocalrection) stripe, so prefetching doesn't
	// help us in any case.
	//
	// Anyway, I haven't done any tests and just chose a second variant
	// intuitively, but someone could do some research. Also, there could
	// be more clever data structure than a plain mask used to speed up
	// collision detection even more. It's not a bottleneck for this to
	// make sense though...
	SDL2pp::Point point;
	for (point.x = localrect.GetX2(); point.x >= localrect.x; point.x--) {
		for (point.y = localrect.GetY2(); point.y >= localrect.y; point.y--) {
			if (map_[point.x + tile_size_ * point.y]) {
				coll.AddLeftCollision(point + offset);
				return;
			}
		}
	}
}

void Tile::ObstacleMap::CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	SDL2pp::Point point;
	for (point.x = localrect.x; point.x <= localrect.GetX2(); point.x++) {
		for (point.y = localrect.GetY2(); point.y >= localrect.y; point.y--) {
			if (map_[point.x + tile_size_ * point.y]) {
				coll.AddRightCollision(point + offset);
				return;
			}
		}
	}
}

void Tile::ObstacleMap::CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	SDL2pp::Point point;
	for (point.y = localrect.GetY2(); point.y >= localrect.y; point.y--) {
		for (point.x = localrect.x; point.x <= localrect.GetX2(); point.x++) {
			if (map_[point.x + tile_size_ * point.y]) {
				coll.AddTopCollision(point.y + offset.y);
				return;
			}
		}
	}
}

void Tile::ObstacleMap::CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const {
	SDL2pp::Point point;
	for (point.y = localrect.y; point.y <= localrect.GetY2(); point.y++) {
		for (point.x = localrect.x; point.x <= localrect.GetX2(); point.x++) {
			if (map_[point.x + tile_size_ * point.y]) {
				coll.AddBottomCollision(point.y + offset.y);
				return;
			}
		}
	}
}
