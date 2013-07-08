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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef GRIM_EMIAUDIOSTREAM_H
#define GRIM_EMIAUDIOSTREAM_H

#include "common/ptr.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "common/types.h"

#include "audio/timestamp.h"
#include "audio/audiostream.h"

namespace Grim {

class SubLoopingRewindableAudioStream : public Audio::AudioStream {
public:
	/**
	 * Constructor for a SubLoopingRewindableAudioStream.
	 *
	 *
	 * @param stream          Stream to loop
	 * @param loops           How often the stream should be looped (-1 means infinite)
	 * @param loopStart       Start of the loop (this must be smaller than loopEnd)
	 * @param loopEnd         End of the loop (thus must be greater than loopStart)
	 * @param disposeAfterUse Whether the stream should be disposed, when the
	 *                        SubLoopingAudioStream is destroyed.
	 */
	SubLoopingRewindableAudioStream(Audio::SeekableAudioStream *stream, int loops, const Audio::Timestamp &loopStart, const Audio::Timestamp &loopEnd, DisposeAfterUse::Flag disposeAfterUse = DisposeAfterUse::YES);

	int readBuffer(int16 *buffer, const int numSamples);
	bool endOfData() const { return _done; }
	bool isStereo() const { return _stream->isStereo(); }
	int getRate() const { return _stream->getRate(); }

	/**
	 * Seeks to a given offset in the stream.
	 *
	 * @param where offset in milliseconds
	 * @return true on success, false on failure.
	 */
	bool seek(uint32 where) { return seek(Audio::Timestamp(where, getRate())); }
	bool seek(const Audio::Timestamp &where);
	virtual bool rewind() { return seek(0); }
	Audio::Timestamp getPosition() const { return _pos; }

private:
	Common::DisposablePtr<Audio::SeekableAudioStream> _stream;

	int _loops;
	Audio::Timestamp _pos;
	Audio::Timestamp _loopStart, _loopEnd;

	bool _done;
};

}
#endif
