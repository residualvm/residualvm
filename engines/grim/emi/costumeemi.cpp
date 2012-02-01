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
#include "engines/grim/costume.h"
#include "engines/grim/grim.h"
#include "engines/grim/resource.h"
#include "engines/grim/emi/costumeemi.h"
#include "engines/grim/emi/modelemi.h"
#include "engines/grim/costume/chore.h"
#include "engines/grim/costume/head.h"
#include "engines/grim/emi/costume/emianim_component.h"
#include "engines/grim/emi/costume/emimesh_component.h"
#include "engines/grim/emi/costume/emiskel_component.h"
#include "engines/grim/costume/main_model_component.h"

namespace Grim {

CostumeEMI::CostumeEMI(const Common::String &fname, Common::SeekableReadStream *data, Costume *prevCost) :
		Costume(fname, data, prevCost, false), _emiSkel(NULL), _emiMesh(NULL) {

	_fname = fname;
	_lookAtRate = 200;
	_prevCostume = prevCost;

	load(data, prevCost);
}

void CostumeEMI::load(Common::SeekableReadStream *data, Costume *prevCost) {
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

		_chores[i]->setOwner(this);
		_chores[i]->createTracks(data->readUint32LE());

		for (int k = 0; k < _chores[i]->_numTracks; k++) {
			int componentNameLength = data->readUint32LE();

			char *name = new char[componentNameLength];
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
				prevComponent = prevCost->getComponent(0);
				mmc = dynamic_cast<MainModelComponent *>(prevComponent);
				// Make sure that the component is valid
				if (!mmc)
					prevComponent = NULL;
			}
			// Actually load the appropriate component
			Component *component = loadComponent(parentID < 0 ? NULL : _components[parentID], parentID, name, prevComponent);
			if (component) {
				component->setCostume(this);
				component->init();

				if (strcmp(_chores[i]->_name, "wear_default") == 0) {
					EMIMeshComponent *m = dynamic_cast<EMIMeshComponent *>(component);
					EMISkelComponent *s = dynamic_cast<EMISkelComponent *>(component);
					if (m) {
						_emiMesh = m;
						if (_emiSkel) {
							_emiMesh->_obj->setSkeleton(_emiSkel->_obj);
						}
					} else if (s) {
						_emiSkel = s;
						if (_emiMesh) {
							_emiMesh->_obj->setSkeleton(_emiSkel->_obj);
						}
					}
				
				}
			}

			//Component *component = loadComponentEMI(name, parent);

			components.push_back(component);

			ChoreTrack &track = _chores[i]->_tracks[k];
			track.numKeys = data->readUint32LE();
			track.keys = new TrackKey[track.numKeys];
			track.component = component;

			// this is probably wrong
			track.compID = 0;
			for (int j = 0; j < track.numKeys; j++) {
				float time, value;
				data->read(&time, 4);
				data->read(&value, 4);
				track.keys[j].time = (int)time;
				track.keys[j].value = (int)value;
			}
			delete[] name;
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

	}
}

Component *CostumeEMI::loadComponent(Component *parent, int parentID, const char *name, Component *prevComponent) {
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
		//Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement MESH-handling: %s" , name);
		return new EMIMeshComponent(parent, parentID, name, prevComponent, tag);
	} else if (FROM_BE_32(tag) == MKTAG('s','k','e','l')) {
		//Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement SKEL-handling: %s" , name);
		return new EMISkelComponent(parent, parentID, name, prevComponent, tag);
	} else if (FROM_BE_32(tag) == MKTAG('t','e','x','i')) {
		Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement TEXI-handling: %s" , name);
		//return new MaterialComponent(parent, parentID, name, tag);
	} else if (FROM_BE_32(tag) == MKTAG('a','n','i','m')) {
		//Debug::warning(Debug::Costumes, "Actor::loadComponentEMI Implement ANIM-handling: %s" , name);
		return new EMIAnimComponent(parent, parentID, name, prevComponent, tag);
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

void CostumeEMI::draw() {
	for (Common::List<Chore*>::iterator it = _playingChores.begin(); it != _playingChores.end(); ++it) {
		Chore *c = (*it);
		for (int i = 0; i < c->_numTracks; ++i) {
			if (c->_tracks[i].component) {
				c->_tracks[i].component->draw();
			}
		}
	}

	if (_emiMesh) {
		_emiMesh->draw();
	}
}

int CostumeEMI::update(uint time) {
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

void CostumeEMI::setPosRotate(Math::Vector3d pos, const Math::Angle &pitch,
						   const Math::Angle &yaw, const Math::Angle &roll) {
	_matrix.setPosition(pos);
	_matrix.buildFromPitchYawRoll(pitch, yaw, roll);
}

void CostumeEMI::saveState(SaveGame *state) const {
	// TODO
	return;
}

bool CostumeEMI::restoreState(SaveGame *state) {
	// TODO
	return true;
}

} // end of namespace Grim
