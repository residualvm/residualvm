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

#include "math/line3d.h"
#include "math/rect2d.h"

#include "engines/grim/debug.h"
#include "engines/grim/actor.h"
#include "engines/grim/grim.h"
#include "engines/grim/costume.h"
#include "engines/grim/lipsync.h"
#include "engines/grim/movie/movie.h"
#include "engines/grim/sound.h"
#include "engines/grim/lua.h"
#include "engines/grim/resource.h"
#include "engines/grim/savegame.h"
#include "engines/grim/set.h"
#include "engines/grim/gfx_base.h"
#include "engines/grim/model.h"

#include "engines/grim/emi/emi.h"
#include "engines/grim/emi/costumeemi.h"
#include "engines/grim/emi/skeleton.h"
#include "engines/grim/emi/costume/emiskel_component.h"

#include "common/foreach.h"

namespace Grim {

static int animTurn(float turnAmt, const Math::Angle &dest, Math::Angle *cur) {
	Math::Angle d = dest - *cur;
	d.normalize(-180);
	// If the actor won't turn because the rate is set to zero then
	// have the actor turn all the way to the destination yaw.
	// Without this some actors will lock the interface on changing
	// scenes, this affects the Bone Wagon in particular.
	if (turnAmt == 0 || turnAmt >= fabsf(d.getDegrees())) {
		*cur = dest;
	} else if (d > 0) {
		*cur += turnAmt;
	} else {
		*cur -= turnAmt;
	}
	if (d != 0) {
		return (d > 0 ? 1 : -1);
	}
	return 0;
}

bool Actor::_isTalkingBackground = false;

void Actor::saveStaticState(SaveGame *state) {
	state->writeBool(_isTalkingBackground);
}

void Actor::restoreStaticState(SaveGame *state) {
	_isTalkingBackground = state->readBool();
}

Actor::Actor() :
		_talkColor(255, 255, 255), _pos(0, 0, 0),
		_lookingMode(false), _followBoxes(false), _running(false), 
		_pitch(0), _yaw(0), _roll(0), _walkRate(0.3f),
		_turnRateMultiplier(0.f), _talkAnim(0),
		_reflectionAngle(80), _scale(1.f), _timeScale(1.f),
		_visible(true), _lipSync(NULL), _turning(false), _walking(false),
		_walkedLast(false), _walkedCur(false),
		_collisionMode(CollisionOff), _collisionScale(1.f),
		_lastTurnDir(0), _currTurnDir(0),
		_sayLineText(0), _talkDelay(0),
		_attachedActor(0), _attachedJoint(""),
		_globalAlpha(1.f), _alphaMode(AlphaOff),
		 _mustPlaceText(false), 
		_shadowActive(false), _puckOrient(false), _talking(false), 
		_inOverworld(false), _drawnToClean(false), _backgroundTalk(false),
		_sortOrder(0), _haveSectorSortOrder(false), _sectorSortOrder(0),
		_cleanBuffer(0) {

	// Some actors don't set walk and turn rates, so we default the
	// _turnRate so Doug at the cat races can turn and we set the
	// _walkRate so Glottis at the demon beaver entrance can walk and
	// so Chepito in su.set
	_turnRate = 100.0f;

	_activeShadowSlot = -1;
	_shadowArray = new Shadow[5];
	for (int i = 0; i < MAX_SHADOWS; i++) {
		_shadowArray[i].active = false;
		_shadowArray[i].dontNegate = false;
		_shadowArray[i].shadowMask = NULL;
		_shadowArray[i].shadowMaskSize = 0;
	}
}

Actor::~Actor() {
	if (_shadowArray) {
		clearShadowPlanes();
		delete[] _shadowArray;
	}
	while (!_costumeStack.empty()) {
		delete _costumeStack.back();
		_costumeStack.pop_back();
	}
	g_grim->immediatelyRemoveActor(this);

	if (_cleanBuffer) {
		g_driver->delBuffer(_cleanBuffer);
	}
}

void Actor::saveState(SaveGame *savedState) const {
	// store actor name
	savedState->writeString(_name);
	savedState->writeString(_setName);

	savedState->writeColor(_talkColor);

	savedState->writeVector3d(_pos);

	savedState->writeFloat(_pitch.getDegrees());
	savedState->writeFloat(_yaw.getDegrees());
	savedState->writeFloat(_roll.getDegrees());
	savedState->writeFloat(_walkRate);
	savedState->writeFloat(_turnRate);
	savedState->writeFloat(_turnRateMultiplier);
	savedState->writeBool(_followBoxes);
	savedState->writeFloat(_reflectionAngle);
	savedState->writeBool(_visible);
	savedState->writeBool(_lookingMode);
	savedState->writeFloat(_scale);
	savedState->writeFloat(_timeScale);
	savedState->writeBool(_puckOrient);

	savedState->writeString(_talkSoundName);
	savedState->writeBool(_talking);
	savedState->writeBool(_backgroundTalk);

	savedState->writeLEUint32((uint32)_collisionMode);
	savedState->writeFloat(_collisionScale);

	if (_lipSync) {
		savedState->writeBool(true);
		savedState->writeString(_lipSync->getFilename());
	} else {
		savedState->writeBool(false);
	}

	savedState->writeLEUint32(_costumeStack.size());
	for (Common::List<Costume *>::const_iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		savedState->writeString(c->getFilename());
		Costume *pc = c->getPreviousCostume();
		int depth = 0;
		while (pc) {
			++depth;
			pc = pc->getPreviousCostume();
		}
		savedState->writeLESint32(depth);
		pc = c->getPreviousCostume();
		for (int j = 0; j < depth; ++j) { //save the previousCostume hierarchy
			savedState->writeString(pc->getFilename());
			pc = pc->getPreviousCostume();
		}
		c->saveState(savedState);
	}

	savedState->writeBool(_turning);
	savedState->writeFloat(_moveYaw.getDegrees());
	savedState->writeFloat(_movePitch.getDegrees());
	savedState->writeFloat(_moveRoll.getDegrees());

	savedState->writeBool(_walking);
	savedState->writeVector3d(_destPos);

	_restChore.saveState(savedState);

	_walkChore.saveState(savedState);
	savedState->writeBool(_walkedLast);
	savedState->writeBool(_walkedCur);

	_leftTurnChore.saveState(savedState);
	_rightTurnChore.saveState(savedState);
	savedState->writeLESint32(_lastTurnDir);
	savedState->writeLESint32(_currTurnDir);

	for (int i = 0; i < 10; ++i) {
		_talkChore[i].saveState(savedState);
	}
	savedState->writeLESint32(_talkAnim);

	_mumbleChore.saveState(savedState);

	for (int i = 0; i < MAX_SHADOWS; ++i) {
		Shadow &shadow = _shadowArray[i];
		savedState->writeString(shadow.name);

		savedState->writeVector3d(shadow.pos);

		savedState->writeLEUint32(shadow.planeList.size());
		// Cannot use g_grim->getCurrSet() here because an actor can have walk planes
		// from other scenes. It happens e.g. when Membrillo calls Velasco to tell him
		// Naranja is dead.
		for (SectorListType::iterator j = shadow.planeList.begin(); j != shadow.planeList.end(); ++j) {
			Plane &p = *j;
			savedState->writeString(p.setName);
			savedState->writeString(p.sector->getName());
		}

		savedState->writeLESint32(shadow.shadowMaskSize);
		savedState->write(shadow.shadowMask, shadow.shadowMaskSize);
		savedState->writeBool(shadow.active);
		savedState->writeBool(shadow.dontNegate);
	}
	savedState->writeLESint32(_activeShadowSlot);

	savedState->writeLESint32(_sayLineText);

	savedState->writeVector3d(_lookAtVector);

	savedState->writeLEUint32(_path.size());
	for (Common::List<Math::Vector3d>::const_iterator i = _path.begin(); i != _path.end(); ++i) {
		savedState->writeVector3d(*i);
	}

	if (g_grim->getGameType() == GType_MONKEY4) {
		savedState->writeLEUint32(_alphaMode);
		savedState->writeFloat(_globalAlpha);

		savedState->writeBool(_inOverworld);
		savedState->writeLESint32(_sortOrder);
		savedState->writeBool(_shadowActive);

		savedState->writeLESint32(_attachedActor);
		savedState->writeString(_attachedJoint);

		_lastWearChore.saveState(savedState);
	}

	savedState->writeBool(_drawnToClean);
}

bool Actor::restoreState(SaveGame *savedState) {
	for (Common::List<Costume *>::const_iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		delete *i;
	}
	_costumeStack.clear();

	// load actor name
	_name = savedState->readString();
	_setName = savedState->readString();

	_talkColor = savedState->readColor();

	_pos                = savedState->readVector3d();
	_pitch              = savedState->readFloat();
	_yaw                = savedState->readFloat();
	_roll               = savedState->readFloat();
	_walkRate           = savedState->readFloat();
	_turnRate           = savedState->readFloat();
	if (savedState->saveMinorVersion() > 6) {
		_turnRateMultiplier = savedState->readFloat();
	} else {
		_turnRateMultiplier = 1.f;
	}
	_followBoxes        = savedState->readBool();
	_reflectionAngle    = savedState->readFloat();
	_visible            = savedState->readBool();
	_lookingMode        = savedState->readBool();
	_scale              = savedState->readFloat();
	_timeScale          = savedState->readFloat();
	_puckOrient         = savedState->readBool();

	_talkSoundName      = savedState->readString();
	_talking = savedState->readBool();
	_backgroundTalk = savedState->readBool();
	if (isTalking()) {
		g_grim->addTalkingActor(this);
	}

	_collisionMode      = (CollisionMode)savedState->readLEUint32();
	_collisionScale     = savedState->readFloat();

	if (savedState->readBool()) {
		Common::String fn = savedState->readString();
		_lipSync = g_resourceloader->getLipSync(fn);
	} else {
		_lipSync = NULL;
	}

	uint32 size = savedState->readLEUint32();
	for (uint32 i = 0; i < size; ++i) {
		Common::String fname = savedState->readString();
		const int depth = savedState->readLESint32();
		Costume *pc = NULL;
		if (depth > 0) {    //build all the previousCostume hierarchy
			Common::String *names = new Common::String[depth];
			for (int j = 0; j < depth; ++j) {
				names[j] = savedState->readString();
			}
			for (int j = depth - 1; j >= 0; --j) {
				pc = findCostume(names[j]);
				if (!pc) {
					pc = g_resourceloader->loadCostume(names[j], pc);
				}
			}
			delete[] names;
		}

		Costume *c = g_resourceloader->loadCostume(fname, pc);
		c->restoreState(savedState);
		_costumeStack.push_back(c);
	}

	_turning = savedState->readBool();
	_moveYaw = savedState->readFloat();
	if (savedState->saveMinorVersion() > 6) {
		_movePitch = savedState->readFloat();
		_moveRoll = savedState->readFloat();
	} else {
		_movePitch = _pitch;
		_moveRoll = _roll;
	}

	_walking = savedState->readBool();
	_destPos = savedState->readVector3d();

	_restChore.restoreState(savedState, this);

	_walkChore.restoreState(savedState, this);
	_walkedLast = savedState->readBool();
	_walkedCur = savedState->readBool();

	_leftTurnChore.restoreState(savedState, this);
	_rightTurnChore.restoreState(savedState, this);
	_lastTurnDir = savedState->readLESint32();
	_currTurnDir = savedState->readLESint32();

	for (int i = 0; i < 10; ++i) {
		_talkChore[i].restoreState(savedState, this);
	}
	_talkAnim = savedState->readLESint32();

	_mumbleChore.restoreState(savedState, this);

	clearShadowPlanes();
	for (int i = 0; i < MAX_SHADOWS; ++i) {
		Shadow &shadow = _shadowArray[i];
		shadow.name = savedState->readString();

		shadow.pos = savedState->readVector3d();

		size = savedState->readLEUint32();
		Set *scene = NULL;
		for (uint32 j = 0; j < size; ++j) {
			Common::String actorSetName = savedState->readString();
			Common::String secName = savedState->readString();
			if (!scene || scene->getName() != actorSetName) {
				scene = g_grim->findSet(actorSetName);
			}
			if (scene) {
				addShadowPlane(secName.c_str(), scene, i);
			} else {
				warning("%s: No scene \"%s\" found, cannot restore shadow on sector \"%s\"", getName().c_str(), actorSetName.c_str(), secName.c_str());
			}
		}

		shadow.shadowMaskSize = savedState->readLESint32();
		delete[] shadow.shadowMask;
		if (shadow.shadowMaskSize > 0) {
			shadow.shadowMask = new byte[shadow.shadowMaskSize];
			savedState->read(shadow.shadowMask, shadow.shadowMaskSize);
		} else {
			shadow.shadowMask = NULL;
		}
		shadow.active = savedState->readBool();
		shadow.dontNegate = savedState->readBool();
	}
	_activeShadowSlot = savedState->readLESint32();

	_sayLineText = savedState->readLESint32();

	_lookAtVector = savedState->readVector3d();

	size = savedState->readLEUint32();
	_path.clear();
	for (uint32 i = 0; i < size; ++i) {
		_path.push_back(savedState->readVector3d());
	}

	if (g_grim->getGameType() == GType_MONKEY4) {
		_alphaMode = (AlphaMode) savedState->readLEUint32();
		_globalAlpha = savedState->readFloat();

		_inOverworld  = savedState->readBool();
		_sortOrder    = savedState->readLESint32();
		_shadowActive = savedState->readBool();

		_attachedActor = savedState->readLESint32();
		_attachedJoint = savedState->readString();

		// will be recalculated in next update()
		_haveSectorSortOrder = false;
		_sectorSortOrder = 0;

		_lastWearChore.restoreState(savedState, this);
	}

	if (_cleanBuffer) {
		g_driver->delBuffer(_cleanBuffer);
	}
	_cleanBuffer = 0;
	_drawnToClean = false;
	if (savedState->saveMinorVersion() >= 4) {
		bool drawnToClean = savedState->readBool();
		if (drawnToClean) {
			_cleanBuffer = g_driver->genBuffer();
			g_driver->clearBuffer(_cleanBuffer);
		}
	}

	return true;
}

void Actor::setYaw(const Math::Angle &yawParam) {
	_yaw = yawParam;
}

void Actor::setRot(const Math::Vector3d &pos) {
	Math::Angle y, p, r;
	calculateOrientation(pos, &p, &y, &r);
	setRot(p, y, r);
}

void Actor::setRot(const Math::Angle &pitchParam, const Math::Angle &yawParam, const Math::Angle &rollParam) {
	_pitch = pitchParam;
	setYaw(yawParam);
	_moveYaw = _yaw;
	_roll = rollParam;
	_turning = false;
}

void Actor::setPos(const Math::Vector3d &position) {
	_walking = false;
	_pos = position;

	// Don't allow positions outside the sectors.
	// This is necessary after solving the tree pump puzzle, when the bone
	// wagon returns to the signopost set.
	if (_followBoxes) {
		g_grim->getCurrSet()->findClosestSector(_pos, NULL, &_pos);
	}
}

void Actor::calculateOrientation(const Math::Vector3d &pos, Math::Angle *pitch, Math::Angle *yaw, Math::Angle *roll) {
	Math::Vector3d actorForward(0.f, 1.f, 0.f);
	Math::Vector3d actorUp(0.f, 0.f, 1.f);
	Math::Vector3d lookVector = pos - _pos;
	lookVector.normalize();

	// EMI: Y is up-down, actors use an X-Z plane for movement
	if (g_grim->getGameType() == GType_MONKEY4) {
		float temp = lookVector.z();
		lookVector.x() = -lookVector.x();
		lookVector.z() = lookVector.y();
		lookVector.y() = temp;
	}

	Math::Vector3d up = actorUp;
	if (_puckOrient) {
		Sector *s = NULL;
		g_grim->getCurrSet()->findClosestSector(_pos, &s, NULL);
		if (s) {
			up = s->getNormal();
		}
	}

	Math::Matrix3 m;
	m.buildFromTargetDir(actorForward, lookVector, actorUp, up);

	if (_puckOrient) {
		m.getPitchYawRoll(pitch, yaw, roll);
	} else {
		*pitch = _movePitch;
		*yaw = m.getYaw();
		*roll = _moveRoll;
	}
}

void Actor::turnTo(const Math::Vector3d &pos, bool snap) {
	Math::Angle y, p, r;
	calculateOrientation(pos, &p, &y, &r);
	turnTo(p, y, r, snap);
}

bool Actor::singleTurnTo(const Math::Vector3d &pos) {
	Math::Angle y, p, r;
	calculateOrientation(pos, &p, &y, &r);

	float turnAmt = g_grim->getPerSecond(_turnRate);
	bool done = animTurn(turnAmt, y, &_yaw) == 0;
	done = animTurn(turnAmt, p, &_pitch) == 0 && done;
	done = animTurn(turnAmt, r, &_roll) == 0 && done;
	_moveYaw = _yaw;
	_movePitch = _pitch;
	_moveRoll = _roll;

	return done;
}

void Actor::turnTo(const Math::Angle &pitchParam, const Math::Angle &yawParam, const Math::Angle &rollParam, bool snap) {
	_movePitch = pitchParam;
	_moveRoll = rollParam;
	_moveYaw = yawParam;
	_turnRateMultiplier = (snap ? 5.f : 1.f);
	if (_yaw != yawParam || _pitch != pitchParam || _roll != rollParam) {
		_turning = true;
	} else
		_turning = false;
}

void Actor::walkTo(const Math::Vector3d &p) {
	if (p == _pos)
		_walking = false;
	else {
		_walking = true;
		_destPos = p;
		_path.clear();

		if (_followBoxes) {
			Set *currSet = g_grim->getCurrSet();
			currSet->findClosestSector(p, NULL, &_destPos);

			Common::List<PathNode *> openList;
			Common::List<PathNode *> closedList;

			PathNode *start = new PathNode;
			start->parent = NULL;
			start->pos = _pos;
			start->dist = 0.f;
			start->cost = 0.f;
			openList.push_back(start);
			currSet->findClosestSector(_pos, &start->sect, NULL);

			Common::List<Sector *> sectors;
			for (int i = 0; i < currSet->getSectorCount(); ++i) {
				Sector *s = currSet->getSectorBase(i);
				int type = s->getType();
				if ((type == Sector::WalkType || type == Sector::HotType || type == Sector::FunnelType) && s->isVisible()) {
					sectors.push_back(s);
				}
			}

			Sector *endSec = NULL;
			currSet->findClosestSector(_destPos, &endSec, NULL);

			do {
				PathNode *node = NULL;
				float cost = -1.f;
				for (Common::List<PathNode *>::iterator j = openList.begin(); j != openList.end(); ++j) {
					PathNode *n = *j;
					float c = n->dist + n->cost;
					if (cost < 0.f || c < cost) {
						cost = c;
						node = n;
					}
				}
				closedList.push_back(node);
				openList.remove(node);
				Sector *sector = node->sect;

				if (sector == endSec) {
					PathNode *n = closedList.back();
					// Don't put the start position in the list, or else
					// the first angle calculated in updateWalk() will be
					// meaningless. The only node without parent is the start
					// one.
					while (n->parent) {
						_path.push_back(n->pos);
						n = n->parent;
					}

					break;
				}

				for (Common::List<Sector *>::iterator i = sectors.begin(); i != sectors.end(); ++i) {
					Sector *s = *i;
					bool inClosed = false;
					for (Common::List<PathNode *>::iterator j = closedList.begin(); j != closedList.end(); ++j) {
						if ((*j)->sect == s) {
							inClosed = true;
							break;
						}
					}
					if (inClosed)
						continue;

					Common::List<Math::Line3d> bridges = sector->getBridgesTo(s);
					if (bridges.empty())
						continue; // The sectors are not adjacent.

					Math::Vector3d closestPoint = s->getClosestPoint(_destPos);
					Math::Vector3d best;
					float bestDist = 1e6f;
					Math::Line3d l(node->pos, closestPoint);
					while (!bridges.empty()) {
						Math::Line3d bridge = bridges.back();
						Math::Vector3d pos;
						const bool useXZ = (g_grim->getGameType() == GType_MONKEY4);
						if (!bridge.intersectLine2d(l, &pos, useXZ)) {
							pos = bridge.middle();
						}
						float dist = (pos - closestPoint).getMagnitude();
						if (dist < bestDist) {
							bestDist = dist;
							best = pos;
						}
						bridges.pop_back();
					}
					best = handleCollisionTo(node->pos, best);

					PathNode *n = NULL;
					for (Common::List<PathNode *>::iterator j = openList.begin(); j != openList.end(); ++j) {
						if ((*j)->sect == s) {
							n = *j;
							break;
						}
					}
					if (n) {
						float newCost = node->cost + (best - node->pos).getMagnitude();
						if (newCost < n->cost) {
							n->cost = newCost;
							n->parent = node;
							n->pos = best;
							n->dist = (n->pos - _destPos).getMagnitude();
						}
					} else {
						n = new PathNode;
						n->parent = node;
						n->sect = s;
						n->pos = best;
						n->dist = (n->pos - _destPos).getMagnitude();
						n->cost = node->cost + (n->pos - node->pos).getMagnitude();
						openList.push_back(n);
					}
				}
			} while (!openList.empty());

			for (Common::List<PathNode *>::iterator j = closedList.begin(); j != closedList.end(); ++j) {
				delete *j;
			}
			for (Common::List<PathNode *>::iterator j = openList.begin(); j != openList.end(); ++j) {
				delete *j;
			}
		}

		_path.push_front(_destPos);
	}
}

bool Actor::isWalking() const {
	return _walkedLast || _walkedCur || _walking;
}

bool Actor::isTurning() const {
	if (_turning)
		return true;

	if (_lastTurnDir != 0 || _currTurnDir != 0)
		return true;

	return false;
}

void Actor::moveTo(const Math::Vector3d &pos) {
	// The walking actor doesn't always have the collision mode set, but it must however check
	// the collisions. E.g. the set hl doesn't set Manny's mode, but it must check for
	// collisions with Raoul.
	// On the other hand, it must not *set* the sphere mode, it must just use it for this call:
	// the set xb sets Manny's collision scale as 1 and mode as Off. If you then go to set tb
	// and talk to Doug at the photo window, Manny's mode *must be* Off, otherwise Doug will
	// collide, miss the target point and will walk away, leaving Manny waiting for him at
	// the window forever.
	CollisionMode mode = _collisionMode;
	if (_collisionMode == CollisionOff) {
		mode = CollisionSphere;
	}

	Math::Vector3d v = pos - _pos;
	foreach (Actor *a, g_grim->getActiveActors()) {
		handleCollisionWith(a, mode, &v);
	}
	_pos += v;
}

void Actor::walkForward() {
	float dist = g_grim->getPerSecond(_walkRate);
	// Limit the amount of the movement per frame, otherwise with low fps
	// scripts that use WalkActorForward and proximity may break.
	if ((dist > 0 && dist > _walkRate / 5.f) || (dist < 0 && dist < _walkRate / 5.f))
		dist = _walkRate / 5.f;

	_walking = false;

	if (!_followBoxes) {
		Math::Vector3d forwardVec(-_moveYaw.getSine() * _pitch.getCosine(),
								  _moveYaw.getCosine() * _pitch.getCosine(), _pitch.getSine());

		// EMI: Y is up-down, actors use an X-Z plane for movement
		if (g_grim->getGameType() == GType_MONKEY4) {
			float temp = forwardVec.z();
			forwardVec.x() = -forwardVec.x();
			forwardVec.z() = forwardVec.y();
			forwardVec.y() = temp;
		}

		_pos += forwardVec * dist;
		_walkedCur = true;
		return;
	}

	bool backwards = false;
	if (dist < 0) {
		dist = -dist;
		backwards = true;
	}

	int tries = 0;
	while (dist > 0.0f) {
		Sector *currSector = NULL, *prevSector = NULL, *startSector = NULL;
		Sector::ExitInfo ei;

		g_grim->getCurrSet()->findClosestSector(_pos, &currSector, &_pos);
		if (!currSector) { // Shouldn't happen...
			Math::Vector3d forwardVec(-_moveYaw.getSine() * _pitch.getCosine(),
									  _moveYaw.getCosine() * _pitch.getCosine(), _pitch.getSine());

			// EMI: Y is up-down, actors use an X-Z plane for movement
			if (g_grim->getGameType() == GType_MONKEY4) {
				float temp = forwardVec.z();
				forwardVec.x() = -forwardVec.x();
				forwardVec.z() = forwardVec.y();
				forwardVec.y() = temp;
			}

			if (backwards)
				forwardVec = -forwardVec;

			moveTo(_pos + forwardVec * dist);
			_walkedCur = true;
			return;
		}
		startSector = currSector;

		float oldDist = dist;
		while (currSector) {
			prevSector = currSector;
			Math::Vector3d forwardVec;
			const Math::Vector3d &normal = currSector->getNormal();
			if (g_grim->getGameType() == GType_GRIM) {
				Math::Angle ax = Math::Vector2d(normal.x(), normal.z()).getAngle();
				Math::Angle ay = Math::Vector2d(normal.y(), normal.z()).getAngle();

				float z1 = -_moveYaw.getCosine() * (ay - _pitch).getCosine();
				float z2 = _moveYaw.getSine() * (ax - _pitch).getCosine();
				forwardVec = Math::Vector3d(-_moveYaw.getSine() * ax.getSine() * _pitch.getCosine(),
											_moveYaw.getCosine() * ay.getSine() * _pitch.getCosine(), z1 + z2);
			} else {
				Math::Angle ax = Math::Vector2d(normal.x(), normal.y()).getAngle();
				Math::Angle az = Math::Vector2d(normal.z(), normal.y()).getAngle();

				float y1 = _moveYaw.getCosine() * (az - _pitch).getCosine();
				float y2 = _moveYaw.getSine() * (ax - _pitch).getCosine();
				forwardVec = Math::Vector3d(-_moveYaw.getSine() * ax.getSine() * _pitch.getCosine(), y1 + y2,
											-_moveYaw.getCosine() * az.getSine() * _pitch.getCosine());
			}

			if (backwards)
				forwardVec = -forwardVec;

			Math::Vector3d puckVec = currSector->getProjectionToPuckVector(forwardVec);
			puckVec /= puckVec.getMagnitude();
			currSector->getExitInfo(_pos, puckVec, &ei);
			float exitDist = (ei.exitPoint - _pos).getMagnitude();
			if (dist < exitDist) {
				moveTo(_pos + puckVec * dist);
				_walkedCur = true;
				return;
			}
			_pos = ei.exitPoint;
			dist -= exitDist;
			if (exitDist > 0.0001)
				_walkedCur = true;

			// Check for an adjacent sector which can continue
			// the path
			currSector = g_grim->getCurrSet()->findPointSector(ei.exitPoint + (float)0.0001 * puckVec, Sector::WalkType);

			// EMI: some sectors are significantly higher/lower than others.
			if (currSector && g_grim->getGameType() == GType_MONKEY4) {
				float planeDist = currSector->distanceToPoint(_pos);
				if (fabs(planeDist) < 1.f)
					_pos -= planeDist * currSector->getNormal();
			}

			if (currSector == prevSector || currSector == startSector)
				break;
		}

		int turnDir = 1;
		if (ei.angleWithEdge > 90) {
			ei.angleWithEdge = 180 - ei.angleWithEdge;
			ei.edgeDir = -ei.edgeDir;
			turnDir = -1;
		}
		if (ei.angleWithEdge > _reflectionAngle)
			return;

		ei.angleWithEdge += (float)1.0f;

		if (g_grim->getGameType() == GType_MONKEY4) {
			ei.angleWithEdge = -ei.angleWithEdge;
		}
		turnTo(0, _moveYaw + ei.angleWithEdge * turnDir, 0, true);

		if (oldDist <= dist + 0.001f) {
			// If we didn't move at all, keep trying a couple more times
			// in case we can move in the new direction.
			tries++;
			if (tries > 3)
				break;
		}
	}
}

Math::Vector3d Actor::getSimplePuckVector() const {
	if (g_grim->getGameType() == GType_MONKEY4) {
		Math::Angle yaw = 0;
		const Actor *a = this;
		while (a) {
			yaw += a->_yaw;
			if (!a->isAttached())
				break;
			a = Actor::getPool().getObject(a->_attachedActor);
		}
		return Math::Vector3d(yaw.getSine(), 0, yaw.getCosine());
	} else {
		return Math::Vector3d(-_yaw.getSine(), _yaw.getCosine(), 0);
	}
}

Math::Vector3d Actor::getPuckVector() const {
	Math::Vector3d forwardVec = getSimplePuckVector();

	Set *currSet = g_grim->getCurrSet();
	if (!currSet)
		return forwardVec;

	Sector *sector = currSet->findPointSector(_pos, Sector::WalkType);
	if (!sector)
		return forwardVec;
	else
		return sector->getProjectionToPuckVector(forwardVec);
}

void Actor::setPuckOrient(bool orient) {
	_puckOrient = orient;
	warning("Actor::setPuckOrient() not implemented");
}

void Actor::setRestChore(int chore, Costume *cost) {
	if (_restChore.equals(cost, chore))
		return;

	_restChore.stop(true);

	if (!cost) {
		cost = _restChore._costume;
	}
	if (!cost) {
		cost = getCurrentCostume();
	}

	_restChore = Chore(cost, chore);

	_restChore.playLooping(true);
}

int Actor::getRestChore() const {
	return _restChore._chore;
}

void Actor::setWalkChore(int chore, Costume *cost) {
	if (_walkChore.equals(cost, chore))
		return;

	if (_walkedLast && _walkChore.isPlaying()) {
		_walkChore.stop(true);

		_restChore.playLooping(true);
	}

	if (!cost) {
		cost = _walkChore._costume;
	}
	if (!cost) {
		cost = getCurrentCostume();
	}

	_walkChore = Chore(cost, chore);
}

void Actor::setTurnChores(int left_chore, int right_chore, Costume *cost) {
	if (_leftTurnChore.equals(cost, left_chore) && _rightTurnChore.equals(cost, right_chore))
		return;

	if (!cost) {
		cost = _leftTurnChore._costume;
	}
	if (!cost) {
		cost = getCurrentCostume();
	}

	_leftTurnChore.stop(true);
	_rightTurnChore.stop(true);
	_lastTurnDir = 0;

	_leftTurnChore = Chore(cost, left_chore);
	_rightTurnChore = Chore(cost, right_chore);

	if ((left_chore >= 0 && right_chore < 0) || (left_chore < 0 && right_chore >= 0))
		error("Unexpectedly got only one turn chore");
}

void Actor::setTalkChore(int index, int chore, Costume *cost) {
	if (index < 1 || index > 10)
		error("Got talk chore index out of range (%d)", index);

	index--;

	if (!cost) {
		cost = _talkChore[index]._costume;
	}
	if (!cost) {
		cost = getCurrentCostume();
	}

	if (_talkChore[index].equals(cost, chore))
		return;

	_talkChore[index].stop();

	_talkChore[index] = Chore(cost, chore);
}

int Actor::getTalkChore(int index) const {
	return _talkChore[index]._chore;
}

Costume *Actor::getTalkCostume(int index) const {
	return _talkChore[index]._costume;
}

void Actor::setMumbleChore(int chore, Costume *cost) {
	if (_mumbleChore.isPlaying()) {
		_mumbleChore.stop();
	}

	if (!cost) {
		cost = _mumbleChore._costume;
	}
	if (!cost) {
		cost = getCurrentCostume();
	}

	_mumbleChore = Chore(cost, chore);
}

bool Actor::playLastWearChore() {
	if (!_lastWearChore.isValid())
		return false;

	_lastWearChore.playLooping(false, 0);
	return true;
}

void Actor::setLastWearChore(int chore, Costume *cost) {
	if (! _costumeStack.empty() && cost == _costumeStack.back()) {
		_lastWearChore = Chore(cost, chore);
	}
}

void Actor::turn(int dir) {
	_walking = false;
	float delta = g_grim->getPerSecond(_turnRate) * dir;
	if (g_grim->getGameType() == GType_MONKEY4) {
		delta = -delta;
	}
	_moveYaw = _moveYaw + delta;
	_turning = true;
	_turnRateMultiplier = 5.f;
	_currTurnDir = dir;
}

Math::Angle Actor::getYawTo(const Actor *a) const {
	Math::Vector3d forwardVec = getSimplePuckVector();
	Math::Vector3d delta = a->getWorldPos() - getWorldPos();

	if (g_grim->getGameType() == GType_MONKEY4) {
		delta.y() = 0;
	} else {
		delta.z() = 0;
	}

	return Math::Vector3d::angle(forwardVec, delta);
}

Math::Angle Actor::getYawTo(const Math::Vector3d &p) const {
	Math::Vector3d dpos = p - _pos;

	if (g_grim->getGameType() == GType_MONKEY4) {
		dpos.y() = dpos.z();
	}
	if (dpos.x() == 0 && dpos.y() == 0)
		return 0;
	else
		return Math::Angle::arcTangent2(-dpos.x(), dpos.y());
}

void Actor::sayLine(const char *msgId, bool background) {
	assert(msgId);

	if (msgId[0] == 0) {
		warning("Actor::sayLine: Empty message");
		return;
	}

	char id[50];
	Common::String msg = LuaBase::instance()->parseMsgText(msgId, id);

	if (id[0] == 0) {
		error("Actor::sayLine: No message ID for text");
		return;
	}

	// During Fullscreen movies SayLine is usually called for text display only.
	// The movie with Charlie screaming after Manny put the sheet on him instead
	// uses sayLine for the voice too.
	// However, normal SMUSH movies may call SayLine, for example:
	// When Domino yells at Manny (a SMUSH movie) he does it with
	// a SayLine request rather than as part of the movie!

	Common::String soundName = id;

	if (g_grim->getGameType() == GType_GRIM) {
		soundName += ".wav";
	} else if (g_grim->getGameType() == GType_MONKEY4 && g_grim->getGamePlatform() == Common::kPlatformPS2) {
		soundName += ".scx";
	} else {
		soundName += ".wVC";
	}
	if (_talkSoundName == soundName)
		return;

	if (_talking || msg.empty())
		shutUp();

	_talkSoundName = soundName;

	Set *currSet = g_grim->getCurrSet();

	if (g_grim->getSpeechMode() != GrimEngine::TextOnly) {
		// if there is no costume probably the character is drawn by a smush movie, so
		// we don't want to go out of sync with it.
		if (getCurrentCostume()) {
			_talkDelay = 500;
		}
		if (g_sound->startVoice(_talkSoundName.c_str()) && currSet) {
			currSet->setSoundPosition(_talkSoundName.c_str(), _pos);
		}
	}

	// If the actor is clearly not visible then don't try to play the lip sync
	if (_visible && (!g_movie->isPlaying() || g_grim->getMode() == GrimEngine::NormalMode)) {
		Common::String soundLip = id;
		soundLip += ".lip";

		if (!_talkChore[0].isPlaying()) {
			// _talkChore[0] is *_stop_talk
			_talkChore[0].setLastFrame();
		}
		// Sometimes actors speak offscreen before they, including their
		// talk chores are initialized.
		// For example, when reading the work order (a LIP file exists for no reason).
		// Also, some lip sync files have no entries
		// In these cases, revert to using the mumble chore.
		if (g_grim->getSpeechMode() != GrimEngine::TextOnly)
			_lipSync = g_resourceloader->getLipSync(soundLip);
		// If there's no lip sync file then load the mumble chore if it exists
		// (the mumble chore doesn't exist with the cat races announcer)
		if (!_lipSync)
			_mumbleChore.playLooping();

		_talkAnim = -1;
	}

	_talking = true;
	g_grim->addTalkingActor(this);

	_backgroundTalk = background;
	if (background)
		_isTalkingBackground = true;

	if (_sayLineText && g_grim->getMode() != GrimEngine::SmushMode) {
		delete TextObject::getPool().getObject(_sayLineText);
		_sayLineText = 0;
	}

	if (!msg.empty()) {
		GrimEngine::SpeechMode m = g_grim->getSpeechMode();
		if (!g_grim->_sayLineDefaults.getFont() || m == GrimEngine::VoiceOnly)
			return;

		if (background) {
			// if we're talking background draw the text object only if there are no no-background
			// talking actors. This prevents glottis and nick subtitles overlapping in the high roller lounge,
			// where glottis is background and nick isn't. (https://github.com/residualvm/residualvm/issues/685)
			foreach (Actor *a, g_grim->getTalkingActors()) {
				if (!a->_backgroundTalk && a->_sayLineText) {
					return;
				}
			}
		} else {
			// if we're not background then delete the TextObject of any talking background actor.
			foreach (Actor *a, g_grim->getTalkingActors()) {
				if (a->_backgroundTalk && a->_sayLineText) {
					delete TextObject::getPool().getObject(a->_sayLineText);
					a->_sayLineText = 0;
				}
			}
		}

		TextObject *textObject = new TextObject();
		textObject->setDefaults(&g_grim->_sayLineDefaults);
		textObject->setFGColor(_talkColor);
		textObject->setIsSpeech();
		if (m == GrimEngine::TextOnly || g_grim->getMode() == GrimEngine::SmushMode) {
			textObject->setDuration(500 + msg.size() * 15 * (11 - g_grim->getTextSpeed()));
		}
		if (g_grim->getMode() == GrimEngine::SmushMode) {
			textObject->setX(640 / 2);
			textObject->setY(456);
			g_grim->setMovieSubtitle(textObject);
		} else {
			if (_visible && isInSet(currSet->getName())) {
				_mustPlaceText = true;
			} else {
				_mustPlaceText = false;
				textObject->setX(640 / 2);
				textObject->setY(463);
			}
		}
		textObject->setText(msgId);
		if (g_grim->getMode() != GrimEngine::SmushMode)
			_sayLineText = textObject->getId();
	}
}

void Actor::lineCleanup() {
	if (_sayLineText) {
		delete TextObject::getPool().getObject(_sayLineText);
		_sayLineText = 0;
	}
}

bool Actor::isTalking() {
	// This should return if the actor is actually talking, disregarding of the background status,
	// otherwise when Naranja is asleep Toto's lines will be cut sometimes. Naranja and Toto both
	// talk in background, and when Naranja lines stop toto:wait_for_message() should not return
	// until he actually stops talking.
	return _talking;
}

bool Actor::isTalkingForeground() const {
	if (!_talking) {
		return false;
	}

	if (_backgroundTalk)
		return _isTalkingBackground;

	return true;
}

void Actor::shutUp() {
	// While the call to stop the sound is usually made by the game,
	// we also need to handle when the user terminates the dialog.
	// Some warning messages will occur when the user terminates the
	// actor dialog but the game will continue alright.
	if (_talkSoundName != "") {
		g_sound->stopSound(_talkSoundName.c_str());
		_talkSoundName = "";
	}

	if (_lipSync) {
		if (_talkAnim != -1)
			_talkChore[_talkAnim].stop();
		_lipSync = NULL;
	}
	// having a lipsync is no guarantee the mumble chore is no running. the talk chores may be -1 (domino in do)
	stopMumbleChore();
	stopTalking();

	if (_sayLineText) {
		delete TextObject::getPool().getObject(_sayLineText);
		_sayLineText = 0;
	}

	// The actors talking in background have a special behaviour: if there are two or more of them
	// talking at the same time, after one of them finishes talking calling isTalking() an *all*
	// of them must return false. This is necessary for the angelitos in set fo: when they start crying
	// they talk in background, and the lua script that hangs on IsMessageGoing() must return before they
	// stop, since they can go on forever.
	if (_backgroundTalk)
		_isTalkingBackground = false;

	_talking = false;
}

void Actor::pushCostume(const char *n) {
	Costume *c = findCostume(n);
	if (c) {
		Debug::debug(Debug::Actors, "Trying to push a costume already in the stack. %s, %s", getName().c_str(), n);
		return;
	}

	Costume *newCost = g_resourceloader->loadCostume(n, getCurrentCostume());

	newCost->setColormap(NULL);
	if (Common::String("fx/dumbshadow.cos").equals(n))
		_costumeStack.push_front(newCost);
	else
		_costumeStack.push_back(newCost);
}

void Actor::setColormap(const char *map) {
	if (!_costumeStack.empty()) {
		Costume *cost = _costumeStack.back();
		cost->setColormap(map);
	} else {
		warning("Actor::setColormap: No costumes");
	}
}

void Actor::setCostume(const char *n) {
	if (!_costumeStack.empty())
		popCostume();

	pushCostume(n);
}

void Actor::popCostume() {
	if (!_costumeStack.empty()) {
		freeCostumeChore(_costumeStack.back(), &_restChore);
		freeCostumeChore(_costumeStack.back(), &_walkChore);

		if (_leftTurnChore._costume == _costumeStack.back()) {
			_leftTurnChore = Chore();
			_rightTurnChore = Chore();
		}

		freeCostumeChore(_costumeStack.back(), &_mumbleChore);
		for (int i = 0; i < 10; i++)
			freeCostumeChore(_costumeStack.back(), &_talkChore[i]);
		delete _costumeStack.back();
		_costumeStack.pop_back();

		if (_costumeStack.empty()) {
			Debug::debug(Debug::Actors, "Popped (freed) the last costume for an actor.\n");
		}
	} else {
		Debug::warning(Debug::Actors, "Attempted to pop (free) a costume when the stack is empty!");
	}
}

void Actor::clearCostumes() {
	// Make sure to destroy costume copies in reverse order
	while (!_costumeStack.empty())
		popCostume();
}

void Actor::setHead(int joint1, int joint2, int joint3, float maxRoll, float maxPitch, float maxYaw) {
	if (!_costumeStack.empty()) {
		_costumeStack.back()->setHead(joint1, joint2, joint3, maxRoll, maxPitch, maxYaw);
	}
}

void Actor::setLookAtRate(float rate) {
	_costumeStack.back()->setLookAtRate(rate);
}

float Actor::getLookAtRate() const {
	return _costumeStack.back()->getLookAtRate();
}

Costume *Actor::findCostume(const Common::String &n) {
	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		if ((*i)->getFilename().compareToIgnoreCase(n) == 0)
			return *i;
	}

