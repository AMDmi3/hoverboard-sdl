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
	: coords_(coords) {
	std::string path = MakeTilePath(coords).c_str();
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		visual_data_.reset(new NoVisual);
		obstacle_data_.reset(new NoObstacle);
		return;
	}

	// temporary surface for image loading
	SDL2pp::Surface surface(path);
	assert(surface.GetWidth() >= tile_size_ && surface.GetHeight() >= tile_size_);

	SDL2pp::Surface::LockHandle lock = surface.Lock();

	// we only support palettes which tiles by fact are
	SDL_Color* palette = surface.Get()->format->palette ? surface.Get()->format->palette->colors : nullptr;
	assert(palette);
	assert(lock.GetFormat().BytesPerPixel == 1);

	// read pixels
	PixelVisual::PixelData pixels;
	ObstacleMap::Map obstacle_map;
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
	if (same_color)
		visual_data_.reset(new SolidVisual(default_color));
	else
		visual_data_.reset(new PixelVisual(std::move(pixels)));

	if (same_obstacle && default_obstacle)
		obstacle_data_.reset(new SolidObstacle());
	else if (same_obstacle && !default_obstacle)
		obstacle_data_.reset(new NoObstacle());
	else
		obstacle_data_.reset(new ObstacleMap(std::move(obstacle_map)));
}

Tile::~Tile() {
}

SDL2pp::Point Tile::GetCoords() const {
	return coords_;
}

SDL2pp::Rect Tile::GetRect() const {
	return SDL2pp::Rect(coords_.x * tile_size_, coords_.y * tile_size_, tile_size_, tile_size_);
}

void Tile::Materialize(SDL2pp::Renderer& renderer) {
	if (visual_data_->NeedsUpgrade()) {
		auto upgrade = visual_data_->Upgrade(renderer);
		visual_data_ = std::move(upgrade);
	}
}

void Tile::Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport) {
	if (!GetRect().Intersects(viewport))
		return;

	visual_data_->Render(renderer, GetRect().GetTopLeft() - viewport.GetTopLeft());
}

void Tile::CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	if (auto real_rect = rect.GetIntersection(GetRect()))
		obstacle_data_->CheckLeftCollision(coll, *real_rect - GetRect().GetTopLeft(), GetRect().GetTopLeft());
}

void Tile::CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	if (auto real_rect = rect.GetIntersection(GetRect()))
		obstacle_data_->CheckRightCollision(coll, *real_rect - GetRect().GetTopLeft(), GetRect().GetTopLeft());
}

void Tile::CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	if (auto real_rect = rect.GetIntersection(GetRect()))
		obstacle_data_->CheckTopCollision(coll, *real_rect - GetRect().GetTopLeft(), GetRect().GetTopLeft());
}

void Tile::CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const {
	if (auto real_rect = rect.GetIntersection(GetRect()))
		obstacle_data_->CheckBottomCollision(coll, *real_rect - GetRect().GetTopLeft(), GetRect().GetTopLeft());
}
