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

#include "game.hh"

#include <sys/types.h>
#include <sys/stat.h>

#include <memory>
#include <sstream>
#include <cmath>
#include <algorithm>

#include <SDL2pp/Surface.hh>

#include "collision.hh"

constexpr SDL2pp::Rect Game::deposit_area_rect_;
constexpr SDL2pp::Rect Game::play_area_rect_;
constexpr float Game::player_max_speed_;

Game::Game(SDL2pp::Renderer& renderer)
	: renderer_(renderer),
	  coin_texture_(renderer_, DATADIR "/coin.png"),
	  player_texture_(renderer_, DATADIR "/all-four.png"),
	  font_18_(DATADIR "/xkcd-Regular.otf", 18),
	  font_20_(DATADIR "/xkcd-Regular.otf", 20),
	  font_34_(DATADIR "/xkcd-Regular.otf", 34),
	  font_40_(DATADIR "/xkcd-Regular.otf", 40),
	  arrowkeys_message_(renderer_, font_18_.RenderText_Blended("use the arrow keys to move, esc/q to quit", SDL_Color{ 255, 255, 255, 192 })),
	  playarea_message_(renderer_, font_40_.RenderText_Blended("RETURN TO THE PLAY AREA", SDL_Color{ 255, 0, 0, 255 } )),
	  tile_cache_(renderer),
	  session_start_(std::chrono::steady_clock::now()),
	  coins_(coin_locations_) {
}

Game::~Game() {
}

void Game::SetActionFlag(int flag) {
	action_flags_ |= flag;
}

void Game::ClearActionFlag(int flag) {
	action_flags_ &= ~flag;
}

SDL2pp::Rect Game::GetCameraRect() const {
	SDL2pp::Rect rect(
			(int)player_x_ - renderer_.GetOutputWidth() / 2,
			(int)player_y_ - renderer_.GetOutputHeight() / 2,
			renderer_.GetOutputWidth(),
			renderer_.GetOutputHeight()
		);

	if (rect.x < left_world_bound_)
		rect.x = left_world_bound_;
	if (rect.GetX2() > right_world_bound_)
		rect.x -= rect.GetX2() - right_world_bound_;

	return rect;
}

SDL2pp::Rect Game::GetPlayerRect() const {
	return SDL2pp::Rect(
			(int)player_x_ - player_width_ + player_width_ / 2,
			(int)player_y_ - player_height_ + player_height_ / 2,
			player_width_,
			player_height_
		);
}

SDL2pp::Rect Game::GetPlayerCollisionRect() const {
	SDL2pp::Rect rect = GetPlayerRect();
	rect.x += player_x1_margin_;
	rect.y += player_y1_margin_;
	rect.w -= player_x1_margin_ + player_x2_margin_;
	rect.h -= player_y1_margin_ + player_y2_margin_;
	return SDL2pp::Rect(
			(int)player_x_ - player_width_ + player_width_ / 2,
			(int)player_y_ - player_height_ + player_height_ / 2,
			player_width_,
			player_height_
		);
}

SDL2pp::Rect Game::GetCoinRect(const SDL2pp::Point& coin) const {
	return SDL2pp::Rect(
			(int)coin.x - coin_size_ + coin_size_ / 2,
			(int)coin.y - coin_size_ + coin_size_ / 2,
			coin_size_,
			coin_size_
		);
}

