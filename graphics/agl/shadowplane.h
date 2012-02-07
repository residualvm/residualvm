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

#ifndef AGL_SHADOWPLANE_H
#define AGL_SHADOWPLANE_H

#include "common/array.h"

#include "math/vector3d.h"

namespace Graphics {
class Color;
}

namespace AGL {

class ShadowPlane {
public:
	typedef Common::Array<Math::Vector3d> Vertices;

	ShadowPlane();
	virtual ~ShadowPlane();

	void addSector(const Vertices &vertices);

	virtual void enable(const Math::Vector3d &pos, const Graphics::Color &color) = 0;
	virtual void disable() = 0;

protected:
	struct Sector {
		Vertices _vertices;
		Math::Vector3d _normal;
	};

	inline const Common::Array<Sector> &getSectors() const { return _sectors; }

	void resetShouldUpdateFlag();
	inline bool shouldUpdate() const { return _shouldUpdate; }

private:
	Common::Array<Sector> _sectors;
	bool _shouldUpdate;
};

}

#endif
