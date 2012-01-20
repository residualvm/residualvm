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

#include "common/endian.h"

#include "engines/grim/debug.h"
#include "engines/grim/colormap.h"
#include "engines/grim/costume.h"
#include "engines/grim/textsplit.h"
#include "engines/grim/grim.h"
#include "engines/grim/resource.h"
#include "engines/grim/model.h"

#include "engines/grim/costume/chore.h"
#include "engines/grim/costume/head.h"
#include "engines/grim/costume/main_model_component.h"
#include "engines/grim/costume/colormap_component.h"
#include "engines/grim/costume/emimesh_component.h"
#include "engines/grim/costume/emiskel_component.h"
#include "engines/grim/costume/keyframe_component.h"
#include "engines/grim/costume/mesh_component.h"
#include "engines/grim/costume/lua_var_component.h"
#include "engines/grim/costume/sound_component.h"
#include "engines/grim/costume/bitmap_component.h"
#include "engines/grim/costume/material_component.h"
#include "engines/grim/costume/sprite_component.h"

namespace Grim {

// A costume in the Residual/GrimE engine consists of a set of
// components, and a set of chores.  Each component represents an
// on-screen object, or a keyframe animation, or a sound effect; each
// chore gives a set of instructions for how to move and/or activate
// or deactivate each component at certain times.
//
// Each actor contains a stack of costumes, on which a new costume can
// be pushed or from which an old costume can be popped at any time.
// For the most part, these costumes are independent.  The exception
// is the main model component ('MMDL'), for which multiple costumes
// share the same base 3D object (if they refer to the same file).
//
// This is complicated by the fact that multiple keyframe animations
// can have an effect on the positions of the 3D objects.  Each
// keyframe animation has certain nodes internally "tagged", and the
// keyframe components specify precedences for the tagged nodes and
// for the non-tagged nodes.  If the highest precedence for a given
// node is given by multiple keyframe animations, their contributions
// are averaged.
//
// Each component can implement several virtual methods which are
// called by the costume:
//
// init() -- allows the component to initialize itself.  This is
//           separate from the constructor since there are cases where
//           information from child components may be needed before
//           the object can be fully constructed.  This is particularly
//           the case with colormaps, which are needed before even
//           starting to load a 3D model.
// setKey(val) -- notifies the component of a change in the "state"
//                given by a playing chore
// update() -- gives the component a chance to update its internal
//             state once every frame
// setupTexture() -- sets up animated textures for the object.  This
//                   is a separate stage from update() since all the
//                   costumes on screen need to get updated before any
//                   drawing can start.
// draw() -- actually draws the component onto the screen
// reset() -- notifies the component that a chore controlling it
//            has stopped
//
// For the 3D objects, a model's component first initializes internal
// state for the model's nodes in its update() method.  Then the
// keyframes' update() methods work with this data to implement the
// precedence and add up all contributions for the highest precedence.
// Then the model's draw() method does the averaging and draws the
// polygons accordingly.  (Actually, this is a lie -- the top-level
// 3D objects draw themselves and all their children.  This makes it
// easier to move objects Manny is holding when his hands move, for
// example.)
//
// For bitmaps, the actual drawing is handled by the Set class.  The
// bitmaps to be drawn are associated to the needed camera setups
// using NewObjectState; bitmaps marked OBJSTATE_UNDERLAY and
// OBJSTATE_STATE are drawn first, then the 3D objects, then bitmaps
// marked OBJSTATE_OVERLAY.  So the BitmapComponent just needs to pass
// along setKey requests to the actual bitmap object.

Costume::Costume(const Common::String &fname, Common::SeekableReadStream *data, Costume *prevCost) :
		Object(), _head(new Head()), _chores(NULL) {

	_fname = fname;
	_lookAtRate = 200;
	_prevCostume = prevCost;
	if (g_grim->getGameType() == GType_MONKEY4) {
		loadEMI(data, prevCost);
	} else {
		TextSplitter ts(data);
		loadGRIM(ts, prevCost);
	}
}

void Costume::loadGRIM(TextSplitter &ts, Costume *prevCost) {
	ts.expectString("costume v0.1");
	ts.expectString("section tags");
	int numTags;
	ts.scanString(" numtags %d", 1, &numTags);
	tag32 *tags = new tag32[numTags];
	for (int i = 0; i < numTags; i++) {
		unsigned char t[4];
		int which;

		// Obtain a tag ID from the file
		ts.scanString(" %d '%c%c%c%c'", 5, &which, &t[0], &t[1], &t[2], &t[3]);
		// Force characters to upper case
		for (int j = 0; j < 4; j++)
			t[j] = toupper(t[j]);
		memcpy(&tags[which], t, sizeof(tag32));
	}

	ts.expectString("section components");
	ts.scanString(" numcomponents %d", 1, &_numComponents);
	_components = new Component *[_numComponents];
	for (int i = 0; i < _numComponents; i++) {
		int id, tagID, hash, parentID, namePos;
		const char *line = ts.getCurrentLine();
		Component *prevComponent = NULL;

		if (sscanf(line, " %d %d %d %d %n", &id, &tagID, &hash, &parentID, &namePos) < 4)
			error("Bad component specification line: `%s'", line);
		ts.nextLine();

		// A Parent ID of "-1" indicates that the component should
		// use the properties of the previous costume as a base
		if (parentID == -1) {
			if (prevCost) {
				MainModelComponent *mmc;

				// However, only the first item can actually share the
				// node hierarchy with the previous costume, so flag
				// that component so it knows what to do
				if (i == 0)
					parentID = -2;
				prevComponent = prevCost->_components[0];
				mmc = dynamic_cast<MainModelComponent *>(prevComponent);
				// Make sure that the component is valid
				if (!mmc)
					prevComponent = NULL;
			} else if (id > 0) {
				// Use the MainModelComponent of this costume as prevComponent,
				// so that the component can use its colormap.
				prevComponent = _components[0];
			}
		}
		// Actually load the appropriate component
		_components[id] = loadComponent(tags[tagID], parentID < 0 ? NULL : _components[parentID], parentID, line + namePos, prevComponent);
		_components[id]->setCostume(this);
	}

	delete[] tags;

	for (int i = 0; i < _numComponents; i++)
		if (_components[i]) {
			_components[i]->init();
		}

	ts.expectString("section chores");
	ts.scanString(" numchores %d", 1, &_numChores);
	_chores = new Chore *[_numChores];
	for (int i = 0; i < _numChores; i++) {
		int id, length, tracks;
		char name[32];
		ts.scanString(" %d %d %d %32s", 4, &id, &length, &tracks, name);
		_chores[id] = new Chore();
		_chores[id]->_length = length;
		_chores[id]->_numTracks = tracks;
		memcpy(_chores[id]->_name, name, 32);
		Debug::debug(Debug::Chores, "Loaded chore: %s\n", name);
	}

	ts.expectString("section keys");
	for (int i = 0; i < _numChores; i++) {
		int which;
		ts.scanString("chore %d", 1, &which);
		_chores[which]->load(i, this, ts);
	}
}

void Costume::loadEMI(Common::SeekableReadStream *data, Costume *prevCost) {
	Common::List<Component *>components;

	_numChores = data->readUint32LE();
	_chores = new Chore *[_numChores];
	for (int i = 0; i < _numChores; i++) {
		_chores[i] = new PoolChore();
		uint32 nameLength;
		Component *prevComponent = NULL;
		nameLength = data->readUint32LE();
		data->read(_chores[i]->_name, nameLength);
		float length;
		data->read(&length, 4);
		_chores[i]->_length = (int)length;

		_chores[i]->_owner = this;
		_chores[i]->_numTracks = data->readUint32LE();
		_chores[i]->_tracks = new ChoreTrack[_chores[i]->_numTracks];

		for (int k = 0; k < _chores[i]->_numTracks; k++) {
			int componentNameLength = data->readUint32LE();
			assert(componentNameLength < 64);

			char name[64];
			data->read(name, componentNameLength);

			data->readUint32LE();
			int parentID = data->readUint32LE();
			if (parentID == -1 && prevCost) {
				MainModelComponent *mmc;

				// However, only the first item can actually share the
				// node hierarchy with the previous costume, so flag
				// that component so it knows what to do
				if (i == 0)
					parentID = -2;
				prevComponent = prevCost->_components[0];
				mmc = dynamic_cast<MainModelComponent *>(prevComponent);
				// Make sure that the component is valid
				if (!mmc)
					prevComponent = NULL;
			}
			// Actually load the appropriate component
			Component *component = loadComponentEMI(parentID < 0 ? NULL : _components[parentID], parentID, name, prevComponent);


			//Component *component = loadComponentEMI(name, parent);

			components.push_back(component);

			ChoreTrack &track = _chores[i]->_tracks[k];
			track.numKeys = data->readUint32LE();
			track.keys = new TrackKey[track.numKeys];

			// this is probably wrong
			track.compID = 0;
			for (int j = 0; j < track.numKeys; j++) {
				float time, value;
				data->read(&time, 4);
				data->read(&value, 4);
				track.keys[j].time = (int)time;
				track.keys[j].value = (int)value;
			}
		}
		//_chores[i]._tracks->compID;
	}

	_numComponents = components.size();
	_components = new Component *[_numComponents];
	int i = 0;
	for (Common::List<Component *>::iterator it = components.begin(); it != components.end(); ++it, ++i) {
		_components[i] = *it;
		if (!_components[i])
			continue;
		_components[i]->setCostume(this);
		_components[i]->init();
	}
}

Costume::~Costume() {
	if (_chores) {
		stopChores();
		for (int i = _numComponents - 1; i >= 0; i--) {
			// The "Sprite" component can be NULL
			if (_components[i])
				delete _components[i];
		}
		delete[] _components;

		for (int i = 0; i < _numChores; ++i) {
			delete _chores[i];
		}
		delete[] _chores;
	}
	delete _head;
}

Component *Costume::loadComponent (tag32 tag, Component *parent, int parentID, const char *name, Component *prevComponent) {
	if (FROM_BE_32(tag) == MKTAG('M','M','D','L'))
		return new MainModelComponent(parent, parentID, name, prevComponent, tag);
	else if (FROM_BE_32(tag) == MKTAG('M','O','D','L'))
		return new ModelComponent(parent, parentID, name, prevComponent, tag);
	else if (FROM_BE_32(tag) == MKTAG('C','M','A','P'))
		return new ColormapComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('K','E','Y','F'))
		return new KeyframeComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('M','E','S','H'))
		return new MeshComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('L','U','A','V'))
		return new LuaVarComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('I','M','L','S'))
		return new SoundComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('B','K','N','D'))
		return new BitmapComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('M','A','T',' '))
		return new MaterialComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('S','P','R','T'))
		return new SpriteComponent(parent, parentID, name, tag);
	else if (FROM_BE_32(tag) == MKTAG('A','N','I','M')) //Used  in the demo
		return new BitmapComponent(parent, parentID, name, tag);

	char t[4];
	memcpy(t, &tag, sizeof(tag32));
	warning("loadComponent: Unknown tag '%c%c%c%c', name '%s'", t[0], t[1], t[2], t[3], name);
	return NULL;
}