void Game::Update(float delta_t) {
	// All original game constants work at 60 fps fixed frame
	// rate and do not take real frame time into account, so
	// we have to adjust these for our arbitrary fps.
	//
	// Linear values such as velocity may be converted from
	// original [units per frame] to our [units per second]
	// by simply dividing by original frame time and
	// multiplying by our frame time.
	//
	// Drag handling is a bit more complex, as it uses non-linear
	// progression ("speed *= 1 - drag" on each frame). Because
	// of that, it's asymptote (e.g. maximal speed) depends on
	// frame rate. We derive correction formula for it from from
	// a formula of sum of power series:
	//
	// vmax = (1 - drag) * acceleration / drag
	//
	// Next, we just derive corrected drag from the fact that
	// while acceleration changes by `fps_correction', vmax
	// should stay the same.
	//
	// To negate other effects of different time quantization
	// (which still give about +/- 10% position offset for
	// 30/1000 fps frame limiter may be tuned as well).

	const float fps_correction = 60.0f * delta_t;
	const float corrected_drag = drag_ * fps_correction / (1.0f - drag_ + drag_ * fps_correction);

	// Velocity updates caused by player actions
	if (action_flags_ & UP) {
		if (!(prev_action_flags_ & UP))
			player_yvel_ = player_jump_force_;
	}
	if (action_flags_ & LEFT) {
		player_target_direction_ = PlayerDirection::FACING_LEFT;
		player_xvel_ -= player_acceleration_ * fps_correction;
	}
	if (action_flags_ & RIGHT) {
		player_target_direction_ = PlayerDirection::FACING_RIGHT;
		player_xvel_ += player_acceleration_ * fps_correction;
	}

	// Velocity updates caused by world physics
	player_xvel_ *= 1.0 - corrected_drag;
	player_yvel_ += gravity_ * fps_correction;

	player_xvel_ = std::max(-player_max_speed_, std::min(player_xvel_, player_max_speed_));
	player_yvel_ = std::max(-player_max_speed_, std::min(player_yvel_, player_max_speed_));

	// Velocity updates caused by collisions
	CollisionInfo collisions_;
	tile_cache_.UpdateCollisions(collisions_, GetPlayerCollisionRect(), (int)std::ceil(player_max_speed_));

	if (collisions_.HasLeftCollision()) {
		int dist_to_left = (collisions_.GetLeftCollision().x - GetPlayerCollisionRect().x + 1);
		int step_height = GetPlayerCollisionRect().GetY2() - collisions_.GetLeftCollision().y + 1;

		if (player_xvel_ < -player_speed_epsilon_ && player_xvel_ < -(float)dist_to_left && step_height <= max_step_height_ && player_yvel_ * fps_correction > -step_height)
			player_yvel_ = -step_height / fps_correction;

		player_xvel_ = std::max(player_xvel_, (float)dist_to_left);
	}
	if (collisions_.HasRightCollision()) {
		int dist_to_right = collisions_.GetRightCollision().x - GetPlayerCollisionRect().GetX2() - 1;
		int step_height = GetPlayerCollisionRect().GetY2() - collisions_.GetRightCollision().y + 1;

		if (player_xvel_ > -player_speed_epsilon_ && player_xvel_ > (float)dist_to_right && step_height <= max_step_height_ && player_yvel_ * fps_correction > -step_height)
			player_yvel_ = -step_height / fps_correction;

		player_xvel_ = std::min(player_xvel_, (float)dist_to_right);
	}
	if (collisions_.HasTopCollision()) {
		int dist_to_top = collisions_.GetTopCollision() - GetPlayerCollisionRect().y + 1;
		player_yvel_ = std::max(player_yvel_, (float)dist_to_top);
	}
	if (collisions_.HasBottomCollision()) {
		int dist_to_bottom = collisions_.GetBottomCollision() - GetPlayerCollisionRect().GetY2() - 1;
		player_yvel_ = std::min(player_yvel_, (float)dist_to_bottom);
	}

	// Update player position
	player_x_ += player_xvel_ * fps_correction;
	player_y_ += player_yvel_ * fps_correction;

	if (action_flags_)
		player_moved_ = true;

	// Limit world
	if (GetPlayerRect().x < left_world_bound_)
		player_x_ += left_world_bound_ - GetPlayerRect().x;
	if (GetPlayerRect().GetX2() > right_world_bound_)
		player_x_ -= GetPlayerRect().GetX2() - right_world_bound_;

	// Calculate player state
	if (player_target_direction_ == PlayerDirection::FACING_LEFT)
		player_direction_ = std::max(player_direction_ - player_turn_speed_ * delta_t, -1.0f);
	else
		player_direction_ = std::min(player_direction_ + player_turn_speed_ * delta_t, 1.0f);

	if (player_yvel_ < -player_tangible_speed_)
		player_state_ = PlayerState::ASCENDING;
	else if (player_yvel_ > player_tangible_speed_)
		player_state_ = PlayerState::DESCENDING;
	else if (player_xvel_ < -player_tangible_speed_ || player_xvel_ > player_tangible_speed_)
		player_state_ = PlayerState::MOVING;
	else
		player_state_ = PlayerState::STILL;

	// Player rectangle
	SDL2pp::Rect player_rect = GetPlayerRect();

	// Collect coins
	coins_.remove_if([&](const SDL2pp::Point& coin){ return player_rect.Intersects(GetCoinRect(coin)); } );

	// Deposit coins
	if (player_rect.Intersects(deposit_area_rect_)) {
		if (!is_in_deposit_area_)
			DepositCoins();
		is_in_deposit_area_ = true;
	} else {
		is_in_deposit_area_ = false;
	}

	// Handle player leaving play area
	if (player_rect.Intersects(play_area_rect_)) {
		is_in_play_area_ = true;
	} else {
		if (is_in_play_area_)
			playarea_leave_moment_ = std::chrono::steady_clock::now();
		is_in_play_area_ = false;
	}

	// Update tile cache
	tile_cache_.UpdateCache(GetCameraRect(), 512, 512);

	prev_action_flags_ = action_flags_;
}

