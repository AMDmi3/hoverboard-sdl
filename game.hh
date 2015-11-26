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

#ifndef GAME_HH
#define GAME_HH

#include <map>
#include <set>
#include <vector>

#include <SDL2pp/Rect.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Renderer.hh>

#include "tilecache.hh"

class Game {
private:
	const static std::vector<SDL2pp::Point> coin_locations_;

private:
	SDL2pp::Renderer& renderer_;

	SDL2pp::Texture coin_texture_;
	SDL2pp::Texture player_texture_;

	TileCache tc_;

public:
	Game(SDL2pp::Renderer& renderer);
	~Game();

	void Render(SDL2pp::Rect rect);
};

#endif // GAME_HH
