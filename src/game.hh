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

	enum class PlayerDirection {
		FACING_LEFT,
		FACING_RIGHT,
	};

	enum class PlayerState {
		STILL      = 0,
		ASCENDING  = 1,
		MOVING     = 2,
		DESCENDING = 3,
	};

private:
	const static std::vector<SDL2pp::Point> coin_locations_;

	constexpr static int start_player_x_ = 512106;
	constexpr static int start_player_y_ = -549612;

	constexpr static int left_world_bound_ = 475136;
	constexpr static int right_world_bound_ = 567295;

	constexpr static int player_width_ = 29;
	constexpr static int player_height_ = 59;

	constexpr static int player_x1_margin_ = 0;
	constexpr static int player_y1_margin_ = 6;
	constexpr static int player_x2_margin_ = 0;
	constexpr static int player_y2_margin_ = 1;

	constexpr static int coin_size_ = 25;

	constexpr static float player_turn_speed_ = 20.0f;

	constexpr static float player_acceleration_ = 0.85f;
	constexpr static float player_max_speed_ = 20.0f;
	constexpr static float player_jump_force_ = -10.0f;

	constexpr static float drag_ = 0.15f;
	constexpr static float gravity_ = 0.3f;

	constexpr static float player_tangible_speed_ = 0.25f;
	constexpr static float player_speed_epsilon_ = 0.1f;

	constexpr static int max_step_height_ = 5;

	constexpr static SDL2pp::Rect deposit_area_rect_ = SDL2pp::Rect::FromCorners(512257, -549650, 512309, -549584);
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

	TileCache tile_cache_;

	// Controls
	int action_flags_ = 0;
	int prev_action_flags_ = 0;

	struct GameState {
		// Timing
		std::chrono::steady_clock::time_point deposit_message_expiration;
		std::chrono::steady_clock::time_point playarea_leave_moment;

		std::chrono::steady_clock::time_point session_start;

		// Some statistics used mainly for messaging
		bool player_moved = false;

		bool is_in_deposit_area = false;
		bool is_in_play_area = true;

		// Physics
		float player_x = start_player_x_;
		float player_y = start_player_y_;

		float player_xvel = 0.0f;
		float player_yvel = 0.0f;

		// Player sprite state
		float player_direction = 1.0f; // [-1.0..1.0]
		PlayerDirection player_target_direction = PlayerDirection::FACING_RIGHT;
		PlayerState player_state = PlayerState::STILL;

		// Coins
		std::vector<bool> picked_coins;

		GameState() : picked_coins(coin_locations_.size(), false) {
		}
	};

	GameState game_state_;

private:
	static std::string GetStatePath();

public:
	Game(SDL2pp::Renderer& renderer);
	~Game();

	void SetActionFlag(int flag);
	void ClearActionFlag(int flag);

	SDL2pp::Rect GetCameraRect() const;
	SDL2pp::Rect GetPlayerRect() const;
	SDL2pp::Rect GetPlayerCollisionRect() const;
	SDL2pp::Rect GetCoinRect(const SDL2pp::Point& coin) const;

	void LoadVisibleTiles();
	void Update(float delta_t);
	void Render();//const SDL2pp::Rect& viewport);

	void DepositCoins();

	void LoadState();
	void SaveState() const;
};

#endif // GAME_HH
