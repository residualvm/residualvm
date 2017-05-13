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

#include "common/endian.h"
#include "image/tga.h"
#include "graphics/pixelbuffer.h"
#include "graphics/surface.h"

#include "engines/grim/grim.h"
#include "engines/grim/debug.h"
#include "engines/grim/material.h"
#include "engines/grim/gfx_base.h"
#include "engines/grim/colormap.h"
#include "engines/grim/resource.h"
#include "engines/grim/textsplit.h"

namespace Grim {

Common::List<MaterialData *> *MaterialData::_materials = nullptr;

MaterialData::MaterialData(const Common::String &filename, Common::SeekableReadStream *data, CMap *cmap) :
		_fname(filename), _cmap(cmap), _refCount(1), _textures(nullptr) {

	if (g_grim->getGameType() == GType_MONKEY4) {
		initEMI(data);
	} else {
		initGrim(data);
	}
}

void MaterialData::initGrim(Common::SeekableReadStream *data) {
	uint32 tag = data->readUint32BE();
	if (tag != MKTAG('M','A','T',' '))
		error("Invalid header for texture %s. Expected 'MAT ', got '%c%c%c%c'", _fname.c_str(),
		                 (tag >> 24) & 0xFF, (tag >> 16) & 0xFF, (tag >> 8) & 0xFF, tag & 0xFF);
	assert(_cmap);

	Graphics::PixelFormat pf(4, 8, 8, 8, 8, 0, 8, 16, 24);
	data->seek(12, SEEK_SET);
	_numImages = data->readUint32LE();
	_textures = new Texture*[_numImages];
	/* Discovered by diffing orange.mat with pink.mat and blue.mat .
	 * Actual meaning unknown, so I prefer to use it as an enum-ish
	 * at the moment, to detect unexpected values.
	 */
	data->seek(0x4c, SEEK_SET);
	uint32 offset = data->readUint32LE();
	if (offset == 0x8)
		offset = 16;
	else if (offset != 0)
		error("Unknown offset: %d", offset);

	data->seek(60 + _numImages * 40 + offset, SEEK_SET);
	for (int i = 0; i < _numImages; ++i) {
		uint32 width, height;
		Graphics::PixelBuffer buf;
		unsigned int pixel_count;
		bool hasAlpha;
		Texture *t;

		width = (uint16) data->readUint32LE();
		height = (uint16) data->readUint32LE();
		if (width == 0 || height == 0 ||
		    width >= (1 << 16) || height >= (1 << 16)
		) {
			Debug::warning(Debug::Materials, "skip load texture: bad texture size (%dx%d) for texture %d of material %s",
						   width, height, i, _fname.c_str());
			for (; i < _numImages; i++)
				_textures[i] = nullptr;
			break;
		}
		hasAlpha = data->readUint32LE();
		data->seek(12, SEEK_CUR);
		pixel_count = width * height;
		t = new Texture();
		t->create(width, height, pf);
		t->_hasAlpha = hasAlpha;
		buf = Graphics::PixelBuffer(*t);
		for (unsigned int pixel = 0; pixel < pixel_count; pixel++) {
			uint8 a, r, g, b;
			uint8 color = data->readByte();
			if (color == 0) {
				if (hasAlpha) {
					a = 0x00; // transparent
				} else {
					a = 0xff; // fully opaque
				}
				r = g = b = 0;
			} else {
				const uint8 *cmap_color = (const uint8 *) _cmap->_colors + color * 3;
				r = cmap_color[0];
				g = cmap_color[1];
				b = cmap_color[2];
				a = 0xff; // fully opaque
			}
			buf.setPixelAt(pixel, a, r, g, b);
		}
		_textures[i] = t;
	}
}

void loadTGA(Common::SeekableReadStream *data, Texture *t) {
	Image::TGADecoder *tgaDecoder = new Image::TGADecoder();
	tgaDecoder->loadStream(*data);
	const Graphics::Surface *tgaSurface;
	Graphics::Surface *convertedTgaSurface;
	tgaSurface = tgaDecoder->getSurface();
	t->_hasAlpha = tgaSurface->format.aBits();
	convertedTgaSurface = tgaSurface->convertTo(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
	t->copyFrom(*convertedTgaSurface);
	convertedTgaSurface->free();
	delete convertedTgaSurface;
	delete tgaDecoder;
}

void MaterialData::initEMI(Common::SeekableReadStream *data) {

	if (_fname.hasSuffix(".sur")) {  // This expects that we want all the materials in the sur-file
		Common::Array<Common::String> texFileNames;
		char readFileName[64];
		TextSplitter *ts = new TextSplitter(_fname, data);
		ts->setLineNumber(2); // Skip copyright-line
		ts->expectString("version\t1.0");
		if (ts->checkString("name:"))
			ts->scanString("name:%s", 1, readFileName);

		while (!ts->checkString("END_OF_SECTION")) {
			ts->scanString("tex:%s", 1, readFileName);
			Common::String mFileName(readFileName);
			texFileNames.push_back(ResourceLoader::fixFilename(mFileName, false));
		}
		_textures = new Texture*[texFileNames.size()];
		for (uint i = 0; i < texFileNames.size(); i++) {
			Common::String name = texFileNames[i];
			if (name.hasPrefix("specialty")) {
				_textures[i] = g_driver->getSpecialtyTexturePtr(name);
			} else {
				_textures[i] = new Texture();
				Common::SeekableReadStream *texData = g_resourceloader->openNewStreamFile(texFileNames[i].c_str(), true);
				if (!texData) {
					warning("Couldn't find tex-file: %s", texFileNames[i].c_str());
					_textures[i]->_texture = new int(1); // HACK to avoid initializing.
					continue;
				}
				loadTGA(texData, _textures[i]);
				delete texData;
			}
		}
		_numImages = texFileNames.size();
		delete ts;
		return;
	} else if (_fname.hasSuffix(".tga")) {
		_numImages = 1;
		_textures = new Texture*[1];
		_textures[0] = new Texture();
		loadTGA(data, _textures[0]);
		return;
	} else if (_fname.hasPrefix("specialty")) {
		_numImages = 1;
		_textures = new Texture*[1];
		_textures[0] = g_driver->getSpecialtyTexturePtr(_fname);
	} else {
		warning("Unknown material-format: %s", _fname.c_str());
	}
}

MaterialData::~MaterialData() {
	_materials->remove(this);
	if (_materials->empty()) {
		delete _materials;
		_materials = nullptr;
	}

	for (int i = 0; i < _numImages; ++i) {
		Texture *t = _textures[i];
		if (!t) continue;
		if (t->_isShared) continue; // don't delete specialty textures
		if (t->_texture)
			g_driver->destroyTexture(t);
		t->free();
		delete t;
	}
	delete[] _textures;
}

MaterialData *MaterialData::getMaterialData(const Common::String &filename, Common::SeekableReadStream *data, CMap *cmap) {
	if (!_materials) {
		_materials = new Common::List<MaterialData *>();
	}

	for (Common::List<MaterialData *>::iterator i = _materials->begin(); i != _materials->end(); ++i) {
		MaterialData *m = *i;
		if (m->_fname == filename && g_grim->getGameType() == GType_MONKEY4) {
			++m->_refCount;
			return m;
		}
		if (m->_fname == filename && m->_cmap->getFilename() == cmap->getFilename()) {
			++m->_refCount;
			return m;
		}
	}

	MaterialData *m = new MaterialData(filename, data, cmap);
	_materials->push_back(m);
	return m;
}

Material::Material(const Common::String &filename, Common::SeekableReadStream *data, CMap *cmap, bool clamp) :
		Object(), _currImage(0) {
	_data = MaterialData::getMaterialData(filename, data, cmap);
	_clampTexture = clamp;
}

Material::Material() :
		Object(), _currImage(0), _data(nullptr), _clampTexture(false) {
}

void Material::reload(CMap *cmap) {
	Common::String fname = _data->_fname;
	--_data->_refCount;
	if (_data->_refCount < 1) {
		delete _data;
	}

	Material *m = g_resourceloader->loadMaterial(fname, cmap, _clampTexture);
	// Steal the data from the new material and discard it.
	_data = m->_data;
	++_data->_refCount;
	delete m;
}

void Material::select() const {
	Texture *t = _data->_textures[_currImage];
	if (t) {
		if (!t->_texture) {
			uint16 w = t->w, h = t->h;
			g_driver->createTexture(t, _data->_cmap, _clampTexture);
			t->free();
			// free() zeroes wisth and height, but we need these when selecting the texture.
			t->w = w;
			t->h = h;
		}
		g_driver->selectTexture(t);
	} else {
		warning("Can't select material: %s", getFilename().c_str());
	}
}

Material::~Material() {
	if (_data) {
		--_data->_refCount;
		if (_data->_refCount < 1) {
			delete _data;
		}
	}
}

void Material::setActiveTexture(int n) {
	_currImage = n;
}

int Material::getNumTextures() const {
	return _data->_numImages;
}

int Material::getActiveTexture() const {
	return _currImage;
}

const Common::String &Material::getFilename() const {
	return _data->_fname;
}

MaterialData *Material::getData() const {
	return _data;
}

} // end of namespace Grim