	return NULL;
}

void Actor::updateWalk() {
	if (_path.empty()) {
		return;
	}

	Math::Vector3d destPos = _path.back();
	Math::Vector3d dir = destPos - _pos;
	float dist = dir.getMagnitude();

	_walkedCur = true;
	float walkAmt = g_grim->getPerSecond(_walkRate);
	if (walkAmt >= dist) {
		walkAmt = dist;
		_path.pop_back();
		if (_path.empty()) {
			_walking = false;
			_pos = destPos;
// It seems that we need to allow an already active turning motion to
// continue or else turning actors away from barriers won't work right
			_turning = false;
			return;
		}
	}

	turnTo(destPos, true);

	dir = destPos - _pos;
	dir.normalize();
	_pos += dir * walkAmt;
}

void Actor::update(uint frameTime) {
	Set *set = g_grim->getCurrSet();
	// Snap actor to walkboxes if following them.  This might be
	// necessary for example after activating/deactivating
	// walkboxes, etc.
	if (_followBoxes && !_walking) {
		set->findClosestSector(_pos, NULL, &_pos);
	}

	if (g_grim->getGameType() == GType_MONKEY4) {
		// Check for sort order information in the current sector
		int oldSortOrder = getEffectiveSortOrder();

		Sector *sect = set->findPointSector(_pos, Sector::WalkType);
		int setup = set->getSetup();
		if (sect && setup < sect->getNumSortplanes()) {
			_haveSectorSortOrder = true;
			_sectorSortOrder = sect->getSortplane(setup);
		} else {
			_haveSectorSortOrder = false;
		}

		if (oldSortOrder != getEffectiveSortOrder())
			g_emi->invalidateSortOrder();
	}

	if (_turning) {
		float turnAmt = g_grim->getPerSecond(_turnRate) * _turnRateMultiplier;
		_currTurnDir = animTurn(turnAmt, _moveYaw, &_yaw);
		if (g_grim->getGameType() == GType_MONKEY4) {
			_currTurnDir = -_currTurnDir;
		}
		int p = animTurn(turnAmt, _movePitch, &_pitch);
		int r = animTurn(turnAmt, _moveRoll, &_roll);
		if (_currTurnDir == 0 && p == 0 && r == 0) {
			_turning = false;
			_turnRateMultiplier = 1.f;
		}
	}

	if (_walking) {
		updateWalk();
	}

	if (_walkChore.isValid()) {
		if (_walkedCur) {
			if (!_walkChore.isPlaying()) {
				_walkChore.playLooping(true);
			}
			if (_restChore.isPlaying()) {
				_restChore.stop(true);
			}
		} else {
			if (_walkedLast && _walkChore.isPlaying()) {
				_walkChore.stop(true);
				if (!_restChore.isPlaying()) {
					_restChore.playLooping(true);
				}
			}
		}
	}

	if (_leftTurnChore.isValid()) {
		if (_walkedCur || _walkedLast)
			_currTurnDir = 0;

		if (_restChore.isValid()) {
			if (_currTurnDir != 0) {
				if (getTurnChore(_currTurnDir)->isPlaying() && _restChore.isPlaying()) {
					_restChore.stop(true, 500);
				}
			} else if (_lastTurnDir != 0) {
				if (!_walkedCur && getTurnChore(_lastTurnDir)->isPlaying()) {
					_restChore.playLooping(true);
				}
			}
		}

		if (_lastTurnDir != 0 && _lastTurnDir != _currTurnDir) {
			getTurnChore(_lastTurnDir)->stop(true);
		}
		if (_currTurnDir != 0 && _currTurnDir != _lastTurnDir) {
			getTurnChore(_currTurnDir)->playLooping(true, 500);
			if (_currTurnDir == 1) {
				// When turning to the left, ensure that the components of the right turn chore
				// are fading out (or stopped).
				// This is necessary because the left turn chore typically contains both the
				// left turn and right turn keyframe components. The above call to playLooping
				// will thus start fading in both of the components, overriding the right turn's
				// fade out that was started before.
				// The left turn chore's keys will eventually stop the right turn keyframe from
				// playing, but the stopping will be instantaneous. To get a smooth transition,
				// we want to keep fading out the right turn. The chore's "stop" key will be
				// ignored when the keyframe is fading out (see KeyframeComponent::stop()).
				_rightTurnChore.stop(true);
			}
		}
	} else {
		_currTurnDir = 0;
	}

	// The rest chore might have been stopped because of a
	// StopActorChore(nil).  Restart it if so.
	if (!_walkedCur && _currTurnDir == 0 && !_restChore.isPlaying()) {
		_restChore.playLooping(true);
	}

	_walkedLast = _walkedCur;
	_walkedCur = false;
	_lastTurnDir = _currTurnDir;
	_currTurnDir = 0;

	// Update lip syncing
	if (_lipSync) {
		int posSound;

		// While getPosIn16msTicks will return "-1" to indicate that the
		// sound is no longer playing, it is more appropriate to check first
		if (g_grim->getSpeechMode() != GrimEngine::TextOnly && g_sound->getSoundStatus(_talkSoundName.c_str()))
			posSound = g_sound->getPosIn16msTicks(_talkSoundName.c_str());
		else
			posSound = -1;
		if (posSound != -1) {
			int anim = _lipSync->getAnim(posSound);
			if (_talkAnim != anim) {
				if (anim != -1) {
					if (_talkChore[anim].isValid()) {
						stopMumbleChore();
						if (_talkAnim != -1) {
							_talkChore[_talkAnim].stop();
						}

						// Run the stop_talk chore so that it resets the components
						// to the right visibility.
						stopTalking();
						_talkAnim = anim;
						_talkChore[_talkAnim].play();
					} else if (_mumbleChore.isValid() && !_mumbleChore.isPlaying()) {
						_mumbleChore.playLooping();
						_talkAnim = -1;
					}
				} else {
					stopMumbleChore();
					if (_talkAnim != -1)
						_talkChore[_talkAnim].stop();

					_talkAnim = 0;
					stopTalking();
				}
			}
		}
	}

	frameTime = (uint)(frameTime * _timeScale);
	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		c->setPosRotate(_pos, _pitch, _yaw, _roll);
		int marker = c->update(frameTime);
		if (marker > 0) {
			costumeMarkerCallback(marker);
		}
	}

	Costume *c = getCurrentCostume();
	if (c) {
		c->animate();
	}

	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		(*i)->moveHead(_lookingMode, _lookAtVector);
	}
}

