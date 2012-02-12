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

#include "graphics/color.h"

namespace Graphics {

Color::Color() {
	memset(_vals, 0, 4);
}

Color::Color(uint8 r, uint8 g, uint8 b, uint8 a) {
	set(r, g, b, a);
}

Color::Color(const Color &c) {
	*this = c;
}

void Color::setRed(uint8 r) {
	_vals[0] = r;
}

void Color::setGreen(uint8 g) {
	_vals[1] = g;
}

void Color::setBlue(uint8 b) {
	_vals[2] = b;
}

void Color::setAlpha(uint8 a) {
	_vals[3] = a;
}

void Color::set(uint8 r, uint8 g, uint8 b, uint8 a) {
	_vals[0] = r;
	_vals[1] = g;
	_vals[2] = b;
	_vals[3] = a;
}

Color &Color::operator=(const Color &c) {
	memcpy(_vals, c._vals, 4);

	return *this;
}

}

Common::Debug &operator<<(Common::Debug dbg, const Graphics::Color &c) {
	dbg.nospace() << "Color(" << c.getRed() << ", " << c.getGreen() << ", " << c. getBlue() << ", " << c.getAlpha() << ")";

	return dbg.space();
}
