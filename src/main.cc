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

#ifdef _WIN32
#	include <windows.h>
#endif

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

static const unsigned int AUTOSAVE_INTERVAL_MS = 5000;

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

	// We use extracted large icon because otherwise SDL_image will
	// small (16x16) icon which looks too ugly
	SDL2pp::Surface icon(HOVERBOARD_DATADIR "/xkcd.png");
	window.SetIcon(icon);

	SDL2pp::Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	Game game(renderer);

	game.LoadState();

	unsigned int prev_ticks = SDL_GetTicks();
	unsigned int prev_save_ticks = prev_ticks;

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

		if (frame_ticks - prev_save_ticks > AUTOSAVE_INTERVAL_MS) {
			game.SaveState();
			prev_save_ticks = frame_ticks;
		}

		// Frame limiter
		SDL_Delay(5);
	}

	return 0;
} catch (std::exception& e) {
#ifdef _WIN32
	MessageBox(nullptr, e.what(), "Error", MB_ICONERROR | MB_OK);
#else
	std::cerr << "Error: " << e.what() << std::endl;
#endif
	return 1;
} catch (...) {
#ifdef _WIN32
	MessageBox(nullptr, "Unknown error", "Error", MB_ICONERROR | MB_OK);
#else
	std::cerr << "Unknown error" << std::endl;
#endif
	return 1;
}