// Not all the talking actors are in the current set, and so on not all the talking actors
// update() is called. For example, Don when he comes out of his office after reaping Meche.
bool Actor::updateTalk(uint frameTime) {
	if (_talking) {
		// If there's no sound file then we're obviously not talking
		GrimEngine::SpeechMode m = g_grim->getSpeechMode();
		TextObject *textObject = NULL;
		if (_sayLineText)
			textObject = TextObject::getPool().getObject(_sayLineText);
		if (m == GrimEngine::TextOnly && !textObject) {
			shutUp();
			return false;
		} else if (m != GrimEngine::TextOnly && (_talkSoundName.empty() || !g_sound->getSoundStatus(_talkSoundName.c_str()))) {
			_talkDelay -= frameTime;
			if (_talkDelay <= 0) {
				_talkDelay = 0;
				shutUp();
				return false;
			}
		}
		return true;
	}

	return false;
}

void Actor::draw() {
	for (Common::List<Costume *>::iterator i = _costumeStack.begin(); i != _costumeStack.end(); ++i) {
		Costume *c = *i;
		c->setupTextures();
	}

	if (!g_driver->isHardwareAccelerated() && g_grim->getFlagRefreshShadowMask()) {
		for (int l = 0; l < MAX_SHADOWS; l++) {
			if (!_shadowArray[l].active)
				continue;
			g_driver->setShadow(&_shadowArray[l]);
			g_driver->drawShadowPlanes();
			g_driver->setShadow(NULL);
		}
	}

	// FIXME: if isAttached(), factor in the joint rotation as well.
	const Math::Vector3d &absPos = getWorldPos();
	if (!_costumeStack.empty()) {
		g_grim->getCurrSet()->setupLights(absPos);
		if (g_grim->getGameType() == GType_GRIM) {
			Costume *costume = _costumeStack.back();
			drawCostume(costume);
		} else {
			for (Common::List<Costume *>::iterator it = _costumeStack.begin(); it != _costumeStack.end(); ++it) {
				Costume *costume = *it;
				drawCostume(costume);
			}
		}
	}

	if (_mustPlaceText) {
		int x1, y1, x2, y2;
		x1 = y1 = 1000;
		x2 = y2 = -1000;
		if (!_costumeStack.empty()) {
			g_driver->startActorDraw(this);
			_costumeStack.back()->getBoundingBox(&x1, &y1, &x2, &y2);
			g_driver->finishActorDraw();
		}

		TextObject *textObject = TextObject::getPool().getObject(_sayLineText);
		if (textObject) {
			if (x1 == 1000 || x2 == -1000 || y2 == -1000) {
				textObject->setX(640 / 2);
				textObject->setY(463);
			} else {
				textObject->setX((x1 + x2) / 2);
				textObject->setY(y1);
			}
			textObject->reset();
		}
		_mustPlaceText = false;
	}

	_drawnToClean = false;
}

