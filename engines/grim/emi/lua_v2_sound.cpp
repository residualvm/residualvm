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

#include "audio/mixer.h"
#include "audio/audiostream.h"
#include "common/system.h"
#include "engines/grim/set.h"

#include "engines/grim/emi/sound/aifftrack.h"
#include "engines/grim/emi/sound/scxtrack.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/poolsound.h"
#include "engines/grim/lua/lua.h"

#include "engines/grim/debug.h"
#include "engines/grim/sound.h"
#include "engines/grim/grim.h"
#include "engines/grim/resource.h"
#include "audio/decoders/aiff.h"

namespace Grim {

void Lua_V2::ImGetMillisecondPosition() {
	lua_Object soundObj = lua_getparam(1);

	if (lua_isnumber(soundObj)) {
		int sound = (int)lua_getnumber(soundObj);
		// FIXME int ms = func(sound);
		// lua_pushnumber(ms);
		// push -1 for now
		// Currently a bit of guesswork, and probably wrong, as the stateId
		// is ignored by emisound (which only has one music-track now).
		uint32 msPos = g_sound->getMsPos(sound);
		Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImGetMillisecondPosition: sound: %d ms: %d", sound, msPos);
		lua_pushnumber(msPos);
	}
}

void Lua_V2::SetReverb() {
	lua_Object eaxObj = lua_getparam(1);
	lua_Object decayObj = lua_getparam(2);
	lua_Object mixObj = lua_getparam(3);
	lua_Object predelayObj = lua_getparam(4);
	lua_Object dampingObj = lua_getparam(5);

	if (!lua_isnumber(eaxObj))
		return;

	int eax = (int)lua_getnumber(eaxObj);
	int param = 0;
	float decay = -1;
	float mix = -1;
	float predelay = -1;
	float damping = -1;

	if (eax == 60) {
		param = 26;
	} else if (eax == 70) {
		param = 27;
	} else if (eax >= 0 && eax <= 25) {
		param = eax;
		// there is some table, initialy is like eax
	} else {
		return;
	}

	if (lua_isnumber(decayObj))
		decay = lua_getnumber(decayObj);
	if (lua_isnumber(mixObj))
		mix = lua_getnumber(mixObj);
	if (lua_isnumber(predelayObj))
		predelay = lua_getnumber(predelayObj);
	if (lua_isnumber(dampingObj))
		damping = lua_getnumber(dampingObj);

	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::SetReverb, eax: %d, decay: %f, mix: %f, predelay: %f, damping: %f", param, decay, mix, predelay, damping);
	// FIXME: func(param, decay, mix, predelay, damping);
}

void Lua_V2::ImSetState() {
	lua_Object stateObj = lua_getparam(1);
	if (!lua_isnumber(stateObj))
		return;

	int state = (int)lua_getnumber(stateObj);
	g_imuseState = state;
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImSetState: stub, state: %d", state);
}

void Lua_V2::ImStateHasEnded() {
	lua_Object stateObj = lua_getparam(1);
	if (!lua_isnumber(stateObj))
		return;

	int state = (int)lua_getnumber(stateObj);

	// FIXME: Make sure this logic is correct.
	pushbool(g_imuseState != state);

	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImStateHasEnded: state %d.", state);
}

// TODO: Implement this:
void Lua_V2::ImStateHasLooped() {
	lua_Object stateObj = lua_getparam(1);
	if (!lua_isnumber(stateObj))
		return;

	int state = (int)lua_getnumber(stateObj);

	pushbool(g_sound->stateHasLooped(state));
}

void Lua_V2::EnableVoiceFX() {
	lua_Object stateObj = lua_getparam(1);

	bool state = false;
	if (!lua_isnil(stateObj))
		state = true;

	// FIXME: func(state);
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::EnableVoiceFX: implement opcode, state: %d", (int)state);
}

void Lua_V2::SetGroupVolume() {
	lua_Object groupObj = lua_getparam(1);
	lua_Object volumeObj = lua_getparam(2);

	if (!lua_isnumber(groupObj))
		return;
	int group = (int)lua_getnumber(groupObj);

	int volume = 100;
	if (lua_isnumber(volumeObj))
		volume = (int)lua_getnumber(volumeObj);

	volume = (volume * Audio::Mixer::kMaxMixerVolume) / 100;

	switch (group) {
		case 1: // SFX
			g_system->getMixer()->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, volume);
			break;
		case 2: // Voice
			g_system->getMixer()->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, volume);
			break;
		case 3: // Music
			g_system->getMixer()->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, volume);
			break;
		default:
			error("Lua_V2::SetGroupVolume - unknown group %d", group);
	}
	// FIXME: func(group, volume);
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::SetGroupVolume: group: %d, volume %d", group, volume);
}

