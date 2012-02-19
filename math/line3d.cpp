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
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "math/line3d.h"

namespace Math {

Line3d::Line3d() {

}

Line3d::Line3d(const Vector3d &b, const Vector3d &e) :
	_begin(b), _end(e) {

}

Line3d::Line3d(const Line3d &other) {
	*this = other;
}

Math::Vector3d Line3d::begin() const {
	return _begin;
}

Math::Vector3d Line3d::end() const {
	return _end;
}

Math::Vector3d Line3d::middle() const {
	return (_begin + _end) / 2.f;
}

bool Line3d::intersectLine2d(const Line3d &other, Math::Vector3d *pos, bool useXZ) {

	float denom, nume_a;
	if (useXZ) {
		denom = ((other._end.z() - other._begin.z()) * (_end.x() - _begin.x())) -
		((other._end.x() - other._begin.x()) * (_end.z() - _begin.z()));

		nume_a = ((other._end.x() - other._begin.x()) * (_begin.z() - other._begin.z())) -
		((other._end.z() - other._begin.z()) * (_begin.x() - other._begin.x()));
	} else {
		denom = ((other._end.y() - other._begin.y()) * (_end.x() - _begin.x())) -
		((other._end.x() - other._begin.x()) * (_end.y() - _begin.y()));

		nume_a = ((other._end.x() - other._begin.x()) * (_begin.y() - other._begin.y())) -
		((other._end.y() - other._begin.y()) * (_begin.x() - other._begin.x()));
	}

	if (denom == 0.0f) {
		return false;
	}

	float ua = nume_a / denom;

	if (ua < 0)
		ua = 0;
	if (ua > 1)
		ua = 1;

	// Get the intersection point.
	if (pos)
		*pos = _begin + ua * (_end - _begin);

	return true;
}

void Line3d::operator=(const Line3d &other) {
	_begin = other._begin;
	_end = other._end;
}

}
