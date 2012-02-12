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

#define FORBIDDEN_SYMBOL_EXCEPTION_printf

#include "common/foreach.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/light.h"

#include "math/matrix3.h"

#include "engines/grim/debug.h"
#include "engines/grim/set.h"
#include "engines/grim/textsplit.h"
#include "engines/grim/colormap.h"
#include "engines/grim/grim.h"
#include "engines/grim/savegame.h"
#include "engines/grim/resource.h"
#include "engines/grim/bitmap.h"

#include "engines/grim/sound.h"

namespace Grim {

Set::Set(const Common::String &sceneName, Common::SeekableReadStream *data) :
		PoolObject<Set, MKTAG('S', 'E', 'T', ' ')>(), _locked(false), _name(sceneName), _enableLights(false) {

	char header[7];
	data->read(header, 7);
	data->seek(0, SEEK_SET);
	if (memcmp(header, "section", 7) == 0) {
		TextSplitter ts(data);
		loadText(ts);
	} else {
		loadBinary(data);
	}
}

Set::Set() :
	PoolObject<Set, MKTAG('S', 'E', 'T', ' ')>(), _cmaps(NULL) {

}

Set::~Set() {
	if (_cmaps || g_grim->getGameType() == GType_MONKEY4) {
		delete[] _cmaps;
		for (int i = 0; i < _numSetups; ++i) {
			delete _setups[i]._bkgndBm;
			delete _setups[i]._bkgndZBm;
		}
		delete[] _setups;
		turnOffLights();
		delete[] _lights;
		for (int i = 0; i < _numSectors; ++i) {
			delete _sectors[i];
		}
		delete[] _sectors;
		foreach (ObjectState *s, _states)
			delete s;
	}
}

void Set::loadText(TextSplitter &ts) {
	char tempBuf[256];

	ts.expectString("section: colormaps");
	ts.scanString(" numcolormaps %d", 1, &_numCmaps);
	_cmaps = new ObjectPtr<CMap>[_numCmaps];
	char cmap_name[256];
	for (int i = 0; i < _numCmaps; i++) {
		ts.scanString(" colormap %256s", 1, cmap_name);
		_cmaps[i] = g_resourceloader->getColormap(cmap_name);
	}

	if (ts.checkString("section: objectstates") || ts.checkString("sections: object_states")) {
		ts.nextLine();
		ts.scanString(" tot_objects %d", 1, &_numObjectStates);
		char object_name[256];
		for (int l = 0; l < _numObjectStates; l++) {
			ts.scanString(" object %256s", 1, object_name);
		}
	} else {
		_numObjectStates = 0;
	}

	ts.expectString("section: setups");
	ts.scanString(" numsetups %d", 1, &_numSetups);
	_setups = new Setup[_numSetups];
	for (int i = 0; i < _numSetups; i++)
		_setups[i].load(ts);
	_currSetup = _setups;

	_numSectors = -1;
	_numLights = -1;
	_lights = NULL;
	_sectors = NULL;

	_minVolume = 0;
	_maxVolume = 0;

	// Lights are optional
	if (ts.isEof())
		return;

	ts.expectString("section: lights");
	ts.scanString(" numlights %d", 1, &_numLights);
	_lights = new Light[_numLights];
	for (int i = 0; i < _numLights; i++) {
		_lights[i].load(ts);
		_lightsList.push_back(&_lights[i]);
	}

	// Calculate the number of sectors
	ts.expectString("section: sectors");
	if (ts.isEof())	// Sectors are optional, but section: doesn't seem to be
		return;

	int sectorStart = ts.getLineNumber();
	_numSectors = 0;
	// Find the number of sectors (while the sectors usually
	// count down from the highest number there are a few
	// cases where they count up, see hh.set for example)
	while (!ts.isEof()) {
		ts.scanString(" %s", 1, tempBuf);
		if (!scumm_stricmp(tempBuf, "sector"))
			_numSectors++;
	}
	// Allocate and fill an array of sector info
	_sectors = new Sector*[_numSectors];
	ts.setLineNumber(sectorStart);
	for (int i = 0; i < _numSectors; i++) {
		// Use the ids as index for the sector in the array.
		// This way when looping they are checked from the id 0 sto the last,
		// which seems important for sets with overlapping camera sectors, like ga.set.
		Sector *s = new Sector();
		s->load(ts);
		_sectors[s->getSectorId()] = s;
	}
}

void Set::loadBinary(Common::SeekableReadStream *data) {
	// yes, an array of size 0
	_cmaps = NULL;//new CMapPtr[0];


	_numSetups = data->readUint32LE();
	_setups = new Setup[_numSetups];
	for (int i = 0; i < _numSetups; i++)
		_setups[i].loadBinary(data);
	_currSetup = _setups;

	_numSectors = 0;
	_numLights = 0;
	_lights = NULL;
	_sectors = NULL;

	_minVolume = 0;
	_maxVolume = 0;

	// the rest may or may not be optional. Might be a good idea to check if there is no more data.

	_numLights = data->readUint32LE();
	_lights = new Light[_numLights];
	for (int i = 0; i < _numLights; i++) {
		_lights[i].loadBinary(data);
		_lightsList.push_back(&_lights[i]);
	}

	// bypass light stuff for now
	_numLights = 0;

	_numSectors = data->readUint32LE();
	// Allocate and fill an array of sector info
	_sectors = new Sector*[_numSectors];
	for (int i = 0; i < _numSectors; i++) {
		_sectors[i] = new Sector();
		_sectors[i]->loadBinary(data);
	}
}

void Set::saveState(SaveGame *savedState) const {
	savedState->writeString(_name);
	savedState->writeLESint32(_numCmaps);
	for (int i = 0; i < _numCmaps; ++i) {
		savedState->writeString(_cmaps[i]->getFilename());
	}
	savedState->writeLEUint32((uint32)(_currSetup - _setups)); // current setup id
	savedState->writeBool(_locked);
	savedState->writeBool(_enableLights);
	savedState->writeLESint32(_minVolume);
	savedState->writeLESint32(_maxVolume);

	savedState->writeLEUint32(_states.size());
	for (StateList::const_iterator i = _states.begin(); i != _states.end(); ++i) {
		savedState->writeLESint32((*i)->getId());
	}

	//Setups
	savedState->writeLESint32(_numSetups);
	for (int i = 0; i < _numSetups; ++i) {
		_setups[i].saveState(savedState);
	}

	//Sectors
	savedState->writeLESint32(_numSectors);
	for (int i = 0; i < _numSectors; ++i) {
		_sectors[i]->saveState(savedState);
	}

	//Lights
	savedState->writeLESint32(_numLights);
	for (int i = 0; i < _numLights; ++i) {
		_lights[i].saveState(savedState);
	}
}

bool Set::restoreState(SaveGame *savedState) {
	_name = savedState->readString();
	_numCmaps = savedState->readLESint32();
	_cmaps = new CMapPtr[_numCmaps];
	for (int i = 0; i < _numCmaps; ++i) {
		Common::String str = savedState->readString();
		_cmaps[i] = g_resourceloader->getColormap(str);
	}

	int32 currSetupId = savedState->readLEUint32();
	_locked           = savedState->readBool();
	_enableLights     = savedState->readBool();
	_minVolume        = savedState->readLESint32();
	_maxVolume        = savedState->readLESint32();

	_numObjectStates = savedState->readLESint32();
	_states.clear();
	for (int i = 0; i < _numObjectStates; ++i) {
		int32 id = savedState->readLESint32();
		ObjectState *o = ObjectState::getPool().getObject(id);
		_states.push_back(o);
	}

	//Setups
	_numSetups = savedState->readLESint32();
	_setups = new Setup[_numSetups];
	_currSetup = _setups + currSetupId;
	for (int i = 0; i < _numSetups; ++i) {
		_setups[i].restoreState(savedState);
	}

    //Sectors
	_numSectors = savedState->readLESint32();
	if (_numSectors > 0) {
		_sectors = new Sector*[_numSectors];
		for (int i = 0; i < _numSectors; ++i) {
			_sectors[i] = new Sector();
			_sectors[i]->restoreState(savedState);
		}
	} else {
		_sectors = NULL;
	}

	_numLights = savedState->readLESint32();
	_lights = new Light[_numLights];
	for (int i = 0; i < _numLights; i++) {
		_lights[i].restoreState(savedState);
		_lightsList.push_back(&_lights[i]);
	}

	return true;
}

void Set::Setup::load(TextSplitter &ts) {
	char buf[256];

	ts.scanString(" setup %256s", 1, buf);
	_name = buf;

	ts.scanString(" background %256s", 1, buf);
	_bkgndBm = Bitmap::create(buf);
	if (!_bkgndBm) {
		Debug::warning(Debug::Bitmaps | Debug::Sets,
					   "Unable to load scene bitmap: %s\n", buf);
	} else {
		Debug::debug(Debug::Bitmaps | Debug::Sets,
					 "Loaded scene bitmap: %s\n", buf);
	}

	// ZBuffer is optional
	_bkgndZBm = NULL;
	if (ts.checkString("zbuffer")) {
		ts.scanString(" zbuffer %256s", 1, buf);
		// Don't even try to load if it's the "none" bitmap
		if (strcmp(buf, "<none>.lbm") != 0) {
			_bkgndZBm = Bitmap::create(buf);
			Debug::debug(Debug::Bitmaps | Debug::Sets,
						 "Loading scene z-buffer bitmap: %s\n", buf);
		}
	}

	ts.scanString(" position %f %f %f", 3, &_pos.x(), &_pos.y(), &_pos.z());
	ts.scanString(" interest %f %f %f", 3, &_interest.x(), &_interest.y(), &_interest.z());
	ts.scanString(" roll %f", 1, &_roll);
	ts.scanString(" fov %f", 1, &_fov);
	ts.scanString(" nclip %f", 1, &_nclip);
	ts.scanString(" fclip %f", 1, &_fclip);
	for (;;) {
		if (ts.checkString("object_art"))
			ts.scanString(" object_art %256s", 1, buf);
		else
			break;
		if (ts.checkString("object_z"))
			ts.scanString(" object_z %256s", 1, buf);
	}
}

void Set::Setup::loadBinary(Common::SeekableReadStream *data) {
	char name[128];
	data->read(name, 128);
	_name = Common::String(name);

	// Skip an unknown number (this is the stringlength of the following string)
	int fNameLen = 0;
	fNameLen = data->readUint32LE();

	char* fileName = new char[fNameLen];
	data->read(fileName,fNameLen);

	_bkgndZBm = NULL;
	_bkgndBm = Bitmap::create(fileName);


	data->read(_pos.getData(), 12);

	data->read(_interest.getData(), 12);

	data->read(&_roll, 4);
	data->read(&_fov, 4);
	data->read(&_nclip, 4);
	data->read(&_fclip, 4);

	delete[] fileName;
}

void Set::Setup::saveState(SaveGame *savedState) const {
	//name
	savedState->writeString(_name);

	//bkgndBm
	if (_bkgndBm) {
		savedState->writeLESint32(_bkgndBm->getId());
	} else {
		savedState->writeLESint32(0);
	}

	//bkgndZBm
	if (_bkgndZBm) {
		savedState->writeLESint32(_bkgndZBm->getId());
	} else {
		savedState->writeLESint32(0);
	}

	savedState->writeVector3d(_pos);
	savedState->writeVector3d(_interest);
	savedState->writeFloat(_roll);
	savedState->writeFloat(_fov);
	savedState->writeFloat(_nclip);
	savedState->writeFloat(_fclip);
}

bool Set::Setup::restoreState(SaveGame *savedState) {
	_name = savedState->readString();

	_bkgndBm = Bitmap::getPool().getObject(savedState->readLESint32());
	_bkgndZBm = Bitmap::getPool().getObject(savedState->readLESint32());

	_pos      = savedState->readVector3d();
	_interest = savedState->readVector3d();
	_roll     = savedState->readFloat();
	_fov      = savedState->readFloat();
	_nclip    = savedState->readFloat();
	_fclip    = savedState->readFloat();

	return true;
}

void Light::load(TextSplitter &ts) {
	char buf[256];

	// Light names can be null, but ts doesn't seem flexible enough to allow this
	if (strlen(ts.getCurrentLine()) > strlen(" light"))
		ts.scanString(" light %256s", 1, buf);
	else {
		ts.nextLine();
		strcpy(buf, "");
	}
	_name = buf;

	ts.scanString(" type %256s", 1, buf);
	Common::String type = buf;

	Math::Vector3d pos, dir;
	ts.scanString(" position %f %f %f", 3, &pos.x(), &pos.y(), &pos.z());
	ts.scanString(" direction %f %f %f", 3, &dir.x(), &dir.y(), &dir.z());
	float intensity, umbraangle, penumbraangle;
	ts.scanString(" intensity %f", 1, &intensity);
	ts.scanString(" umbraangle %f", 1, &umbraangle);
	ts.scanString(" penumbraangle %f", 1, &penumbraangle);

	int r, g, b;
	ts.scanString(" color %d %d %d", 3, &r, &g, &b);

	_enabled = true;

	AGL::Light::Type t = AGL::Light::Point;
	if (type == "directi")
		t = AGL::Light::Directional;
	else if (type == "spot")
		t = AGL::Light::Spot;

	_light = AGLMan.createLight(t);
	_light->setPosition(pos);
	_light->setDirection(dir);
	_light->setColor(Graphics::Color(r, g, b));
	_light->setCutoff(t == AGL::Light::Spot ? penumbraangle : 180.f);
	_light->setIntensity(intensity);
}

void Light::loadBinary(Common::SeekableReadStream *data) {
	// skip lights for now
	data->seek(100, SEEK_CUR);
}

void Light::saveState(SaveGame *savedState) const {
	//name
	savedState->writeString(_name);
	savedState->writeBool(_enabled);

	//type
	savedState->writeLESint32(_light->getType());

	savedState->writeVector3d(_light->getPosition());
	savedState->writeVector3d(_light->getDirection());

	savedState->writeColor(Color(_light->getColor()));

	savedState->writeFloat(_light->getIntensity());
	savedState->writeFloat(_light->getCutoff());
// 	savedState->writeFloat(_umbraangle);
}

bool Light::restoreState(SaveGame *savedState) {
	_name = savedState->readString();
	_enabled = savedState->readBool();

	AGL::Light::Type t = (AGL::Light::Type)savedState->readLESint32();
	_light = AGLMan.createLight(t);

	_light->setPosition(savedState->readVector3d());
	_light->setDirection(savedState->readVector3d());

	_light->setColor(savedState->readColor());

	_light->setIntensity(savedState->readFloat());

	float penumbraangle = savedState->readFloat();
	_light->setCutoff(t == AGL::Light::Spot ? penumbraangle : 180.f);
// 	_umbraangle    = savedState->readFloat();

	return true;
}

void Set::Setup::setupCamera() const {
	// Ignore nclip_ and fclip_ for now.  This fixes:
	// (a) Nothing was being displayed in the Land of the Living
	// diner because lr.set set nclip to 0.
	// (b) The zbuffers for setups with different nclip or
	// fclip values.  If it turns out that the clipping planes
	// are important at some point, we'll need to modify the
	// zbuffer transformation in bitmap.cpp to take nclip_ and
	// fclip_ into account.
	AGLMan.setupCamera(_fov, 0.01f, 3276.8f, _roll);
	Math::Matrix3x3 m;
	if (g_grim->getGameType() == GType_MONKEY4) {
		m.buildFromForwardRightUp(Math::Vector3d(0, 1, 0), Math::Vector3d(1, 0, 0), Math::Vector3d(0, 0, 1));
	} else {
		m.buildFromForwardRightUp(Math::Vector3d(1, 0, 0), Math::Vector3d(0, 1, 0), Math::Vector3d(0, 0, 1));
	}
	AGLMan.positionCamera(m, _pos, _interest);
}

class Sorter {
public:
	Sorter(const Math::Vector3d &pos) {
		_pos = pos;
	}

