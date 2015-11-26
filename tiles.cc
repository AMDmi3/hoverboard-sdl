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

#include "tiles.hh"

Tile::Tile(const SDL2pp::Point& coords) : coords_(coords) {
}

Tile::~Tile() {
}

SDL2pp::Point Tile::GetCoords() const {
	return coords_;
}

SDL2pp::Rect Tile::GetRect() const {
	return SDL2pp::Rect(coords_.x * tile_size_, coords_.y * tile_size_, tile_size_, tile_size_);
}

EmptyTile::EmptyTile(const SDL2pp::Point& coords) : Tile(coords) {
}

EmptyTile::~EmptyTile() {
}

void EmptyTile::Render(SDL2pp::Renderer&, const SDL2pp::Rect&) {
}

TextureTile::TextureTile(const SDL2pp::Point& coords, SDL2pp::Texture&& texture) : Tile(coords), texture_(std::move(texture)) {
}

TextureTile::~TextureTile() {
}

void TextureTile::Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport) {
	SDL2pp::Rect tilerect = GetRect();
	if (!tilerect.Intersects(viewport))
		return;

	renderer.Copy(texture_,
			SDL2pp::Rect(0, 0, tile_size_, tile_size_),
			SDL2pp::Point(tilerect.x, tilerect.y) - SDL2pp::Point(viewport.x, viewport.y));
}
