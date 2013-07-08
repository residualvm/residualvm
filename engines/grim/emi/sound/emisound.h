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

#ifndef GRIM_MSS_H
#define GRIM_MSS_H

#include "common/str.h"
#include "common/stack.h"
#include "engines/grim/emi/sound/mp3track.h"
#include "engines/grim/emi/sound/scxtrack.h"

namespace Grim {

class SoundTrack;
	
struct MusicEntry {
	int _x;
	int _y;
	int _sync;
	int _trim;
	int _id;
	float _start;
	float _jumpFrom;
	float _jumpTo;
	Common::String _type;
	Common::String _name;
	Common::String _filename;
};

// Currently this class only implements the exact functions called in iMuse
// from Actor, to allow for splitting that into EMI-sound and iMuse without
// changing iMuse.
class EMISound {

protected:
	SoundTrack **_channels;
	MusicEntry *_musicTable;
	Common::String _musicPrefix;
	Common::String _currentSet;

	void removeItem(SoundTrack* item);
	int32 getFreeChannel();
	int32 getChannelByName(const Common::String &name);
	void freeChannel(int32 channel);

public:
	EMISound();
	virtual ~EMISound();
	bool startVoice(const char *soundName, int volume = 127, int pan = 64);
	bool getSoundStatus(const char *soundName);
	void stopSound(const char *soundName);
	int32 getPosIn16msTicks(const char *soundName);
	void setVolume(const char *soundName, int volume);
	void setPan(const char *soundName, int pan);

	virtual void setMusicState(int stateId) = 0;
	virtual void selectMusicSet(int setId) = 0;
	virtual void pushStateToStack() = 0;
	virtual void popStateFromStack() = 0;
	virtual void flushStack() = 0;
	virtual uint32 getMsPos(int stateId) = 0;
};

// Subclass used for PC/MAC versions of EMI
class EMISoundPC : public EMISound {
	MP3Track *_music;
	Common::Stack<MP3Track*> _stateStack;

	void initMusicTable();
	MusicEntry *initMusicTableRetail(Common::String &filename);
	MusicEntry *initMusicTableDemo(Common::String &filename);

public:
	EMISoundPC();
	~EMISoundPC();
	void setMusicState(int stateId);
	void selectMusicSet(int setId);
	void pushStateToStack();
	void popStateFromStack();
	void flushStack();
	uint32 getMsPos(int stateId);
};

// Subclass used for PS2 versions of EMI
class EMISoundPS2 : public EMISound {
	SCXTrack *_music;
	Common::Stack<SCXTrack*> _stateStack;
	void initMusicTable();

public:
	EMISoundPS2();
	~EMISoundPS2();
	void setMusicState(int stateId);
	void selectMusicSet(int setId);
	void pushStateToStack();
	void popStateFromStack();
	void flushStack();
	uint32 getMsPos(int stateId);
};

}

#endif
