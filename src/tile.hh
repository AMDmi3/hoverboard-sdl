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

#ifndef TILES_HH
#define TILES_HH

#include <vector>

#include <SDL2pp/Point.hh>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

class CollisionInfo;

class Tile {
private:
	static constexpr int tile_size_ = 512;

private:
	// obstacle aspects
	class ObstacleData {
	public:
		virtual ~ObstacleData();

		virtual void CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const = 0;
		virtual void CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const = 0;
		virtual void CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const = 0;
		virtual void CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const = 0;
	};

	class NoObstacle : public ObstacleData {
	public:
		virtual ~NoObstacle();

		virtual void CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
	};

	class SolidObstacle : public ObstacleData {
	public:
		SolidObstacle();
		virtual ~SolidObstacle();

		virtual void CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
	};

	class ObstacleMap : public ObstacleData {
	public:
		typedef std::vector<bool> Map;

	private:
		Map map_;

	public:
		ObstacleMap(Map&& map);
		virtual ~ObstacleMap();

		virtual void CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
		virtual void CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& localrect, const SDL2pp::Point& offset) const final;
	};

	// visual aspects
	class TextureVisual;

	class VisualData {
	public:
		virtual ~VisualData();
		virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) = 0;

		virtual bool NeedsUpgrade() const;
		virtual std::unique_ptr<TextureVisual> Upgrade(SDL2pp::Renderer& renderer) const;
	};

	class NoVisual : public VisualData {
	public:
		virtual ~NoVisual();
		virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) final;
	};

	class SolidVisual : public VisualData {
	private:
		SDL_Color color_;

	public:
		SolidVisual(const SDL_Color& color);
		virtual ~SolidVisual();
		virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) final;
	};

	class PixelVisual : public VisualData {
	public:
		typedef std::vector<unsigned char> PixelData;

	private:
		PixelData pixels_;

	public:
		PixelVisual(PixelData&& pixels);
		virtual ~PixelVisual();

		virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) final;
		virtual bool NeedsUpgrade() const final;
		virtual std::unique_ptr<TextureVisual> Upgrade(SDL2pp::Renderer& renderer) const final;
	};

	class TextureVisual : public VisualData {
	private:
		SDL2pp::Texture texture_;

	public:
		TextureVisual(SDL2pp::Renderer& renderer, const PixelVisual::PixelData& pixels);
		virtual ~TextureVisual();

		virtual void Render(SDL2pp::Renderer& renderer, const SDL2pp::Point& offset) final;
	};

private:
	SDL2pp::Point coords_;

	std::unique_ptr<VisualData> visual_data_;
	std::unique_ptr<ObstacleData> obstacle_data_;

private:
	constexpr static int FloorDiv(int a, int b) {
		// signed division with flooring (instead of rounding to zero)
		return a < 0 ? (a - b + 1) / b : a / b;
	}

	constexpr static bool IsObstacle(unsigned char color) {
		return color < 100 && !(color & 1);
	}

	static std::string MakeTilePath(const SDL2pp::Point& coords);

public:
	static SDL2pp::Point TileForPoint(const SDL2pp::Point& p) {
		return SDL2pp::Point(FloorDiv(p.x, tile_size_), FloorDiv(p.y, tile_size_));
	}

public:
	Tile(const SDL2pp::Point& coords);
	~Tile();

	SDL2pp::Point GetCoords() const;
	SDL2pp::Rect GetRect() const;

	void Materialize(SDL2pp::Renderer& renderer);
	void Render(SDL2pp::Renderer& renderer, const SDL2pp::Rect& viewport);

	void CheckLeftCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
	void CheckRightCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
	void CheckTopCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
	void CheckBottomCollision(CollisionInfo& coll, const SDL2pp::Rect& rect) const;
};

#endif // TILES_HH
