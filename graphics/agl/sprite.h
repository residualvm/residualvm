/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef AGL_SPRITE_H
#define AGL_SPRITE_H

#include "math/vector3d.h"

namespace AGL {

class Texture;

class Sprite {
public:
	Sprite(float width, float height);
	virtual ~Sprite();

	virtual void draw(Texture *tex, float x, float y, float z) const = 0;
	inline void draw(Texture *tex, const Math::Vector3d &pos) const { draw(tex, pos.x(), pos.y(), pos.z()); }

protected:
	inline float getWidth() const { return _width; }
	inline float getHeight() const { return _height; }

private:
	float _width;
	float _height;

};

}

#endif
