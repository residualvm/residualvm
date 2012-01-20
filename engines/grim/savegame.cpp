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
#include "common/system.h"

#include "math/vector3d.h"

#include "engines/grim/savegame.h"
#include "engines/grim/color.h"

namespace Grim {

#define SAVEGAME_HEADERTAG	'RSAV'
#define SAVEGAME_FOOTERTAG	'ESAV'

int SaveGame::SAVEGAME_VERSION = 20;

SaveGame *SaveGame::openForLoading(const Common::String &filename) {
	Common::InSaveFile *inSaveFile = g_system->getSavefileManager()->openForLoading(filename);
	if (!inSaveFile) {
		warning("SaveGame::openForLoading() Error opening savegame file %s", filename.c_str());
		return NULL;
	}

	SaveGame *save = new SaveGame();

	save->_saving = false;
	save->_inSaveFile = inSaveFile;

	uint32 tag = inSaveFile->readUint32BE();
	if (tag != SAVEGAME_HEADERTAG) {
		delete save;
		return NULL;
	}
	save->_version = inSaveFile->readUint32BE();

	return save;
}

SaveGame *SaveGame::openForSaving(const Common::String &filename) {
	Common::OutSaveFile *outSaveFile =  g_system->getSavefileManager()->openForSaving(filename);
	if (!outSaveFile) {
		warning("SaveGame::openForSaving() Error creating savegame file %s", filename.c_str());
		return NULL;
	}

	SaveGame *save = new SaveGame();

	save->_saving = true;
	save->_outSaveFile = outSaveFile;

	outSaveFile->writeUint32BE(SAVEGAME_HEADERTAG);
	outSaveFile->writeUint32BE(SAVEGAME_VERSION);

	save->_version = SAVEGAME_VERSION;

	return save;
}

SaveGame::SaveGame() :
	_currentSection(0), _sectionBuffer(0) {

}

SaveGame::~SaveGame() {
	if (_saving) {
		_outSaveFile->writeUint32BE(SAVEGAME_FOOTERTAG);
		_outSaveFile->finalize();
		if (_outSaveFile->err())
			warning("SaveGame::~SaveGame() Can't write file. (Disk full?)");
		delete _outSaveFile;
	} else {
		delete _inSaveFile;
	}
	free(_sectionBuffer);
}

int SaveGame::saveVersion() const {
	return _version;
}

uint32 SaveGame::beginSection(uint32 sectionTag) {
	assert(_version == SAVEGAME_VERSION);

	if (_currentSection != 0)
		error("Tried to begin a new save game section with ending old section");
	_currentSection = sectionTag;
	_sectionSize = 0;
	if (!_saving) {
		uint32 tag = 0;

		while (tag != sectionTag) {
			tag = _inSaveFile->readUint32BE();
			if (tag == SAVEGAME_FOOTERTAG)
				error("Unable to find requested section of savegame");
			_sectionSize = _inSaveFile->readUint32BE();
			_inSaveFile->seek(_sectionSize, SEEK_CUR);
		}
		if (!_sectionBuffer || _sectionAlloc < _sectionSize) {
			_sectionAlloc = _sectionSize;
			_sectionBuffer = (byte *)realloc(_sectionBuffer, _sectionAlloc);
		}

		_inSaveFile->seek(-(int32)_sectionSize, SEEK_CUR);
		_inSaveFile->read(_sectionBuffer, _sectionSize);

	} else {
		if (!_sectionBuffer) {
			_sectionAlloc = _allocAmmount;
			_sectionBuffer = (byte *)malloc(_sectionAlloc);
		}
	}
	_sectionPtr = 0;
	return _sectionSize;
}

void SaveGame::endSection() {
	if (_currentSection == 0)
		error("Tried to end a save game section without starting a section");
	if (_saving) {
		_outSaveFile->writeUint32BE(_currentSection);
		_outSaveFile->writeUint32BE(_sectionSize);
		_outSaveFile->write(_sectionBuffer, _sectionSize);
	}
	_currentSection = 0;
}

uint32 SaveGame::getBufferPos() {
	if (_saving)
		return _sectionSize;
	else
		return _sectionPtr;
}

void SaveGame::read(void *data, int size) {
	if (_saving)
		error("SaveGame::readBlock called when storing a savegame");
	if (_currentSection == 0)
		error("Tried to read a block without starting a section");
	memcpy(data, &_sectionBuffer[_sectionPtr], size);
	_sectionPtr += size;
}

uint32 SaveGame::readLEUint32() {
	if (_saving)
		error("SaveGame::readBlock called when storing a savegame");
	if (_currentSection == 0)
		error("Tried to read a block without starting a section");
	uint32 data = READ_LE_UINT32(&_sectionBuffer[_sectionPtr]);
	_sectionPtr += 4;
	return data;
}

uint16 SaveGame::readLEUint16() {
	if (_saving)
		error("SaveGame::readBlock called when storing a savegame");
	if (_currentSection == 0)
		error("Tried to read a block without starting a section");
	uint16 data = READ_LE_UINT16(&_sectionBuffer[_sectionPtr]);
	_sectionPtr += 2;
	return data;
}

int32 SaveGame::readLESint32() {
	if (_saving)
		error("SaveGame::readBlock called when storing a savegame");
	if (_currentSection == 0)
		error("Tried to read a block without starting a section");
	int32 data = (int32)READ_LE_UINT32(&_sectionBuffer[_sectionPtr]);
	_sectionPtr += 4;
	return data;
}

byte SaveGame::readByte() {
	if (_saving)
		error("SaveGame::readBlock called when storing a savegame");
	if (_currentSection == 0)
		error("Tried to read a block without starting a section");
	byte data = _sectionBuffer[_sectionPtr];
	_sectionPtr++;
	return data;
}

bool SaveGame::readLEBool() {
	if (_saving)
		error("SaveGame::readBlock called when storing a savegame");
	if (_currentSection == 0)
		error("Tried to read a block without starting a section");
	uint32 data = READ_LE_UINT32(&_sectionBuffer[_sectionPtr]);
	_sectionPtr += 4;
	return data != 0;
}

void SaveGame::checkAlloc(int size) {
	if (_sectionSize + size > _sectionAlloc) {
		while (_sectionSize + size > _sectionAlloc)
			_sectionAlloc += _allocAmmount;
		_sectionBuffer = (byte *)realloc(_sectionBuffer, _sectionAlloc);
		if (!_sectionBuffer)
			error("Failed to allocate space for buffer");
	}
}

void SaveGame::write(const void *data, int size) {
	if (!_saving)
		error("SaveGame::writeBlock called when restoring a savegame");
	if (_currentSection == 0)
		error("Tried to write a block without starting a section");

	checkAlloc(size);

	memcpy(&_sectionBuffer[_sectionSize], data, size);
	_sectionSize += size;
}

void SaveGame::writeLEUint32(uint32 data) {
	if (!_saving)
		error("SaveGame::writeBlock called when restoring a savegame");
	if (_currentSection == 0)
		error("Tried to write a block without starting a section");

	checkAlloc(4);

	WRITE_LE_UINT32(&_sectionBuffer[_sectionSize], data);
	_sectionSize += 4;
}

void SaveGame::writeLEUint16(uint16 data) {
	if (!_saving)
		error("SaveGame::writeBlock called when restoring a savegame");
	if (_currentSection == 0)
		error("Tried to write a block without starting a section");

	checkAlloc(2);

	WRITE_LE_UINT16(&_sectionBuffer[_sectionSize], data);
	_sectionSize += 2;
}

void SaveGame::writeLESint32(int32 data) {
	if (!_saving)
		error("SaveGame::writeBlock called when restoring a savegame");
	if (_currentSection == 0)
		error("Tried to write a block without starting a section");

	checkAlloc(4);

	WRITE_LE_UINT32(&_sectionBuffer[_sectionSize], (uint32)data);
	_sectionSize += 4;
}

void SaveGame::writeLEBool(bool data) {
	if (!_saving)
		error("SaveGame::writeBlock called when restoring a savegame");
	if (_currentSection == 0)
		error("Tried to write a block without starting a section");

	checkAlloc(4);

	WRITE_LE_UINT32(&_sectionBuffer[_sectionSize], (uint32)data);
	_sectionSize += 4;
}

void SaveGame::writeByte(byte data) {
	if (!_saving)
		error("SaveGame::writeBlock called when restoring a savegame");
	if (_currentSection == 0)
		error("Tried to write a block without starting a section");

	checkAlloc(1);

	_sectionBuffer[_sectionSize] = data;
	_sectionSize++;
}

void SaveGame::writeVector3d(const Math::Vector3d &vec) {
	writeFloat(vec.x());
	writeFloat(vec.y());
	writeFloat(vec.z());
}

void SaveGame::writeColor(const Grim::Color &color) {
	writeByte(color.getRed());
	writeByte(color.getGreen());
	writeByte(color.getBlue());
}

void SaveGame::writeFloat(float data) {
	uint32 v;
	memcpy(&v, &data, 4);
	writeLEUint32(v);
}

void SaveGame::writeString(const Common::String &string) {
	int32 len = string.size();
	writeLESint32(len);
	write(string.c_str(), len);
}

Math::Vector3d SaveGame::readVector3d() {
	float x = readFloat();
	float y = readFloat();
	float z = readFloat();
	return Math::Vector3d(x, y, z);
}

Grim::Color SaveGame::readColor() {
	Color color;
	color.getRed() = readByte();
	color.getGreen() = readByte();
	color.getBlue() = readByte();

	return color;
}

float SaveGame::readFloat() {
	float f;
	uint32 v = readLEUint32();
	memcpy(&f, &v, 4);

	return f;
}

Common::String SaveGame::readString() {
	int32 len = readLESint32();
	Common::String s((const char *)&_sectionBuffer[_sectionPtr], len);
	_sectionPtr += len;
	return s;
}

} // end of namespace Grim
