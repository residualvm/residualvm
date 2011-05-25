/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
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

#ifndef GRIM_RESOURCE_H
#define GRIM_RESOURCE_H

#include "common/archive.h"
#include "common/file.h"

#include "engines/grim/object.h"

namespace Grim {

class Bitmap;
class CMap;
class Costume;
class Font;
class KeyframeAnim;
class Material;
class Model;
class LipSync;
class TrackedObject;
class SaveGame;
class Block;
class LuaFile;
class Lab;

class ResourceLoader {
public:
	ResourceLoader();
	~ResourceLoader();

	template<typename T>
	T *loadResource(const Common::String &fname, void *additionalData = 0, int errorLevel = 0);

	template<typename T>
	ObjectPtr<T> getResource(const Common::String &fname, void *additionalData = 0, int errorLevel = 0);

	Bitmap *loadBitmap(const Common::String &fname);
	Costume *loadCostume(const Common::String &fname, Costume *prevCost);

	Block *getFileBlock(const Common::String &filename) const;
	Common::File *openNewStreamFile(const char *filename) const;
	LuaFile *openNewStreamLuaFile(const char *filename) const;
	bool getFileExists(const Common::String &filename) const;

	ObjectPtr<Material> getMaterial(const Common::String &fnamefilename, CMap *c);
	ObjectPtr<Model> getModel(const Common::String &fname, CMap *c);
	ObjectPtr<CMap> getColormap(const Common::String &fname);
	ObjectPtr<KeyframeAnim> getKeyframe(const Common::String &fname);
	ObjectPtr<Font> getFont(const Common::String &fname);
	ObjectPtr<LipSync> getLipSync(const Common::String &fname);

private:
	const Lab *getLab(const Common::String &filename) const;

	typedef Common::List<Lab *> LabList;
	LabList _labs;

	Common::HashMap<Common::String, ObjectPtr<Object>> _objectCache;
};

extern ResourceLoader *g_resourceloader;

} // end of namespace Grim

#endif