void Actor::drawCostume(Costume *costume) {
	for (int l = 0; l < MAX_SHADOWS; l++) {
		if (!shouldDrawShadow(l))
			continue;
		g_driver->setShadow(&_shadowArray[l]);
		g_driver->setShadowMode();
		if (g_driver->isHardwareAccelerated())
			g_driver->drawShadowPlanes();
		g_driver->startActorDraw(this);
		costume->draw();
		g_driver->finishActorDraw();
		g_driver->clearShadowMode();
		g_driver->setShadow(NULL);
	}

	bool isShadowCostume = costume->getFilename().equals("fx/dumbshadow.cos");
	if (!isShadowCostume || (isShadowCostume && _costumeStack.size() > 1 && _shadowActive)) {
		// normal draw actor
		g_driver->startActorDraw(this);
		costume->draw();
		g_driver->finishActorDraw();
	}
}

void Actor::setShadowPlane(const char *n) {
	assert(_activeShadowSlot != -1);

	_shadowArray[_activeShadowSlot].name = n;
}

void Actor::addShadowPlane(const char *n, Set *scene, int shadowId) {
	assert(shadowId != -1);

	Sector *sector = scene->getSectorBySubstring(n);
	if (sector) {
		// Create a copy so we are sure it will not be deleted by the Set destructor
		// behind our back. This is important when Membrillo phones Velasco to tell him
		// Naranja is dead, because the scene changes back and forth few times and so
		// the scenes' sectors are deleted while they are still keeped by the actors.
		Plane p = { scene->getName(), new Sector(*sector) };
		_shadowArray[shadowId].planeList.push_back(p);
		g_grim->flagRefreshShadowMask(true);
	}
}

