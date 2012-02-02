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

#ifndef GRIM_COSTUMEEMI_H
#define GRIM_COSTUMEEMI_H

#include "common/stream.h"

#include "engines/grim/object.h"
#include "engines/grim/costume.h"

namespace Grim {

typedef uint32 tag32;

class EMISkelComponent;
class EMIMeshComponent;

class CostumeEMI : public Costume {
public:
	CostumeEMI(const Common::String &filename, Common::SeekableReadStream *data, Costume *prevCost);

	void load(Common::SeekableReadStream *data, Costume *prevCost);

	int update(uint frameTime);
	void draw();
	void setPosRotate(Math::Vector3d pos, const Math::Angle &pitch,
					  const Math::Angle &yaw, const Math::Angle &roll);

	void saveState(SaveGame *state) const;
	bool restoreState(SaveGame *state);

public:
	Component *loadComponent(Component *parent, int parentID, const char *name, Component *prevComponent);

	EMISkelComponent *_emiSkel;
	EMIMeshComponent *_emiMesh;

	friend class Chore;
};

} // end of namespace Grim

#endif
