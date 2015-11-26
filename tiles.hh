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

#include <SDL2pp/Point.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

class Tile {
protected:
	static constexpr int image_size_ = 513;
	static constexpr int tile_size_ = 512;

private:
	SDL2pp::Point coords_;

private:
	constexpr static int floordiv(int a, int b) {
		// signed division with flooring (instead of rounding to zero)
		return a < 0 ? (a - b + 1) / b : a / b;
	}

public:
	static SDL2pp::Point TileForPoint(const SDL2pp::Point& p) {
		return SDL2pp::Point(floordiv(p.x, tile_size_), floordiv(p.y, tile_size_));
	}

public:
	Tile(const SDL2pp::Point& coords);
	virtual ~Tile();

	SDL2pp::Point GetCoords() const;
	SDL2pp::Rect GetRect() const;

	virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport) = 0;
};

class EmptyTile : public Tile {
public:
	EmptyTile(const SDL2pp::Point& coords);
	virtual ~EmptyTile();

	virtual void Render(SDL2pp::Renderer&, const SDL2pp::Rect&) final;
};

class TextureTile : public Tile {
private:
	SDL2pp::Texture texture_;

public:
	TextureTile(const SDL2pp::Point& coords, SDL2pp::Texture&& texture);
	virtual ~TextureTile();

	virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport) final;
};

#endif // TILES_HH
