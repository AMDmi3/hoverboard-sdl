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

#ifndef COLLISION_HH
#define COLLISION_HH

class CollisionInfo {
private:
	enum Collisions {
		LEFT = 0x01,
		RIGHT = 0x02,
		TOP = 0x04,
		BOTTOM = 0x08,
	};

	int flags_ = 0;

	SDL2pp::Point left_;
	SDL2pp::Point right_;
	int top_;
	int bottom_;

public:
	CollisionInfo() {
	}

	void SetLeftCollision(const SDL2pp::Point& left) {
		left_ = left;
		flags_ |= LEFT;
	}

	void SetRightCollision(const SDL2pp::Point& right) {
		right_ = right;
		flags_ |= RIGHT;
	}

	void SetTopCollision(int top) {
		top_ = top;
		flags_ |= TOP;
	}

	void SetBottomCollision(int bottom) {
		bottom_ = bottom;
		flags_ |= BOTTOM;
	}

	void AddLeftCollision(const SDL2pp::Point& left) {
		if (!(flags_ & LEFT) || left.x > left_.x || (left.x == left_.x && left.y > left_.y))
			SetLeftCollision(left);
	}

	void AddRightCollision(const SDL2pp::Point& right) {
		if (!(flags_ & RIGHT) || right.x < right_.x || (right.x == right_.x && right.y > right_.y))
			SetRightCollision(right);
	}

	void AddTopCollision(int top) {
		if (!(flags_ & TOP) || top > top_)
			SetTopCollision(top);
	}

	void AddBottomCollision(int bottom) {
		if (!(flags_ & BOTTOM) || bottom < bottom_)
			SetBottomCollision(bottom);
	}

	bool HasLeftCollision() const {
		return flags_ & LEFT;
	}

	bool HasRightCollision() const {
		return flags_ & RIGHT;
	}

	bool HasTopCollision() const {
		return flags_ & TOP;
	}

	bool HasBottomCollision() const {
		return flags_ & BOTTOM;
	}

	SDL2pp::Point GetLeftCollision() const {
		return left_;
	}

	SDL2pp::Point GetRightCollision() const {
		return right_;
	}

	int GetTopCollision() const {
		return top_;
	}

	int GetBottomCollision() const {
		return bottom_;
	}
};

#endif // COLLISION_HH
