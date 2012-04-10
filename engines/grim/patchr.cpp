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

#include "common/substream.h"
#include "common/md5.h"
#include "common/file.h"
#include "common/zlib.h"
#include "common/bufferedstream.h"

#include "engines/grim/patchr.h"
#include "engines/grim/debug.h"

namespace Grim {

class PatchedFile : public Common::SeekableReadStream {
public:
	PatchedFile();
	virtual ~PatchedFile();

	bool load(Common::SeekableReadStream *file, Common::String patchName);

	// Common::ReadStream implementation
	virtual bool eos() const;
	virtual uint32 read(void *dataPtr, uint32 dataSize);

	// Common::SeekableReadStream implementation
	virtual int32 pos() const;
	virtual int32 size() const;
	virtual bool seek(int32 offset, int whence = SEEK_SET);

private:
	// Consts
	const uint32 _kDiffBufferSize, _kHeaderSize, _kMd5size, _kVersion;

	// Streams
	Common::SeekableReadStream *_file;
	Common::SeekableReadStream *_ctrl, *_diff, *_extra;

	// Current instruction
	uint32 diffCopy, extraCopy;
	int32 jump;
	void readNextInst();

	int32 _pos;
	uint32 _newSize;

	uint8 *diffBuffer;
};

PatchedFile::PatchedFile():
	_kDiffBufferSize(1024), _kHeaderSize(44), _kMd5size(5000), _file(NULL),
	_ctrl(NULL), _diff(NULL), _extra(NULL), _kVersion(2), _pos(0) {
	diffBuffer = new uint8[_kDiffBufferSize];
}

PatchedFile::~PatchedFile() {
	if (diffBuffer)
		delete[] diffBuffer;

	if (_file)
		delete _file;

	if (_ctrl)
		delete _ctrl;
	if (_extra && _extra != _diff)
		delete _extra;
	if (_diff)
		delete _diff;
}

bool PatchedFile::load(Common::SeekableReadStream *file, Common::String patchName) {
	uint8 md5_p[16], md5_f[16];
	uint32 zctrllen, zdatalen, zextralen;
	Common::File patch;

	// Open the patch
	if (!patch.open(patchName)) {
		error("Unable to open patchfile %s", patchName.c_str());
		return false;
	}

	// Check for appropriate signature and version
	if (patch.readUint32BE() != MKTAG('P','A','T','R') || patch.readUint32LE() != _kVersion) {
		error("%s patchfile is corrupted or has a wrong version number", patchName.c_str());
		return false;
	}

	// Check if the file to patch match
	Common::computeStreamMD5(*file, md5_f, _kMd5size);
	file->seek(0, SEEK_SET);
	patch.read(md5_p, 16);
	if (memcmp(md5_p, md5_f, 16) != 0 || (uint32)file->size() != patch.readUint32LE()) {
		Debug::debug(Debug::Patchr,"%s targets a different file", patchName.c_str());
		return false;
	}

	// Read lengths from header
	_newSize = patch.readUint32LE();
	zctrllen = patch.readUint32LE();
	zdatalen = patch.readUint32LE();
	zextralen = patch.readUint32LE();

	patch.close();

	// Opens ctrl, diff and extra substreams
	Common::File *tmp;
	tmp = new Common::File;
	tmp->open(patchName);
	_ctrl = new Common::SeekableSubReadStream(tmp, _kHeaderSize, _kHeaderSize + zctrllen, DisposeAfterUse::YES);
	_ctrl = Common::wrapCompressedReadStream(_ctrl);

	tmp = new Common::File;
	tmp->open(patchName);
	_diff = new Common::SeekableSubReadStream(tmp, _kHeaderSize + zctrllen, _kHeaderSize + zctrllen + zdatalen, DisposeAfterUse::YES);
	_diff = Common::wrapCompressedReadStream(_diff);

	if (zextralen > 0) {
		tmp = new Common::File;
		tmp->open(patchName);
		_extra = new Common::SeekableSubReadStream(tmp, _kHeaderSize + zctrllen + zdatalen, _kHeaderSize + zctrllen + zdatalen + zextralen, DisposeAfterUse::YES);
		_extra = Common::wrapCompressedReadStream(_extra);
	} else
		_extra = _diff;

	_file = file;

	readNextInst();

	return true;
}

uint32 PatchedFile::read(void *dataPtr, uint32 dataSize) {
	uint32 readSize, diffRead, toRead;
	byte *data = (byte*)dataPtr;

	toRead = dataSize;
	while (toRead > 0 || _ctrl->eos()) {
		// Read data from original file and apply the differences
		if (diffCopy > 0) {
			readSize = MIN(toRead, diffCopy);
			_file->read(data, readSize);

			toRead -= readSize;
			diffCopy -= readSize;

			//Read data from diff as blocks of size _kDiffBufferSize,
			// then xor original data with them in groups of 4 or 8
			// bytes, depending on the architecture
			while (readSize > 0) {
				diffRead = MIN(readSize, _kDiffBufferSize);
				_diff->read(diffBuffer, diffRead);

				#ifdef SCUMM_64BITS
				for (uint32 i = 0; i < diffRead/8; ++i)
					*((uint64*)data + i) ^= *((uint64*)diffBuffer + i);
				for (uint32 i = diffRead - diffRead % 8; i < diffRead; ++i)
					data[i] ^= diffBuffer[i];
				#else
				for (uint32 i = 0; i < diffRead/4; ++i)
					*((uint32*)data + i) ^= *((uint32*)diffBuffer + i);
				for (uint32 i = diffRead - diffRead % 4; i < diffRead; ++i)
					data[i] ^= diffBuffer[i];
				#endif

				readSize -= diffRead;
				data += diffRead;
			}
		}

		if (toRead == 0)
			break;

		// Read data from extra
		if (extraCopy > 0) {
			readSize = MIN(toRead, extraCopy);
			_extra->read(data, readSize);

			data += readSize;
			toRead -= readSize;
			extraCopy -= readSize;
		}

		// Jump and read next instructions
		if (diffCopy == 0 && extraCopy == 0) {
			if (jump != 0)
				_file->seek(jump, SEEK_CUR);
			readNextInst();
		}
	}

	_pos += dataSize - toRead;
	return (dataSize - toRead);
}

void PatchedFile::readNextInst() {
	diffCopy = _ctrl->readUint32LE();
	extraCopy = _ctrl->readUint32LE();
	jump = _ctrl->readSint32LE();
}

bool PatchedFile::eos() const {
	if ( _pos >= (int32)_newSize)
		return true;
	else
		return false;
}

int32 PatchedFile::pos() const {
	return _pos;
}

int32 PatchedFile::size() const {
	return _newSize;
}

bool PatchedFile::seek(int32 offset, int whence) {
	int32 totJump, relOffset;
	uint32 skipDiff, skipExtra, skipSize;
	relOffset = 0;
	skipDiff = 0;
	skipExtra = 0;
	totJump = 0;

	switch (whence) {
		case SEEK_SET:
			relOffset = offset - pos();
			break;
		case SEEK_CUR:
			relOffset = offset;
			break;
		case SEEK_END:
			relOffset = (size() + offset) - pos();
		default:
			error("Invalid seek instruction!");
	}

	if (relOffset == 0)
		return true;
	if (relOffset < 0)
		error("Backward seeking isn't supported in PatchedFile");

	while (relOffset > 0) {
		if (diffCopy > 0) {
			skipSize = MIN(diffCopy, (uint32)relOffset);
			diffCopy -= skipSize;
			relOffset -= skipSize;
			skipDiff += skipSize;
			totJump += skipSize;
		}
		if (relOffset == 0)
			break;

		if (extraCopy > 0) {
			skipSize = MIN(extraCopy, (uint32)relOffset);
			extraCopy -= skipSize;
			relOffset -= skipSize;
			skipExtra += skipSize;
		}

		if (diffCopy == 0 && extraCopy == 0) {
			totJump += jump;
			readNextInst();
		}
	}
	_diff->seek(skipDiff, SEEK_CUR);
	_extra->seek(skipSize, SEEK_CUR);
	_file->seek(totJump, SEEK_CUR);

	return true;
}

Common::SeekableReadStream *wrapPatchedFile(Common::SeekableReadStream *rs, const Common::String &filename) {
	if (!rs)
		return NULL;

	Common::String patchfile = filename + ".patchr";
	int i = 1;
	while (SearchMan.hasFile(patchfile)) {
		Debug::debug(Debug::Patchr, "Patch requested for %s", filename.c_str());

		PatchedFile *pf = new PatchedFile;
		if (pf->load(rs, patchfile)) {
			rs = Common::wrapBufferedSeekableReadStream(pf, 1024, DisposeAfterUse::YES);
			Debug::debug(Debug::Patchr, "Patch for %s sucessfully loaded", filename.c_str());
			break;
		}

		delete pf;
		patchfile = Common::String::format("%s_%d.patchr", filename.c_str(), i++);
	}

	return rs;
}

} // end of namespace Grim