Component *Costume::loadComponentEMI(Component *parent, int parentID, const char *name, Component *prevComponent) {
	// some have an exclimation mark, this could mean something.
	// for now, return 0 otherwise it will just crash in some other part.
	//return 0;

	assert(name[0] == '!');
	++name;

	char type[5];
	tag32 tag = 0;
	memcpy(&tag, name, 4);
	memcpy(&type, name, 4);
	type[4] = 0;

	name += 4;

	if (FROM_BE_32(tag) == MKTAG('m','e','s','h')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement MESH-handling: %s" , name);
		return new EMIMeshComponent(parent, parentID, name, prevComponent, tag);
	} else if (FROM_BE_32(tag) == MKTAG('s','k','e','l')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement SKEL-handling: %s" , name);
		return new EMISkelComponent(parent, parentID, name, prevComponent, tag);
	} else if (FROM_BE_32(tag) == MKTAG('t','e','x','i')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement TEXI-handling: %s" , name);
		//return new MaterialComponent(parent, parentID, name, tag);
	} else if (FROM_BE_32(tag) == MKTAG('a','n','i','m')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement ANIM-handling: %s" , name);
		//return new KeyframeComponent(parent, parentID, name, tag);
	} else if (FROM_BE_32(tag) == MKTAG('l','u','a','c')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement LUAC-handling: %s" , name);
	} else if (FROM_BE_32(tag) == MKTAG('l','u','a','v')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement LUAV-handling: %s" , name);
		//return new LuaVarComponent(parent, parentID, name, tag);
	} else if (FROM_BE_32(tag) == MKTAG('s','p','r','t')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement SPRT-handling: %s" , name);
		//return new SpriteComponent(parent, parentID, name, tag);
	} else if (FROM_BE_32(tag) == MKTAG('s','h','a','d')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement SHAD-handling: %s" , name);
	} else {
		error("Actor::loadComponentEMI missing tag: %s for %s", name, type);
	}
	/*
	char t[4];
	memcpy(t, &tag, sizeof(tag32));
	warning("loadComponent: Unknown tag '%c%c%c%c', name '%s'", t[0], t[1], t[2], t[3], name);*/
	return NULL;
}

