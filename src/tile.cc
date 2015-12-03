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

#include <sys/types.h>
#include <sys/stat.h>

#include <sstream>

#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_render.h>
#include <SDL2pp/Surface.hh>

#include "collision.hh"

std::string Tile::MakeTilePath(const SDL2pp::Point& coords) {
	std::stringstream filename;
	filename << DATADIR << "/" << coords.x << "/" << coords.y << ".png";
	return filename.str();
}

Tile::Tile(const SDL2pp::Point& coords)
	: coords_(coords),
	  color_mode_(ColorMode::EMPTY),
	  obstacle_mode_(ObstacleMode::UNIFORM),
	  is_obstacle_(false) {
	std::string path = MakeTilePath(coords).c_str();
	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		// temporary surface for image loading
		SDL2pp::Surface surface(path);
		assert(surface.GetWidth() >= tile_size_ && surface.GetHeight() >= tile_size_);

		SDL2pp::Surface::LockHandle lock = surface.Lock();

		// we only support palettes which tiles by fact are
		SDL_Color* palette = surface.Get()->format->palette ? surface.Get()->format->palette->colors : nullptr;
		assert(palette);
		assert(lock.GetFormat().BytesPerPixel == 1);

		// read pixels
		ColorMap pixels;
		ObstacleMap obstacle_map;
		pixels.reserve(tile_size_ * tile_size_ * 4);
		obstacle_map.reserve(tile_size_ * tile_size_);

		unsigned char* line = static_cast<unsigned char*>(lock.GetPixels());
		unsigned char* pixel = line;
		SDL_Color default_color{ palette[*pixel].r, palette[*pixel].g, palette[*pixel].b, palette[*pixel].a };
		bool default_obstacle = IsObstacle(palette[*pixel].r);
		bool same_color = true;
		bool same_obstacle = true;
		for (int y = 0; y < tile_size_; y++) {
			for (int x = 0; x < tile_size_; x++) {
				pixels.push_back(palette[*pixel].r);
				pixels.push_back(palette[*pixel].g);
				pixels.push_back(palette[*pixel].b);
				pixels.push_back(palette[*pixel].a);

				obstacle_map.push_back(IsObstacle(palette[*pixel].r));

				if (true && (
						palette[*pixel].r != default_color.r ||
						palette[*pixel].g != default_color.g ||
						palette[*pixel].b != default_color.b ||
						palette[*pixel].a != default_color.a
				   ))
					same_color = false;

				if (true && IsObstacle(palette[*pixel].r) != default_obstacle)
					same_obstacle = false;

				pixel++;
			}
			pixel = (line += lock.GetPitch());
		}

		// determine mode and save data
		if (same_color) {
			color_mode_ = ColorMode::SOLID;
			solid_color_ = default_color;
		} else {
			color_mode_ = ColorMode::PIXELS;
			color_data_ = new ColorMap(std::move(pixels));
		}

		if (same_obstacle) {
			obstacle_mode_ = ObstacleMode::UNIFORM;
			is_obstacle_ = default_obstacle;
		} else {
			obstacle_mode_ = ObstacleMode::MASK;
			try {
				obstacle_map_ = new ObstacleMap(std::move(obstacle_map));
			} catch (...) {
				if (color_mode_ == ColorMode::PIXELS)
					delete color_data_;
				throw;
			}
		}
	}
}

Tile::~Tile() {
	switch (color_mode_) {
	case ColorMode::PIXELS:
		delete color_data_;
		break;
	case ColorMode::TEXTURE:
		delete texture_;
		break;
	default:
		break;
	}

	if (obstacle_mode_ == ObstacleMode::MASK)
		delete obstacle_map_;
}

SDL2pp::Point Tile::GetCoords() const {
	return coords_;
}

SDL2pp::Rect Tile::GetRect() const {
	return SDL2pp::Rect(coords_.x * tile_size_, coords_.y * tile_size_, tile_size_, tile_size_);
}

void Tile::Materialize(SDL2pp::Renderer& renderer) {
	SDL2pp::Texture* texture = new SDL2pp::Texture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, tile_size_, tile_size_);
	texture->Update(SDL2pp::NullOpt, color_data_->data(), tile_size_ * 4);
	delete color_data_;
	texture_ = texture;
	color_mode_ = ColorMode::TEXTURE;
}

