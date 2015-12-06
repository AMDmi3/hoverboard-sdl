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

#include <sys/stat.h>

#include <memory>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>

#include <SDL2pp/Surface.hh>

#include "collision.hh"

constexpr SDL2pp::Rect Game::deposit_area_rect_;
constexpr SDL2pp::Rect Game::play_area_rect_;
constexpr float Game::player_max_speed_;
constexpr int Game::portal_effect_duration_ms_;

Game::Game(SDL2pp::Renderer& renderer)
	: renderer_(renderer),
	  coin_texture_(renderer_, DATADIR "/coin.png"),
	  player_texture_(renderer_, DATADIR "/all-four.png"),
	  player_texture_b_(renderer_, DATADIR "/all-four-b.png"),
	  player_texture_y_(renderer_, DATADIR "/all-four-y.png"),
	  minimap_texture_(renderer_, DATADIR "/minimap.png"),
	  font_18_(DATADIR "/xkcd-Regular.otf", 18),
	  font_20_(DATADIR "/xkcd-Regular.otf", 20),
	  font_34_(DATADIR "/xkcd-Regular.otf", 34),
	  font_40_(DATADIR "/xkcd-Regular.otf", 40),
	  arrowkeys_message_(renderer_, font_18_.RenderText_Blended("use the arrow keys to move, esc/q to quit", SDL_Color{ 255, 255, 255, 192 })),
	  playarea_message_(renderer_, font_40_.RenderText_Blended("RETURN TO THE PLAY AREA", SDL_Color{ 255, 0, 0, 255 } )),
	  tile_cache_(renderer) {
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
			(int)game_state_.player_x - renderer_.GetOutputWidth() / 2,
			(int)game_state_.player_y - renderer_.GetOutputHeight() / 2,
			renderer_.GetOutputWidth(),
			renderer_.GetOutputHeight()
		);

	if (rect.x < left_world_bound_)
		rect.x = left_world_bound_;
	if (rect.GetX2() > right_world_bound_)
		rect.x -= rect.GetX2() - right_world_bound_;

	return rect;
}

SDL2pp::Rect Game::GetPlayerRect(float x, float y) const {
	return SDL2pp::Rect(
			(int)x - player_width_ + player_width_ / 2,
			(int)y - player_height_ + player_height_ / 2,
			player_width_,
			player_height_
		);
}

SDL2pp::Rect Game::GetPlayerRect() const {
	return GetPlayerRect(game_state_.player_x, game_state_.player_y);
}

