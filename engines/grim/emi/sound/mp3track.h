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

#ifndef GRIM_MP3TRACK_H
#define GRIM_MP3TRACK_H

#include "common/str.h"
#include "common/stream.h"
#include "engines/grim/emi/sound/track.h"
#include "engines/grim/emi/sound/emiaudiostream.h"

namespace Audio {
	class AudioStream;
	class SoundHandle;
}

namespace Grim {

class MP3Track : public SoundTrack {
	SubLoopingRewindableAudioStream *_stream;
	uint32 _headerSize;
	uint32 _regionLength;
	uint32 _freq;
	char _bits;
	char _channels;
	bool _endFlag;
	void parseRIFFHeader(Common::SeekableReadStream *data);
public:
	MP3Track(Audio::Mixer::SoundType soundType);
	~MP3Track();
	bool openSound(const Common::String &soundName, Common::SeekableReadStream *file);
	bool openSound(const Common::String &soundName, Common::SeekableReadStream *file, int playFrom, int loopStart, int loopEnd);
	bool play();
	void pause();
	Audio::Timestamp getPosition() {return _stream->getPosition();} 
	bool isPlaying() { return !_endFlag; }
};

}
#endif