bool Actor::shouldDrawShadow(int shadowId) {
	Shadow *shadow = &_shadowArray[shadowId];
	if (!shadow->active)
		return false;

	// Don't draw a shadow if the shadow caster and the actor are on different sides
	// of the the shadow plane.
	Sector *sector = shadow->planeList.front().sector;
	Math::Vector3d n = sector->getNormal();
	Math::Vector3d p = sector->getVertices()[0];
	float d = -(n.x() * p.x() + n.y() * p.y() + n.z() * p.z());

	p = getPos();
	// Move the tested point a bit above ground level.
	if (g_grim->getGameType() == GType_MONKEY4)
		p.y() += 0.01f;
	else
		p.z() += 0.01f;
	bool actorSide = n.x() * p.x() + n.y() * p.y() + n.z() * p.z() + d < 0.f;
	p = shadow->pos;
	bool shadowSide = n.x() * p.x() + n.y() * p.y() + n.z() * p.z() + d < 0.f;

	if (actorSide == shadowSide)
		return true;
	return false;
}

void Actor::addShadowPlane(const char *n) {
	addShadowPlane(n, g_grim->getCurrSet(), _activeShadowSlot);
}

void Actor::setActiveShadow(int shadowId) {
	assert(shadowId >= 0 && shadowId <= 4);

	_activeShadowSlot = shadowId;
	_shadowArray[_activeShadowSlot].active = true;
}