ModelComponent *Costume::getMainModelComponent() const {
	for (int i = 0; i < _numComponents; i++) {
		// Needs to handle Main Models (pigeons) and normal Models
		// (when Manny climbs the rope)
		if (FROM_BE_32(_components[i]->getTag()) == MKTAG('M','M','D','L'))
			return static_cast<ModelComponent *>(_components[i]);
	}
	return NULL;
}

ModelNode *Costume::getModelNodes() {
	ModelComponent *comp = getMainModelComponent();
	if (comp) {
		return comp->getHierarchy();
	}
	return NULL;
}

Model *Costume::getModel() {
	ModelComponent *comp = getMainModelComponent();
	if (comp) {
		return comp->getModel();
	}
	return NULL;
}

void Costume::setChoreLastFrame(int num) {
	if (num < 0 || num >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", num, _numChores);
		return;
	}
	_chores[num]->setLastFrame();
}

void Costume::setChoreLooping(int num, bool val) {
	if (num < 0 || num >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", num, _numChores);
		return;
	}
	_chores[num]->setLooping(val);
}

void Costume::playChoreLooping(int num) {
	if (num < 0 || num >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", num, _numChores);
		return;
	}
	_chores[num]->playLooping();
	if (Common::find(_playingChores.begin(), _playingChores.end(), _chores[num]) == _playingChores.end())
		_playingChores.push_back(_chores[num]);
}

