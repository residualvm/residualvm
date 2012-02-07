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
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "graphics/agl/manager.h"
#include "graphics/agl/primitive.h"
#include "graphics/agl/target.h"

#include "engines/grim/iris.h"
#include "engines/grim/gfx_base.h"
#include "engines/grim/savegame.h"
#include "engines/grim/grim.h"

namespace Grim {

Iris::Iris() :
	_playing(false), _direction(Open) {

	_primitive = NULL;
}

Iris::~Iris() {
	delete _primitive;
}

void Iris::play(Iris::Direction dir, int x, int y, int lenght) {
	_playing = true;
	_direction = dir;
	_targetX = x;
	_targetY = y;
	_lenght = lenght;
	_currTime = 0;

	if (!_primitive) {
		_primitive = AGLMan.createPrimitive();
		Color c(0, 0, 0);
		_primitive->setGlobalColor(c);

		const int height = AGLMan.getTarget()->getHeight();
		const int width = AGLMan.getTarget()->getWidth();

		const float dummy = 0.f;

		_primitive->begin(AGL::Quads);

		_primitive->vertex(Math::Vector2d(0.f, 0.f));
		_primitive->vertex(Math::Vector2d(dummy, 0.f));
		_primitive->vertex(Math::Vector2d(dummy, height));
		_primitive->vertex(Math::Vector2d(0.f, height));

		_primitive->vertex(Math::Vector2d(dummy, height));
		_primitive->vertex(Math::Vector2d(width, height));
		_primitive->vertex(Math::Vector2d(width, dummy));
		_primitive->vertex(Math::Vector2d(dummy, dummy));

		_primitive->vertex(Math::Vector2d(dummy, height));
		_primitive->vertex(Math::Vector2d(width, height));
		_primitive->vertex(Math::Vector2d(width, 0.f));
		_primitive->vertex(Math::Vector2d(dummy, 0.f));

		_primitive->vertex(Math::Vector2d(dummy, dummy));
		_primitive->vertex(Math::Vector2d(dummy, dummy));
		_primitive->vertex(Math::Vector2d(dummy, 0.f));
		_primitive->vertex(Math::Vector2d(dummy, 0.f));

		_primitive->end();
	}
}

void Iris::draw() {
	if (!_playing) {
		if (_direction == Close && g_grim->getMode() != GrimEngine::SmushMode) {
			_x1 = _x2 = 320;
			_y1 = _y2 = 240;
		} else {
			return;
		}
	}

	const int height = AGLMan.getTarget()->getHeight();
	const int width = AGLMan.getTarget()->getWidth();

	_primitive->setVertex(0, 1, Math::Vector2d(_x1, 0.f));
	_primitive->setVertex(0, 2, Math::Vector2d(_x1, height));

	_primitive->setVertex(0, 4, Math::Vector2d(_x1, height));
	_primitive->setVertex(0, 6, Math::Vector2d(width, _y2));
	_primitive->setVertex(0, 7, Math::Vector2d(_x1, _y2));

	_primitive->setVertex(0, 8, Math::Vector2d(_x2, height));
	_primitive->setVertex(0, 11, Math::Vector2d(_x2, 0.f));

	_primitive->setVertex(0, 12, Math::Vector2d(_x1, _y1));
	_primitive->setVertex(0, 13, Math::Vector2d(_x2, _y1));
	_primitive->setVertex(0, 14, Math::Vector2d(_x2, 0.f));
	_primitive->setVertex(0, 15, Math::Vector2d(_x1, 0.f));

	_primitive->draw(0, 0);
}

void Iris::update(int frameTime) {
	if (!_playing) {
		return;
	}

	_currTime += frameTime;
	if (_currTime >= _lenght) {
		_playing = false;
		return;
	}

	float factor = (float)_currTime / (float)_lenght;
	if (_direction == Open) {
		factor = 1 - factor;
	}

	_y1 = (int)(_targetY * factor);
	_x1 = (int)(_targetX * factor);
	_y2 = (int)(480 - (480 - _targetY) * factor);
	_x2 = (int)(640 - (640 - _targetX) * factor);
}

void Iris::saveState(SaveGame *state) const {
	state->beginSection('IRIS');

	state->writeBool(_playing);
	state->writeLEUint32((uint32)_direction);
	state->writeLESint32(_x1);
	state->writeLESint32(_y1);
	state->writeLESint32(_x2);
	state->writeLESint32(_y2);
	state->writeLESint32(_lenght);
	state->writeLESint32(_currTime);

	state->endSection();
}

void Iris::restoreState(SaveGame *state) {
	state->beginSection('IRIS');

	_playing = state->readBool();
	_direction = (Direction)state->readLEUint32();
	_x1 = state->readLESint32();
	_y1 = state->readLESint32();
	_x2 = state->readLESint32();
	_y2 = state->readLESint32();
	_lenght = state->readLESint32();
	_currTime = state->readLESint32();

	state->endSection();
}

}