	bool operator()(Light *l1, Light *l2) const {
		return (l1->_light->getPosition() - _pos).getSquareMagnitude() < (l2->_light->getPosition() - _pos).getSquareMagnitude();
	}

	Math::Vector3d _pos;
};

void Set::setupLights(const Math::Vector3d &pos) {
	if (!_enableLights) {
		AGLMan.disableLighting();
		return;
	}

	// Sort the ligths from the nearest to the farthest to the pos.
	Sorter sorter(pos);
	Common::sort(_lightsList.begin(), _lightsList.end(), sorter);

	foreach (Light *l, _lightsList) {
		if (l->_enabled) {
			l->_light->enable();
		}
	}
}

void Set::turnOffLights() {
	_enableLights = false;
	for (int i = 0; i < _numLights; i++) {
		Light *l = &_lights[i];
		if (l->_enabled) {
			l->_light->disable();
		}
	}
}

void Set::setSetup(int num) {
	// Looks like num is zero-based so >= should work to find values
	// that are out of the range of valid setups

	// Quite weird, but this is what the original does when the setup id is above
	// the upper bound.
	if (num >= _numSetups)
		num %= _numSetups;

	if (num < 0) {
		error("Failed to change scene setup, value out of range");
		return;
	}
	_currSetup = _setups + num;
	g_grim->flagRefreshShadowMask(true);
}

void Set::drawBackground() const {
	if (_currSetup->_bkgndZBm) // Some screens have no zbuffer mask (eg, Alley)
		_currSetup->_bkgndZBm->draw();

	if (!_currSetup->_bkgndBm) {
		// This should fail softly, for some reason jumping to the signpost (sg) will load
		// the scene in such a way that the background isn't immediately available
		warning("Background hasn't loaded yet for setup %s in %s!", _currSetup->_name.c_str(), _name.c_str());
	} else {
		_currSetup->_bkgndBm->draw();
	}
}

void Set::drawBitmaps(ObjectState::Position stage) {
	for (StateList::iterator i = _states.reverse_begin(); i != _states.end(); --i) {
		if ((*i)->getPos() == stage && _currSetup == _setups + (*i)->getSetupID())
			(*i)->draw();
	}
}

Sector *Set::findPointSector(const Math::Vector3d &p, Sector::SectorType type) {
	for (int i = 0; i < _numSectors; i++) {
		Sector *sector = _sectors[i];
		if (sector && (sector->getType() & type) && sector->isVisible() && sector->isPointInSector(p))
			return sector;
	}
	return NULL;
}

void Set::findClosestSector(const Math::Vector3d &p, Sector **sect, Math::Vector3d *closestPoint) {
	Sector *resultSect = NULL;
	Math::Vector3d resultPt = p;
	float minDist = 0.0;

	for (int i = 0; i < _numSectors; i++) {
		Sector *sector = _sectors[i];
		if ((sector->getType() & Sector::WalkType) == 0 || !sector->isVisible())
			continue;
		Math::Vector3d closestPt = sector->getClosestPoint(p);
		float thisDist = (closestPt - p).getMagnitude();
		if (!resultSect || thisDist < minDist) {
			resultSect = sector;
			resultPt = closestPt;
			minDist = thisDist;
		}
	}

	if (sect)
		*sect = resultSect;

	if (closestPoint)
		*closestPoint = resultPt;
}

void Set::shrinkBoxes(float radius) {
	for (int i = 0; i < _numSectors; i++) {
		Sector *sector = _sectors[i];
		sector->shrink(radius);
	}
}

void Set::unshrinkBoxes() {
	for (int i = 0; i < _numSectors; i++) {
		Sector *sector = _sectors[i];
		sector->unshrink();
	}
}

void Set::setLightIntensity(const char *light, float intensity) {
	for (int i = 0; i < _numLights; ++i) {
		Light &l = _lights[i];
		if (l._name == light) {
			l._light->setIntensity(intensity);
			return;
		}
	}
}

void Set::setLightIntensity(int light, float intensity) {
	Light &l = _lights[light];
	l._light->setIntensity(intensity);
}

void Set::setLightEnabled(const char *light, bool enabled) {
	for (int i = 0; i < _numLights; ++i) {
		Light &l = _lights[i];
		if (l._name == light) {
			l._enabled = enabled;
			return;
		}
	}
}

void Set::setLightEnabled(int light, bool enabled) {
	Light &l = _lights[light];
	l._enabled = enabled;
}

void Set::setLightPosition(const char *light, const Math::Vector3d &pos) {
	for (int i = 0; i < _numLights; ++i) {
		Light &l = _lights[i];
		if (l._name == light) {
			l._light->setPosition(pos);
			return;
		}
	}
}

void Set::setLightPosition(int light, const Math::Vector3d &pos) {
	Light &l = _lights[light];
	l._light->setPosition(pos);
}

void Set::setSoundPosition(const char *soundName, const Math::Vector3d &pos) {
	setSoundPosition(soundName, pos, _minVolume, _maxVolume);
}

void Set::setSoundPosition(const char *soundName, const Math::Vector3d &pos, int minVol, int maxVol) {
	// TODO: The volume and pan needs to be updated when the setup changes.
	Math::Vector3d cameraPos = _currSetup->_pos;
	Math::Vector3d vector = pos - cameraPos;
	float distance = vector.getMagnitude();
	float diffVolume = maxVol - minVol;
	//This 8.f is a guess, so it may need some adjusting
	int newVolume = (int)(8.f * diffVolume / distance);
	newVolume += minVol;
	if (newVolume > _maxVolume)
		newVolume = _maxVolume;
	g_sound->setVolume(soundName, newVolume);

	Math::Vector3d cameraVector =_currSetup->_interest - _currSetup->_pos;
	Math::Vector3d up(0,0,1);
	Math::Vector3d right;
	cameraVector.normalize();
	float roll = -_currSetup->_roll * LOCAL_PI / 180.f;
	float cosr = cos(roll);
	// Rotate the up vector by roll.
	up = up * cosr + Math::Vector3d::crossProduct(cameraVector, up) * sin(roll) +
		cameraVector * Math::Vector3d::dotProduct(cameraVector, up) * (1 - cosr);
	right = Math::Vector3d::crossProduct(cameraVector, up);
	right.normalize();
	float angle = atan2(Math::Vector3d::dotProduct(vector, right),
						Math::Vector3d::dotProduct(vector, cameraVector));
	float pan = sin(angle);
	g_sound->setPan(soundName, (int)((pan + 1.f) / 2.f * 127.f + 0.5f));
}

Sector *Set::getSectorBase(int id) {
	if ((_numSectors >= 0) && (id < _numSectors))
		return _sectors[id];
	else
		return NULL;
}

Sector *Set::getSector(const Common::String &name) {
	for (int i = 0; i < _numSectors; i++) {
		Sector *sector = _sectors[i];
		if (strstr(sector->getName(), name.c_str())) {
			return sector;
		}
	}
	return NULL;
}

void Set::setSoundParameters(int minVolume, int maxVolume) {
	_minVolume = minVolume;
	_maxVolume = maxVolume;
}

void Set::getSoundParameters(int *minVolume, int *maxVolume) {
	*minVolume = _minVolume;
	*maxVolume = _maxVolume;
}

void Set::addObjectState(const ObjectState::Ptr &s) {
	_states.push_front(s);
}

ObjectState *Set::addObjectState(int setupID, ObjectState::Position pos, const char *bitmap, const char *zbitmap, bool transparency) {
	ObjectState *state = findState(bitmap);

	if (state) {
		return state;
	}

	state = new ObjectState(setupID, pos, bitmap, zbitmap, transparency);
	addObjectState(state);

	return state;
}

ObjectState *Set::findState(const Common::String &filename) {
	// Check the different state objects for the bitmap
	for (StateList::iterator i = _states.begin(); i != _states.end(); ++i) {
		const Common::String &file = (*i)->getBitmapFilename();

		if (file == filename)
			return *i;
		if (file.compareToIgnoreCase(filename) == 0) {
			Debug::warning(Debug::Sets, "State object request '%s' matches object '%s' but is the wrong case", filename.c_str(), file.c_str());
			return *i;
		}
	}
	return NULL;
}

void Set::moveObjectStateToFront(const ObjectState::Ptr &s) {
	_states.remove(s);
	_states.push_front(s);
	// Make the state invisible. This hides the deadbolt when brennis closes the switcher door
	// in the server room (tu), and therefore fixes https://github.com/residualvm/residualvm/issues/24
	s->setActiveImage(0);
}

void Set::moveObjectStateToBack(const ObjectState::Ptr &s) {
	_states.remove(s);
	_states.push_back(s);
}

} // end of namespace Grim
