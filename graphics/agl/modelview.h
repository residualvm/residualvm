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

#ifndef AGL_MODELVIEW_H
#define AGL_MODELVIEW_H

#include "math/vector3d.h"

namespace Math {
class Angle;
}

namespace AGL {

class ModelView {
public:
	static void pushMatrix();
	static void popMatrix();

	static void translate(float x, float y, float z);
	inline static void translate(const Math::Vector3d &vec) { translate(vec.x(), vec.y(), vec.z()); }

	inline static void rotate(const Math::Angle &angle, const Math::Vector3d &axis) { rotate(angle, axis.x(), axis.y(), axis.z()); }
	static void rotate(const Math::Angle &angle, float x, float y, float z);

	static void scale(float x, float y, float z);
	inline static void scale(float factor) { scale(Math::Vector3d(factor, factor, factor)); }
	inline static void scale(const Math::Vector3d &factor) { scale(factor.x(), factor.y(), factor.z()); }
};

}

#endif
