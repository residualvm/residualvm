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

#include "common/endian.h"

#include "graphics/pixelbuffer.h"

#include "graphics/agl/texture.h"
#include "graphics/agl/manager.h"

#include "engines/grim/grim.h"
#include "engines/grim/debug.h"
#include "engines/grim/material.h"
#include "engines/grim/colormap.h"
#include "engines/grim/resource.h"
#include "engines/grim/textsplit.h"

namespace Grim {

Common::List<MaterialData *> *MaterialData::_materials = NULL;

MaterialData::MaterialData(const Common::String &filename, Common::SeekableReadStream *data, CMap *cmap) :
	_fname(filename), _cmap(cmap), _refCount(1) {

	if (g_grim->getGameType() == GType_MONKEY4) {
		initEMI(data);
	} else {
		initGrim(data, cmap);
	}
}

void MaterialData::initGrim(Common::SeekableReadStream *data, CMap *cmap) {
	uint32 tag = data->readUint32BE();
	if (tag != MKTAG('M','A','T',' '))
		error("invalid magic loading texture");

	data->seek(12, SEEK_SET);
	_numImages = data->readUint32LE();
	_textures = new Texture[_numImages];
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
		Texture *t = _textures + i;
		t->_width = data->readUint32LE();
		t->_height = data->readUint32LE();
		t->_hasAlpha = data->readUint32LE();
		t->_texture = NULL;
		t->_data = NULL;
		if (t->_width == 0 || t->_height == 0) {
			Debug::warning(Debug::Materials, "skip load texture: bad texture size (%dx%d) for texture %d of material %s",
						t->_width, t->_height, i, _fname.c_str());
			break;
		}
		t->_data = new char[t->_width * t->_height];
		data->seek(12, SEEK_CUR);
		data->read(t->_data, t->_width * t->_height);

		char *tdata = t->_data;
		char *texdata = new char[t->_width * t->_height * 4];
		char *texdatapos = texdata;
		for (int y = 0; y < t->_height; y++) {
			for (int x = 0; x < t->_width; x++) {
				uint8 col = *(uint8 *)(tdata);
				if (col == 0) {
					memset(texdatapos, 0, 4); // transparent
					if (!t->_hasAlpha) {
						texdatapos[3] = '\xff'; // fully opaque
					}
				} else {
					memcpy(texdatapos, cmap->_colors + 3 * (col), 3);
					texdatapos[3] = '\xff'; // fully opaque
				}
				texdatapos += 4;
				tdata++;
			}
		}

		Graphics::PixelBuffer buf = Graphics::PixelBuffer::createBuffer<8888>((byte*)texdata);
		t->_tex = AGLMan.createTexture(buf, t->_width, t->_height);
	}
}

void loadTGA(Common::SeekableReadStream *data, Texture *t) {
	int descField = data->readByte();
	assert(descField == 0);	// Verify that description-field is empty

	data->seek(1, SEEK_CUR);

	int format = data->readByte();
	if (format != 2) { // We only support uncompressed TGA, but should also support atleast RLE-RGB
		error("Unsupported TGA-format detected: %d", format);
	}

	data->seek(9, SEEK_CUR);
	t->_width = data->readUint16LE();
	t->_height = data->readUint16LE();;
	t->_hasAlpha = false;
	t->_texture = NULL;

	Graphics::PixelFormat pf;

	int bpp = data->readByte();
	if (bpp == 32) {
		t->_bpp = 4;
		pf = Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
	} else {
		t->_bpp = 3;
		pf = Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0);
	}

	uint8 desc = data->readByte();
	uint8 flipped = !(desc & 32);

	assert(bpp == 24 || bpp == 32); // Assure we have 24/32 bpp
	t->_data = new char[t->_width * t->_height * (bpp / 8)];
	char *writePtr = t->_data + (t->_width * (t->_height - 1) * bpp / 8);

	// Since certain TGA's are flipped (relative to the tex-coords) and others not
	// We'll have to handle that here, otherwise we could just do 1.0f - texCoords
	// When drawing/loading
	if (flipped) {
		for (int i = 0; i < t->_height; i++) {
			data->read(writePtr, t->_width * (bpp / 8));
			writePtr -= (t->_width * bpp / 8);
		}
	} else {
		data->read(t->_data, t->_width * t->_height * (bpp / 8));
	}

	Graphics::PixelBuffer buf(pf, (byte *)t->_data);
	t->_tex = AGLMan.createTexture(buf, t->_width, t->_height);
}

