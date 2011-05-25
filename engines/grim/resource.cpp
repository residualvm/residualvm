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

#include "engines/grim/resource.h"
#include "engines/grim/colormap.h"
#include "engines/grim/costume.h"
#include "engines/grim/keyframe.h"
#include "engines/grim/material.h"
#include "engines/grim/grim.h"
#include "engines/grim/lipsync.h"
#include "engines/grim/savegame.h"
#include "engines/grim/actor.h"
#include "engines/grim/lab.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/font.h"

namespace Grim {

ResourceLoader *g_resourceloader = NULL;

ResourceLoader::ResourceLoader() {
	int lab_counter = 0;

	Lab *l;
	Common::ArchiveMemberList files;

	SearchMan.listMatchingMembers(files, "*.lab");
	SearchMan.listMatchingMembers(files, "*.m4b");
	if (g_grim->getGameFlags() & ADGF_DEMO)
		SearchMan.listMatchingMembers(files, "*.mus");

	if (files.empty())
		error("Cannot find game data - check configuration file");

	for (Common::ArchiveMemberList::const_iterator x = files.begin(); x != files.end(); ++x) {
		const Common::String filename = (*x)->getName();
		l = new Lab();

		if (l->open(filename)) {
			if (filename.equalsIgnoreCase("data005.lab"))
				_labs.push_front(l);
			else
				_labs.push_back(l);
			lab_counter++;
		} else {
			delete l;
		}
	}

	files.clear();
}

ResourceLoader::~ResourceLoader() {
	while (!_labs.empty()) {
		Lab *p = _labs.front();
		_labs.erase(_labs.begin());
		delete p;
	}
}

const Lab *ResourceLoader::getLab(const Common::String &filename) const {
	for (LabList::const_iterator i = _labs.begin(); i != _labs.end(); ++i)
		if ((*i)->getFileExists(filename))
			return *i;

	return NULL;
}

bool ResourceLoader::getFileExists(const Common::String &filename) const {
	return getLab(filename) != NULL;
}

Block *ResourceLoader::getFileBlock(const Common::String &filename) const {
	const Lab *l = getLab(filename);
	Block *block = 0;
	if (l)
		block = l->getFileBlock(filename);
	return block;
}

LuaFile *ResourceLoader::openNewStreamLuaFile(const char *filename) const {
	const Lab *l = getLab(filename);
	LuaFile * file = 0;
	if (l)
		file = l->openNewStreamLua(filename);
	return file;
}

Common::File *ResourceLoader::openNewStreamFile(const char *filename) const {
	const Lab *l = getLab(filename);
	Common::File *file = 0;
	if (l)
		file = l->openNewStreamFile(filename);
	return file;
}

static Common::String fixFilename(const Common::String filename) {
	Common::String fname(filename);
	fname.toLowercase();
	if (g_grim->getGameType() == GType_MONKEY4) {
		int len = fname.size();
		for (int i = 0; i < len; i++) {
			if (fname[i] == '\\') {
				fname.setChar('/', i);
			}
		}
		// Append b to end of filename for EMI
		fname += "b";
	}
	return fname;
}

template<typename T>
T *ResourceLoader::loadResource(const Common::String &filename, void *additionalData, int errorLevel) {
	Common::String fname = fixFilename(filename);

	Block *b = getFileBlock(fname);
	if (!b) {
		if (errorLevel == 0)
			error("Could not find resource %s", fname.c_str());
		else if (errorLevel == 1)
			warning("Could not find resource %s", fname.c_str());
		return 0;
    }
	T *result = new T(fname, b->getData(), b->getLen(), additionalData);
	delete b;
	return result;
}

Bitmap *ResourceLoader::loadBitmap(const Common::String &filename) {
	Bitmap *result = loadResource<Bitmap>(filename, 0, 1);

	if (result)
		g_grim->registerBitmap(result);

	return result;
}

Costume *ResourceLoader::loadCostume(const Common::String &filename, Costume *prevCost) {
	return loadResource<Costume>(filename, prevCost);
}

template<typename T>
ObjectPtr<T> ResourceLoader::getResource(const Common::String &filename, void *additionalData, int errorLevel) {
	Common::String fname = filename;
	fname.toLowercase();

	if (additionalData) {
		// use the pointer to differentiate between resources with different colormaps
		fname += Common::tag2string((uint32)additionalData);
	}

	Object *p = _objectCache[fname];
	if (!p) {
		p = loadResource<T>(filename, additionalData, errorLevel);
		_objectCache[fname] = p;
	}

	return (T *)p;
}

ObjectPtr<Material> ResourceLoader::getMaterial(const Common::String &fname, CMap *c) {
	return getResource<Material>(fname, c);
}

ObjectPtr<Model> ResourceLoader::getModel(const Common::String &fname, CMap *c) {
	return getResource<Model>(fname, c);
}

ObjectPtr<CMap> ResourceLoader::getColormap(const Common::String &fname) {
	return getResource<CMap>(fname);
}

ObjectPtr<KeyframeAnim> ResourceLoader::getKeyframe(const Common::String &fname) {
	return getResource<KeyframeAnim>(fname);
}

ObjectPtr<Font> ResourceLoader::getFont(const Common::String &fname) {
	return getResource<Font>(fname);
}

ObjectPtr<LipSync> ResourceLoader::getLipSync(const Common::String &fname) {
	return getResource<LipSync>(fname, 0, 2);
}

} // end of namespace Grim