Chore *Costume::getChore(const char *name) {
	for (int i = 0; i < _numChores; ++i) {
		if (strcmp(_chores[i]->_name, name) == 0) {
			return _chores[i];
		}
	}
	return 0;
}

void Costume::playChore(const char *name) {
	for (int i = 0; i < _numChores; ++i) {
			if (strcmp(_chores[i]->_name, name) == 0) {
			playChore(i);
			return;
		}
	}
	warning("Costume::playChore: Could not find chore: %s", name);
	return;
}

void Costume::playChore(int num) {
	if (num < 0 || num >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", num, _numChores);
		return;
	}
	_chores[num]->play();
	if (Common::find(_playingChores.begin(), _playingChores.end(), _chores[num]) == _playingChores.end())
		_playingChores.push_back(_chores[num]);
}

void Costume::stopChore(int num) {
	if (num < 0 || num >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", num, _numChores);
		return;
	}
	_chores[num]->stop();
	_playingChores.remove(_chores[num]);
}

void Costume::setColormap(const Common::String &map) {
	// Sometimes setColormap is called on a null costume,
	// see where raoul is gone in hh.set
	if (!map.size())
		return;
	_cmap = g_resourceloader->getColormap(map);
	for (int i = 0; i < _numComponents; i++)
		_components[i]->setColormap(NULL);
}

void Costume::stopChores() {
	for (int i = 0; i < _numChores; i++) {
		_chores[i]->stop();
		_playingChores.remove(_chores[i]);
	}
}

void Costume::fadeChoreIn(int chore, int msecs) {
	if (chore < 0 || chore >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", chore, _numChores);
		return;
	}
	_chores[chore]->fadeIn(msecs);
	if (Common::find(_playingChores.begin(), _playingChores.end(), _chores[chore]) == _playingChores.end())
		_playingChores.push_back(_chores[chore]);
}

void Costume::fadeChoreOut(int chore, int msecs) {
	if (chore < 0 || chore >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", chore, _numChores);
		return;
	}
	_chores[chore]->fadeOut(msecs);
}

int Costume::isChoring(const char *name, bool excludeLooping) {
	for (int i = 0; i < _numChores; i++) {
		if (!strcmp(_chores[i]->_name, name) && _chores[i]->_playing && !(excludeLooping && _chores[i]->_looping))
			return i;
	}
	return -1;
}

int Costume::isChoring(int num, bool excludeLooping) {
	if (num < 0 || num >= _numChores) {
		Debug::warning(Debug::Chores, "Requested chore number %d is outside the range of chores (0-%d)", num, _numChores);
		return -1;
	}
	if (_chores[num]->_playing && !(excludeLooping && _chores[num]->_looping))
		return num;
	else
		return -1;
}

int Costume::isChoring(bool excludeLooping) {
	for (int i = 0; i < _numChores; i++) {
		if (_chores[i]->_playing && !(excludeLooping && _chores[i]->_looping))
			return i;
	}
	return -1;
}

