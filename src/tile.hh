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

#ifndef TILES_HH
#define TILES_HH

#include <vector>

#include <SDL2pp/Point.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

class CollisionInfo;

class Tile {
private:
	static constexpr int tile_size_ = 512;

private:
	enum class ColorMode {
		EMPTY,
		PIXELS,
		TEXTURE,
		SOLID,
	};

	enum class ObstacleMode {
		UNIFORM,
		MASK,
	};

	typedef std::vector<unsigned char> ColorMap;
	typedef std::vector<bool> ObstacleMap;

private:
	SDL2pp::Point coords_;

	ColorMode color_mode_;
	ObstacleMode obstacle_mode_;

	union {
		ColorMap* color_data_;
		SDL2pp::Texture* texture_;
		SDL_Color solid_color_;
	};
	union {
		ObstacleMap* obstacle_map_;
		bool is_obstacle_;
	};

private:
	constexpr static int FloorDiv(int a, int b) {
		// signed division with flooring (instead of rounding to zero)
		return a < 0 ? (a - b + 1) / b : a / b;
	}

	constexpr static bool IsObstacle(unsigned char color) {
		return color < 100 && !(color & 1);
	}

	static std::string MakeTilePath(const SDL2pp::Point& coords);

public:
	static SDL2pp::Point TileForPoint(const SDL2pp::Point& p) {
		return SDL2pp::Point(FloorDiv(p.x, tile_size_), FloorDiv(p.y, tile_size_));
	}

public:
	Tile(const SDL2pp::Point& coords);
	virtual ~Tile();

	SDL2pp::Point GetCoords() const;
	SDL2pp::Rect GetRect() const;

	void Materialize(SDL2pp::Renderer& renderer);
	void Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport);

	void CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
	void CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
	void CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
	void CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
};

#endif // TILES_HH