SDL2pp::Rect Game::GetPlayerCollisionRect() const {
	SDL2pp::Rect rect = GetPlayerRect();
	rect.x += player_x1_margin_;
	rect.y += player_y1_margin_;
	rect.w -= player_x1_margin_ + player_x2_margin_;
	rect.h -= player_y1_margin_ + player_y2_margin_;
	return SDL2pp::Rect(
			(int)game_state_.player_x - player_width_ + player_width_ / 2,
			(int)game_state_.player_y - player_height_ + player_height_ / 2,
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
	auto now = std::chrono::steady_clock::now();

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
			game_state_.player_yvel = player_jump_force_;
	}
	if (action_flags_ & LEFT) {
		game_state_.player_target_direction = PlayerDirection::FACING_LEFT;
		game_state_.player_xvel -= player_acceleration_ * fps_correction;
	}
	if (action_flags_ & RIGHT) {
		game_state_.player_target_direction = PlayerDirection::FACING_RIGHT;
		game_state_.player_xvel += player_acceleration_ * fps_correction;
	}

	// Velocity updates caused by world physics
	game_state_.player_xvel *= 1.0 - corrected_drag;
	game_state_.player_yvel += gravity_ * fps_correction;

	game_state_.player_xvel = std::max(-player_max_speed_, std::min(game_state_.player_xvel, player_max_speed_));
	game_state_.player_yvel = std::max(-player_max_speed_, std::min(game_state_.player_yvel, player_max_speed_));

	// Velocity updates caused by collisions
	CollisionInfo collisions_;
	tile_cache_.UpdateCollisions(collisions_, GetPlayerCollisionRect(), (int)std::ceil(player_max_speed_));

	if (collisions_.HasLeftCollision()) {
		int dist_to_left = (collisions_.GetLeftCollision().x - GetPlayerCollisionRect().x + 1);
		int step_height = GetPlayerCollisionRect().GetY2() - collisions_.GetLeftCollision().y + 1;

		if (game_state_.player_xvel < -player_speed_epsilon_ && game_state_.player_xvel < -(float)dist_to_left && step_height <= max_step_height_ && game_state_.player_yvel * fps_correction > -step_height)
			game_state_.player_yvel = -step_height / fps_correction;

		game_state_.player_xvel = std::max(game_state_.player_xvel, (float)dist_to_left);
	}
	if (collisions_.HasRightCollision()) {
		int dist_to_right = collisions_.GetRightCollision().x - GetPlayerCollisionRect().GetX2() - 1;
		int step_height = GetPlayerCollisionRect().GetY2() - collisions_.GetRightCollision().y + 1;

		if (game_state_.player_xvel > -player_speed_epsilon_ && game_state_.player_xvel > (float)dist_to_right && step_height <= max_step_height_ && game_state_.player_yvel * fps_correction > -step_height)
			game_state_.player_yvel = -step_height / fps_correction;

		game_state_.player_xvel = std::min(game_state_.player_xvel, (float)dist_to_right);
	}
	if (collisions_.HasTopCollision()) {
		int dist_to_top = collisions_.GetTopCollision() - GetPlayerCollisionRect().y + 1;
		game_state_.player_yvel = std::max(game_state_.player_yvel, (float)dist_to_top);
	}
	if (collisions_.HasBottomCollision()) {
		int dist_to_bottom = collisions_.GetBottomCollision() - GetPlayerCollisionRect().GetY2() - 1;
		game_state_.player_yvel = std::min(game_state_.player_yvel, (float)dist_to_bottom);
	}

	// Update player position
	game_state_.player_x += game_state_.player_xvel * fps_correction;
	game_state_.player_y += game_state_.player_yvel * fps_correction;

	if (action_flags_)
		game_state_.player_moved = true;

	// Limit world
	if (GetPlayerRect().x < left_world_bound_)
		game_state_.player_x += left_world_bound_ - GetPlayerRect().x;
	if (GetPlayerRect().GetX2() > right_world_bound_)
		game_state_.player_x -= GetPlayerRect().GetX2() - right_world_bound_;

	// Calculate player state
	if (game_state_.player_target_direction == PlayerDirection::FACING_LEFT)
		game_state_.player_direction = std::max(game_state_.player_direction - player_turn_speed_ * delta_t, -1.0f);
	else
		game_state_.player_direction = std::min(game_state_.player_direction + player_turn_speed_ * delta_t, 1.0f);

	if (game_state_.player_yvel < -player_tangible_speed_)
		game_state_.player_state = PlayerState::ASCENDING;
	else if (game_state_.player_yvel > player_tangible_speed_)
		game_state_.player_state = PlayerState::DESCENDING;
	else if (game_state_.player_xvel < -player_tangible_speed_ || game_state_.player_xvel > player_tangible_speed_)
		game_state_.player_state = PlayerState::MOVING;
	else
		game_state_.player_state = PlayerState::STILL;

	// Player rectangle
	SDL2pp::Rect player_rect = GetPlayerRect();

	// Deposit coins
	if (player_rect.Intersects(deposit_area_rect_)) {
		if (!game_state_.is_in_deposit_area)
			DepositCoins();
		game_state_.is_in_deposit_area = true;
	} else {
		game_state_.is_in_deposit_area = false;

		// Collect coins (only if not in deposit area)
		for (size_t ncoin = 0; ncoin < coin_locations_.size(); ncoin++)
			if (!game_state_.picked_coins[ncoin] && player_rect.Intersects(GetCoinRect(coin_locations_[ncoin])))
				game_state_.picked_coins[ncoin] = true;
	}

	// Handle player leaving play area
	if (player_rect.Intersects(play_area_rect_)) {
		game_state_.is_in_play_area = true;
	} else {
		if (game_state_.is_in_play_area)
			game_state_.playarea_leave_moment = now;
		game_state_.is_in_play_area = false;
	}

	// remove obsolete portals
	portal_effects_.remove_if([now](const PortalEffect& e) {
			return e.start + std::chrono::milliseconds(portal_effect_duration_ms_) < now;
		});

	// Update tile cache
	tile_cache_.UpdateCache(GetCameraRect(), 512, 512);

	prev_action_flags_ = action_flags_;
}