void Lua_V2::EnableAudioGroup() {
	lua_Object groupObj = lua_getparam(1);
	lua_Object stateObj = lua_getparam(2);

	if (!lua_isnumber(groupObj))
		return;
	int group = (int)lua_getnumber(groupObj);

	bool state = false;
	if (!lua_isnil(stateObj))
		state = true;

	// FIXME: func(group, state);
	switch (group) {
		case 1: // SFX
			g_system->getMixer()->muteSoundType(Audio::Mixer::kSFXSoundType, !state);
			break;
		case 2: // Voice
			g_system->getMixer()->muteSoundType(Audio::Mixer::kSpeechSoundType, !state);
			break;
		case 3: // Music
			g_system->getMixer()->muteSoundType(Audio::Mixer::kMusicSoundType, !state);
			break;
		default:
			error("Lua_V2::EnableAudioGroup - unknown group %d", group);
	}

	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::EnableAudioGroup: group: %d, state %d", group, (int)state);
}

void Lua_V2::ImSelectSet() {
	lua_Object qualityObj = lua_getparam(1);

	if (lua_isnumber(qualityObj)) {
		int quality = (int)lua_getnumber(qualityObj);
		// FIXME: func(quality);
		g_sound->selectMusicSet(quality);
		Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImSelectSet: quality mode: %d", quality);
	}
}

void Lua_V2::ImFlushStack() {
	// FIXME
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImFlushStack: currently guesswork");
	g_sound->flushStack();
}

static Common::String addSoundSuffix(const char *fname) {
	Common::String filename = fname;
	if (g_grim->getGameFlags() != ADGF_DEMO) {
		if (g_grim->getGamePlatform() == Common::kPlatformPS2) {
			filename += ".scx";
		} else {
			if (!filename.hasSuffix(".aif") && !filename.hasSuffix(".AIF")) {
				filename += ".aif";
			}
		}
	}
	return filename;
}


void Lua_V2::LoadSound() {
	lua_Object strObj = lua_getparam(1);

	if (!lua_isstring(strObj))
		return;

	const char *str = lua_getstring(strObj);

	Common::String filename = addSoundSuffix(str);

	PoolSound *sound = new PoolSound(filename);
	lua_pushusertag(sound->getId(), MKTAG('A', 'I', 'F', 'F'));
}

void Lua_V2::FreeSound() {
	lua_Object idObj = lua_getparam(1);
	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F'))
		return;
	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	delete sound;
}

void Lua_V2::PlayLoadedSound() {
	lua_Object idObj = lua_getparam(1);
	lua_Object loopingObj = lua_getparam(2);
	lua_Object volumeObj = lua_getparam(3);
	/* FIXME: unknown parameter */
	/*lua_Object bool2Obj =*/ lua_getparam(4);

	if (!lua_isnumber(volumeObj)) {
		error("Lua_V2::PlayLoadedSound - ERROR: Unknown parameters");
		return;
	}

	int volume = (int)lua_getnumber(volumeObj);

	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F')) {
		error("Lua_V2::PlayLoadedSound - ERROR: Unknown parameters");
		return;
	}

	bool looping = !lua_isnil(loopingObj);

	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	if (!sound) {
		warning("Lua_V2::PlayLoadedSound: can't find requested sound object");
		return;
	}
	sound->setVolume(volume);
	sound->play(looping);
}