void Actor::setShadowValid(int valid) {
	if (valid == -1)
		_shadowArray[_activeShadowSlot].dontNegate = true;
	else
		_shadowArray[_activeShadowSlot].dontNegate = false;
}

void Actor::setActivateShadow(int shadowId, bool state) {
	assert(shadowId >= 0 && shadowId <= 4);

	_shadowArray[shadowId].active = state;
}

void Actor::setShadowPoint(const Math::Vector3d &p) {
	assert(_activeShadowSlot != -1);

	_shadowArray[_activeShadowSlot].pos = p;
}

void Actor::clearShadowPlanes() {
	for (int i = 0; i < MAX_SHADOWS; i++) {
		Shadow *shadow = &_shadowArray[i];
		while (!shadow->planeList.empty()) {
			delete shadow->planeList.back().sector;
			shadow->planeList.pop_back();
		}
		delete[] shadow->shadowMask;
		shadow->shadowMaskSize = 0;
		shadow->shadowMask = NULL;
		shadow->active = false;
		shadow->dontNegate = false;
	}
}

void Actor::putInSet(const Common::String &set) {
	// The set should change immediately, otherwise a very rapid set change
	// for an actor will be recognized incorrectly and the actor will be lost.
	_setName = set;

	// clean the buffer. this is needed when an actor goes from frozen state to full model rendering
	if (_setName != "" && _cleanBuffer) {
		g_driver->clearBuffer(_cleanBuffer);
	}

	g_grim->invalidateActiveActorsList();
}

