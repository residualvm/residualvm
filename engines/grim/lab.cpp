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

#include "common/file.h"
#include "common/substream.h"

#include "engines/grim/grim.h"
#include "engines/grim/lab.h"

namespace Grim {

LabEntry::LabEntry()
	: _name(Common::String()), _offset(0), _len(0), _parent(NULL) {
}

LabEntry::LabEntry(Common::String name, uint32 offset, uint32 len, Lab *parent)
	: _offset(offset), _len(len), _parent(parent) {
	_name = name;
	_name.toLowercase();
}

Common::SeekableReadStream *LabEntry::createReadStream() const {
	return _parent->createReadStreamForMember(_name);
}

bool Lab::open(const Common::String &filename) {
	_labFileName = filename;

	close();

	_f = new Common::File();
	if (!_f->open(filename)) {
		close();
		return false;
	}

	if (_f->readUint32BE() != MKTAG('L','A','B','N')) {
		close();
		return false;
	}

	_f->readUint32LE(); // version

	if (g_grim->getGameType() == GType_GRIM)
		parseGrimFileTable();
	else
		parseMonkey4FileTable();

	delete _f;
	_f = NULL;

	return true;
}

void Lab::parseGrimFileTable() {
	uint32 entryCount = _f->readUint32LE();
	uint32 stringTableSize = _f->readUint32LE();

	char *stringTable = new char[stringTableSize];
	_f->seek(16 * (entryCount + 1));
	_f->read(stringTable, stringTableSize);
	_f->seek(16);

	for (uint32 i = 0; i < entryCount; i++) {
		int fnameOffset = _f->readUint32LE();
		int start = _f->readUint32LE();
		int size = _f->readUint32LE();
		_f->readUint32LE();

		Common::String fname = stringTable + fnameOffset;
		fname.toLowercase();

		LabEntry *entry = new LabEntry(fname, start, size, this);
		_entries[fname] = LabEntryPtr(entry);
	}

	delete[] stringTable;
}

void Lab::parseMonkey4FileTable() {
	uint32 entryCount = _f->readUint32LE();
	uint32 stringTableSize = _f->readUint32LE();
	uint32 stringTableOffset = _f->readUint32LE() - 0x13d0f;

	char *stringTable = new char[stringTableSize];
	_f->seek(stringTableOffset);
	_f->read(stringTable, stringTableSize);
	_f->seek(20);

	// Decrypt the string table
	for (uint32 i = 0; i < stringTableSize; i++)
		if (stringTable[i] != 0)
			stringTable[i] ^= 0x96;

	for (uint32 i = 0; i < entryCount; i++) {
		int fnameOffset = _f->readUint32LE();
		int start = _f->readUint32LE();
		int size = _f->readUint32LE();
		_f->readUint32LE();

		char *str = stringTable + fnameOffset;
		int len = strlen(str);

		for (int l = 0; l < len; ++l) {
			if (str[l] == '\\')
				str[l] = '/';
		}
		Common::String fname = str;
		fname.toLowercase();

		LabEntry *entry = new LabEntry(fname, start, size, this);
		_entries[fname] = LabEntryPtr(entry);
	}

	delete[] stringTable;
}

bool Lab::hasFile(const Common::String &filename) const {
	Common::String fname(filename);
	fname.toLowercase();
	return _entries.contains(fname);
}

int Lab::listMembers(Common::ArchiveMemberList &list) const {
	int count = 0;

	for (LabMap::const_iterator i = _entries.begin(); i != _entries.end(); ++i) {
		list.push_back(Common::ArchiveMemberList::value_type(i->_value));
		++count;
	}

	return count;
}

const Common::ArchiveMemberPtr Lab::getMember(const Common::String &name) const {
	if (!hasFile(name))
		return Common::ArchiveMemberPtr();

	Common::String fname(name);
	fname.toLowercase();
	return _entries[fname];
}

Common::SeekableReadStream *Lab::createReadStreamForMember(const Common::String &filename) const {
	if (!hasFile(filename))
		return 0;

	Common::String fname(filename);
	fname.toLowercase();
	LabEntryPtr i = _entries[fname];

	Common::File *file = new Common::File();
	file->open(_labFileName);
	return new Common::SeekableSubReadStream(file, i->_offset, i->_offset + i->_len, DisposeAfterUse::YES );
}

void Lab::close() {
	delete _f;
	_f = NULL;

	_entries.clear();
}

} // end of namespace Grim