void Tile::Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport) {
	SDL2pp::Rect tilerect = GetRect();
	if (!tilerect.Intersects(viewport))
		return;

	SDL2pp::Rect render_rect = SDL2pp::Rect(tilerect.x - viewport.x, tilerect.y - viewport.y, tile_size_, tile_size_);

	switch (color_mode_) {
	case ColorMode::PIXELS:
		Materialize(renderer);
		// fallthrough
	case ColorMode::TEXTURE:
		renderer.Copy(*texture_,
				SDL2pp::Rect(0, 0, tile_size_, tile_size_),
				render_rect);
		break;
	case ColorMode::SOLID:
		renderer.SetDrawColor(solid_color_.r, solid_color_.g, solid_color_.b, solid_color_.a);
		renderer.FillRect(render_rect);
		break;
	default:
		break;
	}
}

void Tile::CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	auto real_rect = rect.GetIntersection(GetRect());
	if (!real_rect)
		return;

	if (obstacle_mode_ == ObstacleMode::UNIFORM) {
		if (is_obstacle_)
			coll.AddLeftCollision(real_rect->GetBottomRight());
		return;
	}

	SDL2pp::Point point;
	// This bit is interesting from the performance perspective
	// here we need to get rightmost (and if there are multiple rightmost,
	// bottommost of them) point for which obstacle mask is true
	//
	// We can either:
	// - scan whole rectangle from top to bottom and left to right and
	//   return the best matching point
	// - scan from the right, the from the bottom, and return the first
	//   matching point
	//
	// The second way is, in theory, really unfriendly to cache prefetching
	// (however modern CPUs may have optimization for backward scan as well),
	// but it lets us stop the scan early in some cases (player close to a
	// wall). The first way is cache-friendly, though it always scans the
	// while rectangle.
	//
	// There are other considerations, such as that we usually only need to
	// scan a thin (in horizontal direction) stripe, so prefetching doesn't
	// help us in any case.
	//
	// Anyway, I haven't done any tests and just chose a second variant
	// intuitively, but someone could do some research. Also, there could
	// be more clever data structure than a plain mask used to speed up
	// collision detection even more. It's not a bottleneck for this to
	// make sense though...
	for (point.x = real_rect->GetX2(); point.x >= real_rect->x; point.x--) {
		for (point.y = real_rect->GetY2(); point.y >= real_rect->y; point.y--) {
			if ((*obstacle_map_)[point.x - GetRect().x + tile_size_ * (point.y - GetRect().y)]) {
				coll.AddLeftCollision(point);
				return;
			}
		}
	}
}

void Tile::CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	auto real_rect = rect.GetIntersection(GetRect());
	if (!real_rect)
		return;

	if (obstacle_mode_ == ObstacleMode::UNIFORM) {
		if (is_obstacle_)
			coll.AddRightCollision(real_rect->GetBottomLeft());
		return;
	}

	SDL2pp::Point point;
	for (point.x = real_rect->x; point.x <= real_rect->GetX2(); point.x++) {
		for (point.y = real_rect->GetY2(); point.y >= real_rect->y; point.y--) {
			if ((*obstacle_map_)[point.x - GetRect().x + tile_size_ * (point.y - GetRect().y)]) {
				coll.AddRightCollision(point);
				return;
			}
		}
	}
}

void Tile::CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	auto real_rect = rect.GetIntersection(GetRect());
	if (!real_rect)
		return;

	if (obstacle_mode_ == ObstacleMode::UNIFORM) {
		if (is_obstacle_)
			coll.AddTopCollision(real_rect->GetY2());
		return;
	}

	SDL2pp::Point point;
	for (point.y = real_rect->GetY2(); point.y >= real_rect->y; point.y--) {
		for (point.x = real_rect->x; point.x <= real_rect->GetX2(); point.x++) {
			if ((*obstacle_map_)[point.x - GetRect().x + tile_size_ * (point.y - GetRect().y)]) {
				coll.AddTopCollision(point.y);
				return;
			}
		}
	}
}

void Tile::CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	auto real_rect = rect.GetIntersection(GetRect());
	if (!real_rect)
		return;

	if (obstacle_mode_ == ObstacleMode::UNIFORM) {
		if (is_obstacle_)
			coll.AddBottomCollision(real_rect->y);
		return;
	}

	SDL2pp::Point point;
	for (point.y = real_rect->y; point.y <= real_rect->GetY2(); point.y++) {
		for (point.x = real_rect->x; point.x <= real_rect->GetX2(); point.x++) {
			if ((*obstacle_map_)[point.x - GetRect().x + tile_size_ * (point.y - GetRect().y)]) {
				coll.AddBottomCollision(point.y);
				return;
			}
		}
	}
}