void Lua_V2::PlayLoadedSoundFrom() {
	lua_Object idObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object volumeOrLoopingObj = lua_getparam(5);
	lua_Object volumeObj = lua_getparam(6);

	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F')) {
		error("Lua_V2::PlayLoadedSoundFrom - ERROR: Unknown parameters");
		return;
	}

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj) ||
	    !lua_isnumber(volumeObj)) {
		error("Lua_V2::PlayLoadedSoundFrom - ERROR: Unknown parameters");
		return;
	}

	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);

	int volume = 100;
	bool looping = false;

	if (lua_isnumber(volumeOrLoopingObj)) {
		volume = (int)lua_getnumber(volumeOrLoopingObj);
		/* special handling if 5th parameter is a boolean */
		if (volume <= 1) {
			looping = volume;
			volume = (int)lua_getnumber(volumeObj);
		}
	} else {
		volume = (int)lua_getnumber(volumeObj);
		looping = !lua_isnil(volumeOrLoopingObj);
	}

	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	if (!sound) {
		warning("Lua_V2::PlayLoadedSoundFrom: can't find requested sound object");
		return;
	}
	int newvolume = volume;
	int newbalance = 64;
	Math::Vector3d pos(x, y, z);
	g_grim->getCurrSet()->calculateSoundPosition(pos, 30, volume, newvolume, newbalance);
	sound->setBalance(newbalance * 2 - 127);
	sound->setVolume(newvolume);
	sound->play(looping);
}

void Lua_V2::StopSound() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F')) {
		warning("Lua_V2::StopSound - ERROR: Unknown parameters");
		return;
	}

	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	if (!sound) {
		warning("Lua_V2::StopSound: can't find requested sound object");
		return;
	}
	sound->stop();
}

void Lua_V2::IsSoundPlaying() {
	lua_Object idObj = lua_getparam(1);

	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F')) {
		// can't use error since it actually may happen during normal operation
		warning("Lua_V2::IsSoundPlaying - ERROR: Unknown parameters");
		pushbool(false);
		return;
	}

	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	if (sound && sound->_track) {
		if (sound->_track->isPlaying()) {
			pushbool(true);
			return;
		}
	} else {
		warning("Lua_V2::IsSoundPlaying: no sound track associated");
	}
	pushbool(false);
}

void Lua_V2::PlaySound() {
	lua_Object strObj = lua_getparam(1);
	lua_Object volumeObj = lua_getparam(2);

	if (!lua_isstring(strObj)) {
		error("Lua_V2::PlaySound - ERROR: Unknown parameters");
		return;
	}
	const char *str = lua_getstring(strObj);

	int volume = 100;
	if (!lua_isnumber(volumeObj)) {
		warning("Lua_V2::PlaySound - Unexpected parameter(s) found, using default volume for %s", str);
	} else {
		volume = (int)lua_getnumber(volumeObj);
	}

	Common::String filename = addSoundSuffix(str);

	SoundTrack *track;

	Common::SeekableReadStream *stream = g_resourceloader->openNewStreamFile(filename, true);
	if (!stream) {
		Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::PlaySound: Could not find sound '%s'", filename.c_str());
		return;
	}

	if (g_grim->getGamePlatform() != Common::kPlatformPS2) {
		track = new AIFFTrack(Audio::Mixer::kSFXSoundType);
	} else {
		track = new SCXTrack(Audio::Mixer::kSFXSoundType);
	}

	track->openSound(filename, stream);
	if (g_grim->getGameFlags() != ADGF_DEMO) {
		track->setVolume(volume);
	}
	track->play();
}

void Lua_V2::PlaySoundFrom() {
	lua_Object strObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	/* FIXME: unknown parameter */
	lua_Object volumeOrUnknownObj = lua_getparam(5);
	lua_Object volumeObj = lua_getparam(6);

	int volume = 100;

	if (!lua_isstring(strObj)) {
		error("Lua_V2::PlaySoundFrom - ERROR: Unknown parameters");
		return;
	}

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj)) {
		error("Lua_V2::PlayLoadedSoundFrom - ERROR: Unknown parameters");
		return;
	}
	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);

	// arg5 is optional and if present, it is FALSE
	if (lua_isnumber(volumeObj)) {
		volume = (int)lua_getnumber(volumeObj);
	} else if (lua_isnumber(volumeOrUnknownObj)) {
		volume = (int)lua_getnumber(volumeOrUnknownObj);
	} else {
		error("Lua_V2::PlaySoundFrom - ERROR: Unknown parameters");
		return;
	}

	const char *str = lua_getstring(strObj);
	Common::String filename = addSoundSuffix(str);

	SoundTrack *track;

	Common::SeekableReadStream *stream = g_resourceloader->openNewStreamFile(filename, true);
	if (!stream) {
		warning("Lua_V2::PlaySoundFrom: Could not find sound '%s'", filename.c_str());
		return;
	}

	if (g_grim->getGamePlatform() != Common::kPlatformPS2) {
		track = new AIFFTrack(Audio::Mixer::kSFXSoundType);
	} else {
		track = new SCXTrack(Audio::Mixer::kSFXSoundType);
	}

	track->openSound(filename, stream);

	int newvolume = volume;
	int newbalance = 64;
	Math::Vector3d pos(x, y, z);
	g_grim->getCurrSet()->calculateSoundPosition(pos, 30, volume, newvolume, newbalance);
	track->setBalance(newbalance * 2 - 127);
	track->setVolume(newvolume);
	track->play();
}

