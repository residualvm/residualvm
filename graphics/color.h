/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#ifndef GRAPHICS_COLOR_H
#define GRAPHICS_COLOR_H

#include "common/system.h"
#include "common/streamdebug.h"

namespace Graphics {

class Color {
public:
	Color();
	Color(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	Color(const Color &c);

	inline uint8 getRed() const { return _vals[0]; }
	inline uint8 getGreen() const { return _vals[1]; }
	inline uint8 getBlue() const { return _vals[2]; }
	inline uint8 getAlpha() const { return _vals[3]; }

	inline const uint8 *getData() const { return _vals; }

	void setRed(uint8 r);
	void setGreen(uint8 r);
	void setBlue(uint8 r);
	void setAlpha(uint8 a);

	void set(uint8 r, uint8 g, uint8 b, uint8 a = 255);

	Color &operator=(const Color &c);

private:
	uint8 _vals[4];
};

}

Common::Debug &operator<<(Common::Debug dbg, const Graphics::Color &c);

#endif
