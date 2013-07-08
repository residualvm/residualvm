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

#include "common/mutex.h"
#include "common/textconsole.h"
#include "audio/mixer.h"
#include "audio/audiostream.h"
#include "audio/decoders/mp3.h"
#include "engines/grim/resource.h"
#include "engines/grim/emi/sound/mp3track.h"
#include "engines/grim/emi/sound/emiaudiostream.h"

namespace Grim {

void MP3Track::parseRIFFHeader(Common::SeekableReadStream *data) {
	uint32 tag = data->readUint32BE();
	if (tag == MKTAG('R','I','F','F')) {
		_endFlag = false;
		data->seek(18, SEEK_CUR);
		_channels = data->readByte();
		data->readByte();
		_freq = data->readUint32LE();
		data->seek(6, SEEK_CUR);
		_bits = data->readByte();
		data->seek(5, SEEK_CUR);
		_regionLength = data->readUint32LE();
		_headerSize = 44;
	} else {
		error("Unknown file header");
	}
}

MP3Track::MP3Track(Audio::Mixer::SoundType soundType) {
	_soundType = soundType;
	_headerSize = 0;
	_regionLength = 0;
	_freq = 0;
	_bits = 0,
	_channels = 0;
	_endFlag = false;
}

MP3Track::~MP3Track() {
	stop();
	delete _handle;
}

bool MP3Track::openSound(const Common::String &soundName, Common::SeekableReadStream *file) {
#ifndef USE_MAD
	return false;
#else
	if (!file) {
		warning("Stream for %s not open", soundName.c_str());
		return false;
	}
	_soundName = soundName;
	parseRIFFHeader(file);
	const Audio::Timestamp loopStart = 0;
	Audio::SeekableAudioStream *stream = Audio::makeMP3Stream(file, DisposeAfterUse::YES);
	const Audio::Timestamp loopEnd = stream->getLength();
	_stream = new SubLoopingRewindableAudioStream(stream, -1, loopStart, loopEnd);
	_handle = new Audio::SoundHandle();
	return true;
#endif
}

bool MP3Track::openSound(const Common::String &soundName, Common::SeekableReadStream *file, int playFrom, int loopStart, int loopEnd) {
#ifndef USE_MAD
	return false;
#else
	if (!file) {
		warning("Stream for %s not open", soundName.c_str());
		return false;
	}
	_soundName = soundName;
	parseRIFFHeader(file);
	Audio::SeekableAudioStream *stream = Audio::makeMP3Stream(file, DisposeAfterUse::YES);
	_stream = new SubLoopingRewindableAudioStream(stream, -1, loopStart, loopEnd);
	bool seekOK = _stream->seek(playFrom);
	_handle = new Audio::SoundHandle();
	return true;
#endif
}

bool MP3Track::play() {
	if (_stream) {
		// If _disposeAfterPlaying is NO, the destructor will take care of the stream.
		g_system->getMixer()->playStream(_soundType, _handle, _stream, -1, Audio::Mixer::kMaxChannelVolume, 0, _disposeAfterPlaying);
		return true;
	}
	return false;
}

void MP3Track::pause() {
	if (_stream) {
		_paused = !_paused;
		g_system->getMixer()->pauseHandle(*_handle, _paused);
	}
}


} // end of namespace Grim
