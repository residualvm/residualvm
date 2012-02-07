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

#include "graphics/agl/primitive.h"

namespace AGL {

Primitive::Primitive()
	: _useGlobalColor(false) {
	newSubPolygon();
}

Primitive::~Primitive() {

}

void Primitive::begin(DrawMode mode) {
	_mode = mode;
}

void Primitive::vertex(float x, float y) {
	_currentPrim->_vertices.push_back(x);
	_currentPrim->_vertices.push_back(y);
	++_currentPrim->_numVertices;
}

void Primitive::vertex(const Math::Vector2d &v) {
	_currentPrim->_vertices.push_back(v.getX());
	_currentPrim->_vertices.push_back(v.getY());
	++_currentPrim->_numVertices;
}

void Primitive::color(const Graphics::Color &c) {
	_currentPrim->_colors.push_back(c.getRed());
	_currentPrim->_colors.push_back(c.getGreen());
	_currentPrim->_colors.push_back(c.getBlue());
	_currentPrim->_colors.push_back(c.getAlpha());
}

void Primitive::newSubPolygon() {
	_prims.push_back(SubPrimitive());
	_currentPrim = &_prims.back();
	_currentPrim->_numVertices = 0;
}

void Primitive::setGlobalColor(const Graphics::Color &c) {
	_globalColor = c;
	_useGlobalColor = true;
}

void Primitive::setVertex(uint sub, uint i, const Math::Vector2d &v) {
	memcpy(_prims[sub]._vertices.begin() + 2 * i, v.getData(),  2 * sizeof(float));
}

void Primitive::setColor(uint sub, uint i, const Graphics::Color &c) {
	memcpy(_prims[sub]._colors.begin() + 4 * i, c.getData(), 4);
}

}