void MaterialData::initEMI(Common::SeekableReadStream *data) {
	Common::Array<Common::String> texFileNames;
	char readFileName[64];

	if (_fname.hasSuffix(".sur")) {  // This expects that we want all the materials in the sur-file
		TextSplitter *ts = new TextSplitter(data);
		ts->setLineNumber(2); // Skip copyright-line
		ts->expectString("version\t1.0");
		if (ts->checkString("name:"))
			ts->scanString("name:\t%s", 1, readFileName);

		while(!ts->checkString("END_OF_SECTION")) {
			ts->scanString("tex:\t%s", 1, readFileName);
			Common::String mFileName(readFileName);
			texFileNames.push_back(mFileName);
		}
		Common::SeekableReadStream *texData;
		_textures = new Texture[texFileNames.size()];
		for (uint i = 0; i < texFileNames.size(); i++) {
			warning("SUR-file texture: %s", texFileNames[i].c_str());
			texData = g_resourceloader->openNewStreamFile(texFileNames[i].c_str(), true);
			if (!texData) {
				warning("Couldn't find tex-file: %s", texFileNames[i].c_str());
				_textures[i]._width = 0;
				_textures[i]._height = 0;
				_textures[i]._texture = new int(1); // HACK to avoid initializing.
				continue;
			}
			loadTGA(texData, _textures + i);
			delete texData;
		}
		_numImages = texFileNames.size();
		delete ts;
		return;
	} else if(_fname.hasSuffix(".tga")) {
		_numImages = 1;
		_textures = new Texture();
		loadTGA(data, _textures);
		//	texFileNames.push_back(filename);
		return;

	} else {
		warning("Unknown material-format: %s", _fname.c_str());
	}
}

MaterialData::~MaterialData() {
	_materials->remove(this);
	if (_materials->empty()) {
		delete _materials;
		_materials = NULL;
	}

	for (int i = 0; i < _numImages; ++i) {
		Texture *t = _textures + i;
		if (t->_width && t->_height && t->_texture)
			delete t->_tex;
		delete[] t->_data;
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

Material::Material(const Common::String &filename, Common::SeekableReadStream *data, CMap *cmap) :
		Object(), _currImage(0) {
	_data = MaterialData::getMaterialData(filename, data, cmap);
}

void Material::reload(CMap *cmap) {
	Common::String fname = _data->_fname;
	--_data->_refCount;
	if (_data->_refCount < 1) {
		delete _data;
	}

	Material *m = g_resourceloader->loadMaterial(fname, cmap);
	// Steal the data from the new material and discard it.
	_data = m->_data;
	++_data->_refCount;
	delete m;
}

void Material::select() const {
	Texture *t = _data->_textures + _currImage;
	if (t->_width && t->_height) {
		t->_tex->bind();
	}
}

Material::~Material() {
	--_data->_refCount;
	if (_data->_refCount < 1) {
		delete _data;
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

AGL::Texture *Material::getCurrentTexture() const {
	if (_currImage < 0 || _currImage >= _data->_numImages) {
		return NULL;
	}

	Texture *t = _data->_textures + _currImage;
	if (t->_width && t->_height) {
		return t->_tex;
	}
	return NULL;
}

const Common::String &Material::getFilename() const {
	return _data->_fname;
}

MaterialData *Material::getData() const {
	return _data;
}

} // end of namespace Grim
