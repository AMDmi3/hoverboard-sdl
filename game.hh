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
#include <list>
#include <vector>
#include <chrono>
#include <memory>

#include <SDL2pp/Rect.hh>
#include <SDL2pp/Texture.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Font.hh>

#include "tilecache.hh"

class Game {
public:
	enum ActionFlags {
		UP    = 0x01,
		DOWN  = 0x02,
		LEFT  = 0x04,
		RIGHT = 0x08,
	};

private:
	typedef std::list<SDL2pp::Point> CoinList;

private:
	const static CoinList coin_locations_;

	constexpr static int start_player_x_ = 512106;
	constexpr static int start_player_y_ = -549612;

	constexpr static int left_world_bound_ = 475136;
	constexpr static int right_world_bound_ = 567295;

	constexpr static int player_width_ = 29;
	constexpr static int player_height_ = 59;

	constexpr static int coin_size_ = 25;

	constexpr static SDL2pp::Rect deposit_rect_ = SDL2pp::Rect::FromCorners(512257, -549650, 512309, -549584);
	constexpr static SDL2pp::Rect play_area_rect_ = SDL2pp::Rect::FromCorners(511484, -550619, 513026, -549568);

private:
	SDL2pp::Renderer& renderer_;

	// Resources
	SDL2pp::Texture coin_texture_;
	SDL2pp::Texture player_texture_;
	SDL2pp::Font font_18_;
	SDL2pp::Font font_20_;
	SDL2pp::Font font_34_;
	SDL2pp::Font font_40_;

	SDL2pp::Texture arrowkeys_message_;
	SDL2pp::Texture playarea_message_;

	std::unique_ptr<SDL2pp::Texture> deposit_big_message_;
	std::unique_ptr<SDL2pp::Texture> deposit_small_message_;

	TileCache tc_;

	// Message-related statistics
	bool player_moved_ = false;

	std::chrono::steady_clock::time_point deposit_message_expiration_;
	std::chrono::steady_clock::time_point playarea_leave_moment_;

	std::chrono::steady_clock::time_point session_start_;

	bool is_depositing_ = false;
	bool is_in_play_area_ = true;

	// Game state
	int action_flags_ = 0;

	float player_x_ = start_player_x_;
	float player_y_ = start_player_y_;

	CoinList coins_;

public:
	Game(SDL2pp::Renderer& renderer);
	~Game();

	void SetActionFlag(int flag);
	void ClearActionFlag(int flag);

	SDL2pp::Rect GetCameraRect() const;
	SDL2pp::Rect GetPlayerRect() const;
	SDL2pp::Rect GetCoinRect(const SDL2pp::Point& coin) const;

	void Update(float delta_t);
	void Render();//const SDL2pp::Rect& viewport);

	void DepositCoins();
};

#endif // GAME_HH
