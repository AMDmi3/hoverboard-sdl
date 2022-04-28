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

#include <SDL_render.h>

#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

Tile::VisualData::~VisualData() {
}

bool Tile::VisualData::NeedsUpgrade() const {
	return false;
}

std::unique_ptr<Tile::TextureVisual> Tile::VisualData::Upgrade(SDL2pp::Renderer&) const {
	return nullptr;
}

Tile::NoVisual::~NoVisual() {
}

void Tile::NoVisual::Render(SDL2pp::Renderer&, const SDL2pp::Point&) {
}

Tile::SolidVisual::SolidVisual(const SDL_Color& color) : color_(color) {
}

Tile::SolidVisual::~SolidVisual() {
}

void Tile::SolidVisual::Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) {
	renderer.SetDrawColor(color_.r, color_.g, color_.b, color_.a);
	renderer.FillRect(SDL2pp::Rect(offset.x, offset.y, tile_size_, tile_size_));
}

Tile::PixelVisual::PixelVisual(PixelVisual::PixelData&& pixels) : pixels_(std::move(pixels)) {
}

Tile::PixelVisual::~PixelVisual() {
}

bool Tile::PixelVisual::NeedsUpgrade() const {
	return true;
}

std::unique_ptr<Tile::TextureVisual> Tile::PixelVisual::Upgrade(SDL2pp::Renderer& renderer) const {
	return std::unique_ptr<Tile::TextureVisual>(new TextureVisual(renderer, pixels_));
}

void Tile::PixelVisual::Render(SDL2pp::Renderer&, const SDL2pp::Point&) {
}

Tile::TextureVisual::TextureVisual(SDL2pp::Renderer& renderer, const Tile::PixelVisual::PixelData& pixels)
	: texture_(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, tile_size_, tile_size_) {
	texture_.Update(SDL2pp::NullOpt, pixels.data(), tile_size_ * 4);
}

Tile::TextureVisual::~TextureVisual() {
}

void Tile::TextureVisual::Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) {
	renderer.Copy(texture_, SDL2pp::NullOpt, offset);
}
