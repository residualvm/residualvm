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

#include "common/debug.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "audio/audiostream.h"
#include "engines/grim/emi/sound/emiaudiostream.h"

namespace Grim {

SubLoopingRewindableAudioStream::SubLoopingRewindableAudioStream(Audio::SeekableAudioStream *stream, int loops, const Audio::Timestamp &loopStart, const Audio::Timestamp &loopEnd, DisposeAfterUse::Flag disposeAfterUse) :
	_stream(stream, disposeAfterUse), _loops(loops),
	_pos(0, getRate() * (isStereo() ? 2 : 1)),
	_loopStart(convertTimeToStreamPos(loopStart, getRate(), isStereo())),
	_loopEnd(convertTimeToStreamPos(loopEnd, getRate(), isStereo())),
	_done(false) {
	assert(loopStart < loopEnd);

	if (!_stream->rewind())
		_done = true;
}

bool SubLoopingRewindableAudioStream::seek(const Audio::Timestamp &where) {
	Audio::Timestamp newPos = convertTimeToStreamPos(where, getRate(), isStereo());
	if (_stream->seek(newPos)) {
		_pos = newPos;
		return true;
	}
	return false;
}


int SubLoopingRewindableAudioStream::readBuffer(int16 *buffer, const int numSamples) {
	if (_done)
		return 0;

	int framesLeft = MIN(_loopEnd.frameDiff(_pos), numSamples);
	int framesRead = _stream->readBuffer(buffer, framesLeft);
	_pos = _pos.addFrames(framesRead);

	if (framesRead < framesLeft && _stream->endOfData()) {
		// TODO: Proper error indication.
		_done = true;
		return framesRead;
	} else if (_pos == _loopEnd) {
		if (_loops > 0) {
			--_loops;
			if (_loops == 0) {
				_done = true;
				return framesRead;
			}
		}

		_stream->seek(_loopStart);
		
		// FIXME:
		// For some reason the first seek will always fail but the second won't
		// Something in the decoder is probably wrong
		if (!_stream->seek(_loopStart)) {
			warning("Seek failed");
			// TODO: Proper error indication.
			_done = true;
			return framesRead;
		}

		_pos = _loopStart;
		framesLeft = numSamples - framesLeft;
		return framesRead + readBuffer(buffer + framesRead, framesLeft);
	} else {
		return framesRead;
	}
}

} // End of namespace Grim