bool Actor::isInSet(const Common::String &set) const {
	return _setName == set;
}

void Actor::freeCostumeChore(const Costume *toFree, Chore *chore) {
	if (chore->_costume == toFree) {
		*chore = Chore();
	}
}

void Actor::stopTalking() {
	// _talkChore[0] is *_stop_talk
	// Don't playLooping it, or else manny's mouth will flicker when he smokes.
	_talkChore[0].setLastFrame();
}

bool Actor::stopMumbleChore() {
	if (_mumbleChore.isPlaying()) {
		_mumbleChore.stop();
		return true;
	}

	return false;
}

void Actor::setCollisionMode(CollisionMode mode) {
	_collisionMode = mode;
}

void Actor::setCollisionScale(float scale) {
	_collisionScale = scale;
}

Math::Vector3d Actor::handleCollisionTo(const Math::Vector3d &from, const Math::Vector3d &pos) const {
	if (_collisionMode == CollisionOff) {
		return pos;
	}

	Math::Vector3d p = pos;
	foreach (Actor *a, Actor::getPool()) {
		if (a != this && a->isInSet(_setName) && a->isVisible()) {
			p = a->getTangentPos(from, p);
		}
	}
	return p;
}

Math::Vector3d Actor::getTangentPos(const Math::Vector3d &pos, const Math::Vector3d &dest) const {
	if (_collisionMode == CollisionOff) {
		return dest;
	}

	Model *model = getCurrentCostume()->getModel();
	Math::Vector3d p = _pos + model->_insertOffset;
	float size = model->_radius * _collisionScale;

	Math::Vector2d p1(pos.x(), pos.y());
	Math::Vector2d p2(dest.x(), dest.y());
	Math::Segment2d segment(p1, p2);

	// TODO: collision with Box
//  if (_collisionMode == CollisionSphere) {
	Math::Vector2d center(p.x(), p.y());

	Math::Vector2d inter;
	float distance = segment.getLine().getDistanceTo(center, &inter);

	if (distance < size && segment.containsPoint(inter)) {
		Math::Vector2d v(inter - center);
		v.normalize();
		v *= size;
		v += center;

		return Math::Vector3d(v.getX(), v.getY(), dest.z());
	}
//  } else {

//  }

	return dest;
}

bool Actor::handleCollisionWith(Actor *actor, CollisionMode mode, Math::Vector3d *vec) const {
	if (actor->_collisionMode == CollisionOff || actor == this) {
		return false;
	}

	if (!actor->getCurrentCostume()) {
		return false;
	}

	Model *model1 = getCurrentCostume()->getModel();
	Model *model2 = actor->getCurrentCostume()->getModel();

	Math::Vector3d p1 = _pos + model1->_insertOffset;
	Math::Vector3d p2 = actor->_pos + model2->_insertOffset;

	float size1 = model1->_radius * _collisionScale;
	float size2 = model2->_radius * actor->_collisionScale;

	CollisionMode mode1 = mode;
	CollisionMode mode2 = actor->_collisionMode;

	if (mode1 == CollisionSphere && mode2 == CollisionSphere) {
		// center the sphere on the model center.
		p1.z() += model1->_bboxSize.z() / 2.f;
		// you may ask: why center the sphere of the first actor only? because it seems the original does so.
		// if you change this code test this places: the rocks in lb and bv (both when booting directly in the
		// set and when coming in from another one) and the poles in xb.

		Math::Vector3d pos = p1 + *vec;
		float distance = (pos - p2).getMagnitude();
		if (distance < size1 + size2) {
			// Move the destination point so that its distance from the
			// center of the circle is size1+size2.
			Math::Vector3d v = pos - p2;
			v.normalize();
			v *= size1 + size2;
			*vec = v + p2 - p1;

			collisionHandlerCallback(actor);
			return true;
		}
	} else if (mode1 == CollisionBox && mode2 == CollisionBox) {
		warning("Collision between box and box not implemented!");
		return false;
	} else {
		Math::Rect2d rect;

		Math::Vector3d bboxPos;
		Math::Vector3d size;
		float scale;
		Math::Vector3d pos;
		Math::Vector3d circlePos;
		Math::Angle yaw;

		Math::Vector2d circle;
		float radius;

		if (mode1 == CollisionBox) {
			pos = p1 + *vec;
			bboxPos = pos + model1->_bboxPos;
			size = model1->_bboxSize;
			scale = _collisionScale;
			yaw = _yaw;

			circle.setX(p2.x());
			circle.setY(p2.y());
			circlePos = p2;
			radius = size2;
		} else {
			pos = p2;
			bboxPos = p2 + model2->_bboxPos;
			size = model2->_bboxSize;
			scale = actor->_collisionScale;
			yaw = actor->_yaw;

			circle.setX(p1.x() + vec->x());
			circle.setY(p1.y() + vec->y());
			circlePos = p1;
			radius = size1;
		}

		rect._topLeft = Math::Vector2d(bboxPos.x(), bboxPos.y() + size.y());
		rect._topRight = Math::Vector2d(bboxPos.x() + size.x(), bboxPos.y() + size.y());
		rect._bottomLeft = Math::Vector2d(bboxPos.x(), bboxPos.y());
		rect._bottomRight = Math::Vector2d(bboxPos.x() + size.x(), bboxPos.y());

		rect.scale(scale);
		rect.rotateAround(Math::Vector2d(pos.x(), pos.y()), yaw);

		if (rect.intersectsCircle(circle, radius)) {
			Math::Vector2d center = rect.getCenter();
			// Draw a line from the center of the rect to the place the character
			// would go to.
			Math::Vector2d v = circle - center;
			v.normalize();

			Math::Segment2d edge;
			// That line intersects (usually) an edge
			rect.getIntersection(center, v, &edge);
			// Take the perpendicular of that edge
			Math::Line2d perpendicular = edge.getPerpendicular(circle);

			Math::Vector3d point;
			Math::Vector2d p;
			// If that perpendicular intersects the edge
			if (edge.intersectsLine(perpendicular, &p)) {
				Math::Vector2d direction = perpendicular.getDirection();
				direction.normalize();

				// Move from the intersection until we are at a safe distance
				Math::Vector2d point1(p - direction * radius);
				Math::Vector2d point2(p + direction * radius);

				if (center.getDistanceTo(point1) < center.getDistanceTo(point2)) {
					point = point2.toVector3d();
				} else {
					point = point1.toVector3d();
				}
			} else { //if not we're around a corner
				// Find the nearest vertex of the rect
				Math::Vector2d vertex = rect.getTopLeft();
				float distance = vertex.getDistanceTo(circle);

				Math::Vector2d other = rect.getTopRight();
				float otherDist = other.getDistanceTo(circle);
				if (otherDist < distance) {
					distance = otherDist;
					vertex = other;
				}

				other = rect.getBottomLeft();
				otherDist = other.getDistanceTo(circle);
				if (otherDist < distance) {
					distance = otherDist;
					vertex = other;
				}

				other = rect.getBottomRight();
				if (other.getDistanceTo(circle) < distance) {
					vertex = other;
				}

				// and move on a circle around it
				Math::Vector2d dst = circle - vertex;
				dst.normalize();
				dst = dst * radius;
				point = (vertex + dst).toVector3d();
			}

			float z = vec->z();
			*vec = point - circlePos;
			vec->z() = z;
			collisionHandlerCallback(actor);
			return true;
		}
	}

	return false;
}