void Game::Render() {
	SDL2pp::Rect camerarect = GetCameraRect();
	auto now = std::chrono::steady_clock::now();

	tile_cache_.Render(camerarect);

	// draw coins
	for (size_t ncoin = 0; ncoin < coin_locations_.size(); ncoin++)
		if (!game_state_.picked_coins[ncoin])
			renderer_.Copy(coin_texture_, SDL2pp::NullOpt, GetCoinRect(coin_locations_[ncoin]) - SDL2pp::Point(camerarect.x, camerarect.y));

	// draw portal effects
	for (auto& effect : portal_effects_) {
		// effect state in [0.0, 1.0]
		float effect_state = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - effect.start).count() / (float)portal_effect_duration_ms_;

		if (effect_state > 1.0)
			continue;

		// pixels to extend effect
		float effect_shrink = (effect.type == PortalEffect::ENTRY) ? portal_effect_size_ - effect_state * portal_effect_size_ : effect_state * portal_effect_size_;

		SDL2pp::Rect rect = GetPlayerRect(effect.player_x, effect.player_y);
		int player_rect_shrink = (int)((float)rect.w / 2.0f * (1.0f - std::abs(effect.player_direction)));
		int flipflag = (effect.player_direction < 0.0f) ? SDL_FLIP_HORIZONTAL : 0;

		// texture to use
		SDL2pp::Texture* texture;

		switch (effect.type) {
		case PortalEffect::ENTRY: texture = &player_texture_y_; break;
		case PortalEffect::EXIT:  texture = &player_texture_b_; break;
		default:                  texture = &player_texture_; break;
		}

		texture->SetAlphaMod((1.0 - effect_state) * 255.0);

		renderer_.Copy(
				*texture,
				SDL2pp::Rect(rect.w * (int)effect.player_state, 0, rect.w, rect.h),
				rect.GetExtension(-player_rect_shrink + (int)effect_shrink, (int)effect_shrink) - SDL2pp::Point(camerarect.x, camerarect.y),
				0.0f,
				SDL2pp::NullOpt,
				flipflag);

		texture->SetAlphaMod(255);
	}

	// draw player
	{
		int player_rect_shrink = (int)((float)GetPlayerRect().w / 2.0f * (1.0f - std::abs(game_state_.player_direction)));
		int flipflag = (game_state_.player_direction < 0.0f) ? SDL_FLIP_HORIZONTAL : 0;
		renderer_.Copy(
				player_texture_,
				SDL2pp::Rect(GetPlayerRect().w * (int)game_state_.player_state, 0, GetPlayerRect().w, GetPlayerRect().h),
				GetPlayerRect().GetExtension(-player_rect_shrink, 0) - SDL2pp::Point(camerarect.x, camerarect.y),
				0.0f,
				SDL2pp::NullOpt,
				flipflag);
	}

	// draw messages
	if (now < game_state_.deposit_message_expiration) {
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

	if (!game_state_.player_moved) {
		SDL2pp::Point pos(
				camerarect.w / 2 - arrowkeys_message_.GetWidth() / 2,
				camerarect.h - arrowkeys_message_.GetHeight() - 20
			);

		renderer_.Copy(arrowkeys_message_, SDL2pp::NullOpt, pos);
	}

	if (!game_state_.is_in_play_area) {
		auto msec_since_escape = std::chrono::duration_cast<std::chrono::milliseconds>(now - game_state_.playarea_leave_moment).count();

		if (msec_since_escape < 5 * 2500 && msec_since_escape % 2500 < 1500 && msec_since_escape % 500 < 250) {
			SDL2pp::Point pos(
					camerarect.w / 2 - playarea_message_.GetWidth() / 2,
					camerarect.h - playarea_message_.GetHeight() - 20
				);

			renderer_.Copy(playarea_message_, SDL2pp::NullOpt, pos);
		}
	}

	// minimap
	if (show_minimap_) {
		minimap_texture_.SetAlphaMod(192);
		renderer_.Copy(minimap_texture_, SDL2pp::NullOpt, SDL2pp::NullOpt);
	}
}

void Game::DepositCoins() {
	size_t numcoins = std::count(game_state_.picked_coins.begin(), game_state_.picked_coins.end(), true);
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - game_state_.session_start).count();

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

	std::fill(game_state_.picked_coins.begin(), game_state_.picked_coins.end(), false);
	game_state_.session_start = std::chrono::steady_clock::now();
	game_state_.deposit_message_expiration = std::chrono::steady_clock::now() + std::chrono::seconds(3);
}

std::string Game::GetStatePath() {
#ifdef STANDALONE
	return "hoverboard.state";
#else
	const char* home = getenv("HOME");
	const char* xdg_data_home = getenv("XDG_DATA_HOME");

	if (xdg_data_home != nullptr) {
		return std::string(xdg_data_home) + "/hoverboard/hoverboard.state";
	} else if (home != nullptr) {
		return std::string(home) + "/.local/share/hoverboard/hoverboard.state";
	} else {
		return "hoverboard.state";
	}
#endif
}

