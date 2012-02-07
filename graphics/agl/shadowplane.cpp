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

#include "graphics/agl/shadowplane.h"

namespace AGL {

ShadowPlane::ShadowPlane() {
	_shouldUpdate = false;
}

ShadowPlane::~ShadowPlane() {

}

void ShadowPlane::addSector(const ShadowPlane::Vertices &vert) {
	Sector s;
	s._vertices = vert;

	assert(vert.size() > 2);
	s._normal = Math::Vector3d::crossProduct(vert[1] - vert[0], vert[vert.size() - 1] - vert[0]);

	_sectors.push_back(s);

	_shouldUpdate = true;
}

void ShadowPlane::resetShouldUpdateFlag() {
	_shouldUpdate = false;
}

}
