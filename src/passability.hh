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

#ifndef PASSABILITY_HH
#define PASSABILITY_HH

#include <vector>

class PassabilityMap {
private:
	std::vector<bool> mask_;

public:
	PassabilityMap() : mask_(512 * 512, false) {
	}

	void Set(int x, int y) {
		mask_[x + y * 512] = true;
	}

	bool Get(int x, int y) const {
		return mask_[x + y * 512];
	}
};

#endif // PASSABILITY_HH
