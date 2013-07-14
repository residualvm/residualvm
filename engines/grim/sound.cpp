/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
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

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse.h"
#include "engines/grim/emi/sound/emisound.h"
#include "engines/grim/sound.h"

namespace Grim {

SoundPlayer *g_sound = NULL;

SoundPlayer::SoundPlayer() {
	// TODO: Replace this with g_emiSound when we get a full working sound-system for more than voices.
	if (g_grim->getGameType() == GType_MONKEY4)
		_emiSound = new EMISound();
	else
		_emiSound = NULL;
}

SoundPlayer::~SoundPlayer() {
	delete _emiSound;
}

bool SoundPlayer::startVoice(const char *soundName, int volume, int pan) {
	if (g_grim->getGameType() == GType_GRIM)
		return g_imuse->startVoice(soundName, volume, pan);
	else
		return _emiSound->startVoice(soundName, volume, pan);
}

bool SoundPlayer::getSoundStatus(const char *soundName) {
	if (g_grim->getGameType() == GType_GRIM)
		return g_imuse->getSoundStatus(soundName);
	else
		return _emiSound->getSoundStatus(soundName);
}

void SoundPlayer::stopSound(const char *soundName) {
	if (g_grim->getGameType() == GType_GRIM) {
		g_imuse->stopSound(soundName);
		return;
	} else {
		_emiSound->stopSound(soundName);
	}
}

int32 SoundPlayer::getPosIn16msTicks(const char *soundName) {
	if (g_grim->getGameType() == GType_GRIM)
		return g_imuse->getPosIn16msTicks(soundName);
	else
		return _emiSound->getPosIn16msTicks(soundName);
}

void SoundPlayer::setVolume(const char *soundName, int volume) {
	if (g_grim->getGameType() == GType_GRIM) {
		g_imuse->setVolume(soundName, volume);
	} else {
		_emiSound->setVolume(soundName, volume);
	}
}

void SoundPlayer::setPan(const char *soundName, int pan) {
	if (g_grim->getGameType() == GType_GRIM) {
		g_imuse->setPan(soundName, pan);
	} else {
		_emiSound->setPan(soundName, pan);
	}
}

void SoundPlayer::setMusicState(int stateId) {
	if (g_grim->getGameType() == GType_GRIM) {
		g_imuse->setMusicState(stateId);
	} else {
		_emiSound->setMusicState(stateId);
	}
}

void SoundPlayer::restoreState(SaveGame *savedState) {
	if (g_grim->getGameType() == GType_GRIM) {
		g_imuse->restoreState(savedState);
	} else {
		_emiSound->restoreState(savedState);
	}
}


void SoundPlayer::saveState(SaveGame *savedState) {
	if (g_grim->getGameType() == GType_GRIM) {
		g_imuse->saveState(savedState);
	} else {
		_emiSound->saveState(savedState);
	}
}
// EMI-only
uint32 SoundPlayer::getMsPos(int stateId) {
	assert(_emiSound); // This shouldn't ever be called from Grim.
	return _emiSound->getMsPos(stateId);
}

void SoundPlayer::selectMusicSet(int setId) {
	assert(_emiSound);
	return _emiSound->selectMusicSet(setId);
}

void SoundPlayer::pushState() {
	assert(_emiSound); // This shouldn't ever be called from Grim.
	return _emiSound->pushStateToStack();
}

void SoundPlayer::popState() {
	assert(_emiSound); // This shouldn't ever be called from Grim.
	return _emiSound->popStateFromStack();
}

void SoundPlayer::flushStack() {
	assert(_emiSound); // This shouldn't ever be called from Grim.
	return _emiSound->flushStack();
}

} // end of namespace Grim
