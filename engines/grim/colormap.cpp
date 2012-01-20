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

#include "engines/grim/colormap.h"
#include "engines/grim/resource.h"

namespace Grim {

// Load a colormap from the given data.
CMap::CMap(const Common::String &fileName, Common::SeekableReadStream *data) :
	Object(), _fname(fileName) {
	uint32 tag = data->readUint32BE();
	if (tag != MKTAG('C','M','P',' '))
		error("Invalid magic loading colormap");

	data->seek(64, SEEK_SET);
	data->read(_colors, sizeof(_colors));
}

CMap::~CMap() {
	if (g_resourceloader)
		g_resourceloader->uncacheColormap(this);
}

bool CMap::operator==(const CMap &c) const {
	if (_fname != c._fname) {
		return false;
	}

	return true;
}


} // end of namespace Grim