void Actor::costumeMarkerCallback(int marker) {
	LuaObjects objects;
	objects.add(this);
	objects.add(marker);

	LuaBase::instance()->callback("costumeMarkerHandler", objects);
}

void Actor::collisionHandlerCallback(Actor *other) const {
	LuaObjects objects;
	objects.add(this);
	objects.add(other);

	LuaBase::instance()->callback("collisionHandler", objects);
}

Math::Vector3d Actor::getWorldPos() const {
	if (! isAttached())
		return getPos();

	Actor *attachedActor = Actor::getPool().getObject(_attachedActor);
	Math::Quaternion q = attachedActor->getRotationQuat();
	Math::Matrix4 attachedToWorld = q.toMatrix();
	attachedToWorld.transpose();
	attachedToWorld.setPosition(attachedActor->getWorldPos());

	// If we were attached to a joint, factor in the joint's position & rotation,
	// relative to its actor.
	EMICostume *cost = static_cast<EMICostume *>(attachedActor->getCurrentCostume());
	if (cost && cost->_emiSkel && cost->_emiSkel->_obj) {
		Joint *j = cost->_emiSkel->_obj->getJointNamed(_attachedJoint);
		const Math::Matrix4 &jointToAttached = j->_finalMatrix;
		attachedToWorld = attachedToWorld * jointToAttached;
	}

	Math::Vector3d myPos = getPos();
	attachedToWorld.transform(&myPos, true);
	return myPos;
}

Math::Quaternion Actor::getRotationQuat() const {
	if (g_grim->getGameType() == GType_MONKEY4) {
		Math::Quaternion ret = Math::Quaternion::fromEuler(-_yaw, _pitch, _roll);
		if (_inOverworld)
			ret = Math::Quaternion::fromEuler(-_yaw, -_pitch, -_roll);

		if (isAttached()) {
			Actor *attachedActor = Actor::getPool().getObject(_attachedActor);
			const Math::Quaternion &attachedQuat = attachedActor->getRotationQuat();

			EMICostume *cost = static_cast<EMICostume *>(attachedActor->getCurrentCostume());
			if (cost && cost->_emiSkel && cost->_emiSkel->_obj) {
				Joint *j = cost->_emiSkel->_obj->getJointNamed(_attachedJoint);
				const Math::Quaternion &jointQuat = j->_finalQuat;
				ret = ret * jointQuat * attachedQuat;
			} else {
				ret = ret * attachedQuat;
			}
		}
		return ret;
	} else {
		return Math::Quaternion::fromEuler(-_roll, -_pitch, -_yaw);
	}
}

int Actor::getSortOrder() const {
	if (_attachedActor != 0) {
		Actor *attachedActor = Actor::getPool().getObject(_attachedActor);

		// FIXME: The + 1 here and in getEffectiveSortOrder is just a guess.
		// Without it it makes some attachments render on top of their owner.
		// Theoritically it could cause an issue if the actor is close to the
		// edge of a sort order boundry.
		return attachedActor->getSortOrder() + 1;
	}
	return _sortOrder;
}

int Actor::getEffectiveSortOrder() const {
	if (_attachedActor != 0) {
		Actor *attachedActor = Actor::getPool().getObject(_attachedActor);
		return attachedActor->getEffectiveSortOrder() + 1;
	}
	return _haveSectorSortOrder ? _sectorSortOrder : getSortOrder();
}

void Actor::attachToActor(Actor *other, const char *joint) {
	assert(other != NULL);
	if (other->getId() == _attachedActor)
		return;
	if (_attachedActor != 0)
		detach();

	Common::String jointStr = joint ? joint : "";

	EMICostume *cost = static_cast<EMICostume *>(other->getCurrentCostume());
	// If 'other' has a skeleton, check if it has the joint.
	// Some models (pile o' boulders) don't have a skeleton,
	// so we don't make the check in that case.
	if (cost && cost->_emiSkel && cost->_emiSkel->_obj)
		assert(cost->_emiSkel->_obj->hasJoint(jointStr));

	_attachedActor = other->getId();
	_attachedJoint = jointStr;
}

void Actor::detach() {
	if (!isAttached())
		return;

	// FIXME: Use last known position of attached joint
	Math::Vector3d oldPos = getWorldPos();
	_attachedActor = 0;
	_attachedJoint = "";
	setPos(oldPos);
	setRot(0, 0, 0);
}

void Actor::drawToCleanBuffer() {
	if (!_cleanBuffer) {
		_cleanBuffer = g_driver->genBuffer();
	}
	if (!_cleanBuffer) {
		return;
	}

	_drawnToClean = true;
	// clean the buffer before drawing to it
	g_driver->clearBuffer(_cleanBuffer);
	g_driver->selectBuffer(_cleanBuffer);
	draw();
	g_driver->selectBuffer(0);

	_drawnToClean = true;
}

void Actor::clearCleanBuffer() {
	if (_cleanBuffer) {
		g_driver->delBuffer(_cleanBuffer);
		_cleanBuffer = 0;
	}
}

void Actor::restoreCleanBuffer() {
	if (_cleanBuffer) {
		update(0);
		drawToCleanBuffer();
	}
}

unsigned const int Actor::Chore::fadeTime = 150;

Actor::Chore::Chore() :
	_costume(NULL),
	_chore(-1) {

}

Actor::Chore::Chore(Costume *cost, int chore) :
	_costume(cost),
	_chore(chore) {

}

void Actor::Chore::play(bool fade, unsigned int time) {
	if (isValid()) {
		_costume->playChore(_chore);
		if (fade) {
			_costume->fadeChoreIn(_chore, time);
		}
	}
}

void Actor::Chore::playLooping(bool fade, unsigned int time) {
	if (isValid()) {
		_costume->playChoreLooping(_chore);
		if (fade) {
			_costume->fadeChoreIn(_chore, time);
		}
	}
}

void Actor::Chore::stop(bool fade, unsigned int time) {
	if (isValid()) {
		if (fade) {
			_costume->fadeChoreOut(_chore, time);
		}
		_costume->stopChore(_chore);
	}
}

void Actor::Chore::setLastFrame() {
	if (isValid()) {
		_costume->setChoreLastFrame(_chore);
	}
}

bool Actor::Chore::isPlaying() const {
	return (isValid() && _costume->isChoring(_chore, false) >= 0);
}

void Actor::Chore::saveState(SaveGame *savedState) const {
	if (_costume) {
		savedState->writeBool(true);
		savedState->writeString(_costume->getFilename());
	} else {
		savedState->writeBool(false);
	}
	savedState->writeLESint32(_chore);
}

void Actor::Chore::restoreState(SaveGame *savedState, Actor *actor) {
	if (savedState->readBool()) {
		Common::String fname = savedState->readString();
		_costume = actor->findCostume(fname);
	} else {
		_costume = NULL;
	}
	_chore = savedState->readLESint32();
}

} // end of namespace Grim