void Game::SaveState() const {
	std::string path = GetStatePath();

	// make directories
	size_t slashpos = 0;

	while ((slashpos = path.find('/', slashpos)) != std::string::npos) {
		if (slashpos != 0)
			mkdir(path.substr(0, slashpos).c_str(), 0777);
		slashpos++;
	}

	// save state
	std::ofstream statefile(path, std::ios::out | std::ios::trunc | std::ios::out);
	if (!statefile.good()) {
		std::cerr << "Warning: could not write game state to " << path << std::endl;
		return;
	}

	// savefile format version
	statefile << (int)0 << std::endl;

	// playtime
	statefile << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - game_state_.session_start).count() << std::endl;

	// player direction
	statefile << (game_state_.player_target_direction == PlayerDirection::FACING_RIGHT) << std::endl;

	// player coords
	statefile << std::setprecision(2);
	statefile << std::fixed;

	statefile << game_state_.player_x << " " << game_state_.player_y << std::endl;

	// coins
	for (size_t ncoin = 0; ncoin < coin_locations_.size(); ncoin++)
		statefile << (ncoin ? " " : "") << game_state_.picked_coins[ncoin];
	statefile << std::endl;

	// saved locations
	for (int nloc = 0; nloc < num_saved_locations_; nloc++) {
		if (game_state_.saved_locations[nloc])
			statefile << true << " " << game_state_.saved_locations[nloc]->first << " " << game_state_.saved_locations[nloc]->second << std::endl;
		else
			statefile << false << std::endl;
	}
}

void Game::LoadState() {
	std::string path = GetStatePath();

	std::ifstream statefile(path, std::ios::in);
	if (!statefile.good())
		return;

	// savefile format version
	int version;
	statefile >> version;

	GameState new_state;

	if (version == 0) {
		// playtime
		long playtime;
		statefile >> playtime;

		new_state.session_start = std::chrono::steady_clock::now() - std::chrono::seconds(playtime);

		// player direction
		bool right;
		statefile >> right;

		if (right) {
			new_state.player_target_direction = PlayerDirection::FACING_RIGHT;
			new_state.player_direction = 1.0;
		} else {
			new_state.player_target_direction = PlayerDirection::FACING_LEFT;
			new_state.player_direction = -1.0;
		}

		// player coords
		statefile >> new_state.player_x;
		statefile >> new_state.player_y;

		// coins
		for (size_t ncoin = 0; ncoin < coin_locations_.size(); ncoin++) {
			bool tmp;
			statefile >> tmp;
			new_state.picked_coins[ncoin] = tmp;
		}

		for (int nloc = 0; nloc < num_saved_locations_; nloc++) {
			bool active;
			float x, y;
			statefile >> active;

			if (active) {
				statefile >> x >> y;
				new_state.saved_locations[nloc] = std::make_pair(x, y);
			}
		}
	} else {
		std::cerr << "Warning: could not read game state from " << path << ", incompatible version " << version << std::endl;
		return;
	}

	// new state overrides
	new_state.is_in_deposit_area = true; // prevent re-deposit
	new_state.is_in_play_area = false;   // prevent "return to play area" message
	new_state.player_moved = true;       // prevent arrow keys message

	if (!statefile.good()) {
		std::cerr << "Warning: could not read game state from " << path << std::endl;
		return;
	}

	game_state_ = new_state;
}

void Game::SaveLocation(int n) {
	if (n >= 0 && n < num_saved_locations_) {
		game_state_.saved_locations[n] = std::make_pair(game_state_.player_x, game_state_.player_y);

		portal_effects_.emplace_back(
				PortalEffect {
					PortalEffect::SAVE,
					game_state_.player_x,
					game_state_.player_y,
					game_state_.player_direction,
					game_state_.player_state,
					std::chrono::steady_clock::now()
				}
			);
	}
}

void Game::JumpToLocation(int n) {
	if (n >= 0 && n < num_saved_locations_ && game_state_.saved_locations[n]) {
		portal_effects_.emplace_back(
				PortalEffect {
					PortalEffect::ENTRY,
					game_state_.player_x,
					game_state_.player_y,
					game_state_.player_direction,
					game_state_.player_state,
					std::chrono::steady_clock::now()
				}
			);

		game_state_.player_x = game_state_.saved_locations[n]->first;
		game_state_.player_y = game_state_.saved_locations[n]->second;

		portal_effects_.emplace_back(
				PortalEffect {
					PortalEffect::EXIT,
					game_state_.player_x,
					game_state_.player_y,
					game_state_.player_direction,
					game_state_.player_state,
					std::chrono::steady_clock::now()
				}
			);
	}
}

void Game::ToggleMinimap() {
	show_minimap_ = !show_minimap_;
}