void Costume::setupTextures() {
	for (int i = 0; i < _numComponents; i++)
		if (_components[i])
			_components[i]->setupTexture();
}

void Costume::draw() {
	for (int i = 0; i < _numComponents; i++)
		if (_components[i])
			_components[i]->draw();
}

void Costume::getBoundingBox(int *x1, int *y1, int *x2, int *y2) {
	for (int i = 0; i < _numComponents; i++) {
		ModelComponent *c = dynamic_cast<ModelComponent *>(_components[i]);
		if (c) {
			c->getBoundingBox(x1, y1, x2, y2);
		}
	}
}

int Costume::update(uint time) {
	for (Common::List<Chore*>::iterator i = _playingChores.begin(); i != _playingChores.end(); ++i) {
		(*i)->update(time);
		if (!(*i)->_playing) {
			i = _playingChores.erase(i);
			--i;
		}
	}

	int marker = 0;
	for (int i = 0; i < _numComponents; i++) {
		if (_components[i]) {
			_components[i]->setMatrix(_matrix);
			int m = _components[i]->update(time);
			if (m > 0) {
				marker = m;
			}
		}
	}

	return marker;
}

void Costume::animate() {
	for (int i = 0; i < _numComponents; i++) {
		if (_components[i]) {
			_components[i]->animate();
		}
	}
}

void Costume::moveHead(bool entering, const Math::Vector3d &lookAt) {
	_head->lookAt(entering, lookAt, _lookAtRate, _matrix);
}

void Costume::setHead(int joint1, int joint2, int joint3, float maxRoll, float maxPitch, float maxYaw) {
	_head->setJoints(joint1, joint2, joint3);
	_head->loadJoints(getModelNodes());
	_head->setMaxAngles(maxPitch, maxYaw, maxRoll);
}

void Costume::setLookAtRate(float rate) {
	_lookAtRate = rate;
}

float Costume::getLookAtRate() const {
	return _lookAtRate;
}

void Costume::setPosRotate(Math::Vector3d pos, const Math::Angle &pitch,
						   const Math::Angle &yaw, const Math::Angle &roll) {
	_matrix.setPosition(pos);
	_matrix.buildFromPitchYawRoll(pitch, yaw, roll);
}

Math::Matrix4 Costume::getMatrix() const {
	return _matrix;
}

Costume *Costume::getPreviousCostume() const {
	return _prevCostume;
}

void Costume::saveState(SaveGame *state) const {
	if (_cmap) {
		state->writeLEUint32(1);
		state->writeString(_cmap->getFilename());
	} else {
		state->writeLEUint32(0);
	}

	for (int i = 0; i < _numChores; ++i) {
		Chore *c = _chores[i];

		state->writeLESint32(c->_hasPlayed);
		state->writeLESint32(c->_playing);
		state->writeLESint32(c->_looping);
		state->writeLESint32(c->_currTime);
	}

	for (int i = 0; i < _numComponents; ++i) {
		Component *c = _components[i];

		if (c) {
			state->writeLESint32(c->_visible);
			state->writeVector3d(c->_matrix.getPosition());
			c->saveState(state);
		}
	}

	state->writeLEUint32(_playingChores.size());
	for (Common::List<Chore*>::const_iterator i = _playingChores.begin(); i != _playingChores.end(); ++i) {
		state->writeLESint32((*i)->getId());
	}

	// FIXME: Decomment this!!
// 	state.writeFloat(_lookAtRate);
	_head->saveState(state);
}

bool Costume::restoreState(SaveGame *state) {
	if (state->readLEUint32()) {
		Common::String str = state->readString();
		setColormap(str);
	}

	for (int i = 0; i < _numChores; ++i) {
		Chore *c = _chores[i];

		c->_hasPlayed = state->readLESint32();
		c->_playing = state->readLESint32();
		c->_looping = state->readLESint32();
		c->_currTime = state->readLESint32();
	}
	for (int i = 0; i < _numComponents; ++i) {
		Component *c = _components[i];

		if (c) {
			c->_visible = state->readLESint32();
			c->_matrix.setPosition(state->readVector3d());
			c->restoreState(state);
		}
	}

	int numPlayingChores = state->readLEUint32();
	for (int i = 0; i < numPlayingChores; ++i) {
		int id = state->readLESint32();
		_playingChores.push_back(_chores[id]);
	}

	// FIXME: Decomment this!!
// 	_lookAtRate = state->readFloat();
	_head->restoreState(state);
	_head->loadJoints(getModelNodes());

	return true;
}

} // end of namespace Grim
