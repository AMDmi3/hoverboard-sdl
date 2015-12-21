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

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include <SDL2/SDL.h>

#include <SDL2pp/SDL.hh>
#include <SDL2pp/SDLTTF.hh>
#include <SDL2pp/Window.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

#include "game.hh"

static const std::map<SDL_Keycode, int> teleport_slots = {
	{ SDLK_0, 0 },
	{ SDLK_1, 1 },
	{ SDLK_2, 2 },
	{ SDLK_3, 3 },
	{ SDLK_4, 4 },
	{ SDLK_5, 5 },
	{ SDLK_6, 6 },
	{ SDLK_7, 7 },
	{ SDLK_8, 8 },
	{ SDLK_9, 9 },
};

int main(int, char*[]) try {
	// SDL stuff
	SDL2pp::SDL sdl(SDL_INIT_VIDEO);
	SDL2pp::SDLTTF sdlttf;
	SDL2pp::Window window("Hoverboard", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 740, 700, SDL_WINDOW_RESIZABLE);
	SDL2pp::Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL2pp::Surface icon(DATADIR "/xkcd.ico");
	window.SetIcon(icon);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	Game game(renderer);

	game.LoadState();

	unsigned int prev_ticks = SDL_GetTicks();

	// Main loop
	while (1) {
		unsigned int frame_ticks = SDL_GetTicks();
		unsigned int frame_delta = frame_ticks - prev_ticks;
		prev_ticks = frame_ticks;

		// Process events
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				game.SaveState();
				return 0;
			} else if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE: case SDLK_q:
					game.SaveState();
					return 0;
				case SDLK_LEFT: case SDLK_a: case SDLK_h:
					game.SetActionFlag(Game::LEFT);
					break;
				case SDLK_RIGHT: case SDLK_d: case SDLK_l:
					game.SetActionFlag(Game::RIGHT);
					break;
				case SDLK_UP: case SDLK_w: case SDLK_k:
					game.SetActionFlag(Game::UP);
					break;
				case SDLK_DOWN: case SDLK_s: case SDLK_j:
					game.SetActionFlag(Game::DOWN);
					break;
				case SDLK_TAB:
					game.ToggleMinimap();
					break;
				}

				auto teleport_slot = teleport_slots.find(event.key.keysym.sym);
				if (teleport_slot != teleport_slots.end()) {
					if (SDL_GetModState() & KMOD_CTRL)
						game.SaveLocation(teleport_slot->second);
					else
						game.JumpToLocation(teleport_slot->second);
				}
			} else if (event.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
				case SDLK_LEFT: case SDLK_a: case SDLK_h:
					game.ClearActionFlag(Game::LEFT);
					break;
				case SDLK_RIGHT: case SDLK_d: case SDLK_l:
					game.ClearActionFlag(Game::RIGHT);
					break;
				case SDLK_UP: case SDLK_w: case SDLK_k:
					game.ClearActionFlag(Game::UP);
					break;
				case SDLK_DOWN: case SDLK_s: case SDLK_j:
					game.ClearActionFlag(Game::DOWN);
					break;
				}
			}
		}

		game.Update((float)frame_delta / 1000.0f, [&](int nloaded, int nmissing) {
				renderer.SetDrawColor(255, 255, 255);
				renderer.Clear();

				game.RenderProgressbar(nloaded, nmissing);

				renderer.Present();
			});

		// Render
		renderer.SetDrawColor(255, 255, 255);
		renderer.Clear();

		game.Render();

		renderer.Present();

		// Frame limiter
		SDL_Delay(5);
	}

	return 0;
} catch (std::exception& e) {
	std::cerr << "Error: " << e.what() << std::endl;
	return 1;
} catch (...) {
	std::cerr << "Unknown error" << std::endl;
	return 1;
}