void Lua_V2::GetSoundVolume() {
	lua_Object idObj = lua_getparam(1);
	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F')) {
		error("Lua_V2::GetSoundVolume: Unknown Parameters");
		return;
	}
	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	if (sound && sound->_track) {
		lua_pushnumber(sound->_track->getVolume());
	} else {
		warning("Lua_V2::GetSoundVolume: can't find sound track");
		lua_pushnumber(Audio::Mixer::kMaxMixerVolume);
	}
}

void Lua_V2::SetSoundVolume() {
	lua_Object idObj = lua_getparam(1);
	lua_Object volumeObj = lua_getparam(2);
	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F')) {
		error("Lua_V2::SetSoundVolume: no valid sound id");
		return;
	}
	if (!lua_isnumber(volumeObj)) {
		error("Lua_V2::SetSoundVolume - ERROR: Unknown parameters");
		return;
	}

	int volume = (int)lua_getnumber(volumeObj);
	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));

	if (sound) {
		sound->setVolume(volume);
	} else {
		warning("Lua_V2:SetSoundVolume: can't find sound track");
	}
}

void Lua_V2::UpdateSoundPosition() {
	lua_Object idObj = lua_getparam(1);
	lua_Object param2 = lua_getparam(2);
	lua_Object param3 = lua_getparam(3);
	lua_Object param4 = lua_getparam(4);

	if (!lua_isuserdata(idObj) || lua_tag(idObj) != MKTAG('A', 'I', 'F', 'F'))
		return;

	if (!lua_isnumber(param2) || !lua_isnumber(param3) || !lua_isnumber(param4))
		return;
	
	float x = lua_getnumber(param2);
	float y = lua_getnumber(param3);
	float z = lua_getnumber(param4);
	PoolSound *sound = PoolSound::getPool().getObject(lua_getuserdata(idObj));
	if (!sound)
		return;
	/* FIXME: store the original maximum volume in the PoolSound object */
	int newvolume = 100;
	int newbalance = 64;
	Math::Vector3d pos(x, y, z);
	g_grim->getCurrSet()->calculateSoundPosition(pos, 30, 100, newvolume, newbalance);
	sound->setBalance(newbalance * 2 - 127);
	sound->setVolume(newvolume);
}

void Lua_V2::ImSetMusicVol() {
	// This only seems to be used in the demo.
	lua_Object volumeObj = lua_getparam(1);

	if (!lua_isnumber(volumeObj))
		return;
	int volume = (int)lua_getnumber(volumeObj);
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImSetMusicVol: implement opcode, wants volume %d", volume);
}

void Lua_V2::ImSetSfxVol() {
	// This only seems to be used in the demo.
	lua_Object volumeObj = lua_getparam(1);

	if (!lua_isnumber(volumeObj))
		return;
	int volume = (int)lua_getnumber(volumeObj);
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImSetSfxVol: implement opcode, wants volume %d", volume);
}

void Lua_V2::ImSetVoiceVol() {
	// This only seems to be used in the demo.
	lua_Object volumeObj = lua_getparam(1);

	if (!lua_isnumber(volumeObj))
		return;
	int volume = (int)lua_getnumber(volumeObj);
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImSetVoiceVol: implement opcode, wants volume %d", volume);
}

void Lua_V2::ImSetVoiceEffect() {
	// This only seems to be used in the demo.
	lua_Object strObj = lua_getparam(1);

	if (!lua_isstring(strObj))
		return;

	const char *str = lua_getstring(strObj);
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImSetVoiceEffect: implement opcode, wants effect %s", str);
}

void Lua_V2::StopAllSounds() {
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::StopAllSounds: implement opcode");
}

void Lua_V2::ImPushState() {
	g_sound->pushState();
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImPushState: currently guesswork");
}
void Lua_V2::ImPopState() {
	g_sound->popState();
	Debug::debug(Debug::Sound | Debug::Scripts, "Lua_V2::ImPopState: currently guesswork");
}

} // end of namespace Grim
