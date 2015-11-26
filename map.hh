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

#ifndef MAP_HH
#define MAP_HH

#include <map>
#include <set>

#include <SDL2pp/Rect.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Renderer.hh>

class Map {
private:
	SDL2pp::Renderer& renderer_;

	std::set<SDL2pp::Point> absent_tiles_;
	std::map<SDL2pp::Point, SDL2pp::Texture> tiles_;

private:
	static std::string MakeTilePath(SDL2pp::Point tile);

	constexpr static int floordiv(int a, int b) {
		// signed division with flooring (instead of rounding to zero)
		return a < 0 ? (a - b + 1) / b : a / b;
	}

public:
	Map(SDL2pp::Renderer& renderer);
	~Map();

	void Render(SDL2pp::Rect rect);
};

#endif // MAP_HH