void Game::Render() {
	SDL2pp::Rect camerarect = GetCameraRect();

	tile_cache_.Render(camerarect);

	// draw coins
	for (auto& coin : coins_)
		renderer_.Copy(coin_texture_, SDL2pp::NullOpt, GetCoinRect(coin) - SDL2pp::Point(camerarect.x, camerarect.y));

	// draw player
	{
		int player_rect_shrink = (int)((float)GetPlayerRect().w / 2.0f * (1.0f - std::abs(player_direction_)));
		int flipflag = (player_direction_ < 0.0f) ? SDL_FLIP_HORIZONTAL : 0;
		renderer_.Copy(
				player_texture_,
				SDL2pp::Rect(GetPlayerRect().w * (int)player_state_, 0, GetPlayerRect().w, GetPlayerRect().h),
				GetPlayerRect().GetExtension(-player_rect_shrink, 0) - SDL2pp::Point(camerarect.x, camerarect.y),
				0.0f,
				SDL2pp::NullOpt,
				flipflag);
	}

	// draw messages
	if (std::chrono::steady_clock::now() < deposit_message_expiration_) {
		if (deposit_big_message_) {
			SDL2pp::Point pos(
					camerarect.w / 2 - deposit_big_message_->GetWidth() / 2,
					camerarect.h - deposit_big_message_->GetHeight() - 46
				);

			renderer_.Copy(*deposit_big_message_, SDL2pp::NullOpt, pos);
		}
		if (deposit_small_message_) {
			SDL2pp::Point pos(
					camerarect.w / 2 - deposit_small_message_->GetWidth() / 2,
					camerarect.h - deposit_small_message_->GetHeight() - 20
				);

			renderer_.Copy(*deposit_small_message_, SDL2pp::NullOpt, pos);
		}
	}

	if (!player_moved_) {
		SDL2pp::Point pos(
				camerarect.w / 2 - arrowkeys_message_.GetWidth() / 2,
				camerarect.h - arrowkeys_message_.GetHeight() - 20
			);

		renderer_.Copy(arrowkeys_message_, SDL2pp::NullOpt, pos);
	}

	if (!is_in_play_area_) {
		auto msec_since_escape = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - playarea_leave_moment_).count();

		if (msec_since_escape < 5 * 2500 && msec_since_escape % 2500 < 1500 && msec_since_escape % 500 < 250) {
			SDL2pp::Point pos(
					camerarect.w / 2 - playarea_message_.GetWidth() / 2,
					camerarect.h - playarea_message_.GetHeight() - 20
				);

			renderer_.Copy(playarea_message_, SDL2pp::NullOpt, pos);
		}
	}
}

void Game::DepositCoins() {
	size_t numcoins = coin_locations_.size() - coins_.size();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - session_start_).count();

	{
		std::stringstream message;

		message << "YOU GOT ";
		if (numcoins == 1)
			message << "A SINGLE COIN";
		else
			message << numcoins << " COINS";
		message << " IN " << seconds << " SECOND";
		if (seconds != 1)
			message << "S";

		deposit_big_message_.reset(new SDL2pp::Texture(renderer_, font_34_.RenderText_Blended(message.str(), SDL_Color{ 0xee, 0xd0, 0x00, 0xff } )));
	}

	{
		std::string message;

		if (numcoins == 0)
			message = "you successfully avoided all the coins!";
		else if (numcoins == 1)
			message = "it's a start.";
		else if (numcoins < 5)
			message = "not bad!";
		else if (numcoins < 10)
			message = "terrific!";
		else if (numcoins == 17)
			message = "you found all the coins! great job!";
		else if (numcoins == 42)
			message = "no answers here.";
		else if (numcoins == coin_locations_.size())
			message = "are you gandalf?";

		// In browser variant, this message is rendered with 26 size font, however
		// while browser renders letters as small caps (haven't checked, but I
		// assume this font doesn't have small letters), SDL_ttf renders them
		// as normal caps. Thus with SDL_ttf we have to take smaller font
		if (!message.empty())
			deposit_small_message_.reset(new SDL2pp::Texture(renderer_, font_20_.RenderText_Blended(message, SDL_Color{ 0xee, 0xd0, 0x00, 0xff } )));
		else
			deposit_small_message_.reset(nullptr);
	}

	coins_ = coin_locations_;
	session_start_ = std::chrono::steady_clock::now();
	deposit_message_expiration_ = std::chrono::steady_clock::now() + std::chrono::seconds(3);
}
