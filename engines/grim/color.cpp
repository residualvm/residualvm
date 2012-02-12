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

#include "engines/grim/color.h"
#include "engines/grim/savegame.h"

namespace Grim {

Color::Color() {
	_vals[0] = _vals[1] = _vals[2] = 0;
}

Color::Color(byte r, byte g, byte b) {
	_vals[0] = r;
	_vals[1] = g;
	_vals[2] = b;
}

Color::Color(const Color& c) {
	_vals[0] = c._vals[0];
	_vals[1] = c._vals[1];
	_vals[2] = c._vals[2];
}

Color::Color(uint32 c) {
	_vals[0] = (c >> 16) & 0xFF;
	_vals[1] = (c >> 8) & 0xFF;
	_vals[2] = c & 0xFF;
}

Color::Color(const Graphics::Color &c) {
	memcpy(_vals, c.getData(), 3);
}

uint32 Color::toEncodedValue() {
	return	(_vals[0] << 16) |
			(_vals[1] << 8 ) |
			 _vals[2];
}

Color& Color::operator =(const Color &c) {
	_vals[0] = c._vals[0];
	_vals[1] = c._vals[1];
	_vals[2] = c._vals[2];
	return *this;
}

Color::operator Graphics::Color() const {
	return Graphics::Color(_vals[0], _vals[1], _vals[2]);
}

} // end of namespace Grim

