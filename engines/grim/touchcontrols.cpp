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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/archive.h"
#include "common/events.h"
#include "common/fs.h"
#include "common/stream.h"
#include "common/system.h"
#include "image/tga.h"

#if defined(ENABLE_TOUCH)

#include "backends/touch/touchcontrols.h"
#include "engines/grim/touchcontrols.h"

#include "graphics/opengl/texture.h"

namespace Grim {

enum TouchArea {
	kTouchAreaNone     = 0,
	kTouchAreaJoystick = 1,
	kTouchAreaCenter   = 2,
	kTouchAreaRight    = 3,
};
enum { kNumPointers = 5 };
enum KeyPressType { DOWN, UP, PRESS };

static const uint _numRightKeycodes = 4;
static const Common::KeyCode _rightKeycodes[] = {
	Common::KEYCODE_i,
	Common::KEYCODE_p,
	Common::KEYCODE_u,
	Common::KEYCODE_e
};

static Common::KeyCode determineKey(int dX, int dY, Common::KeyCode def = Common::KEYCODE_INVALID) {
	if (dX * dX + dY * dY < 50 * 50)
		return def;

	if (dY > abs(dX))
		return Common::KEYCODE_DOWN;
	if (dX > abs(dY))
		return Common::KEYCODE_RIGHT;
	if (-dY > abs(dX))
		return Common::KEYCODE_UP;
	if (-dX > abs(dY))
		return Common::KEYCODE_LEFT;

	return Common::KEYCODE_INVALID;
}

static TouchArea getTouchArea(float xPercent, float yPercent) {
	if (xPercent < 0.3)
		return kTouchAreaJoystick;
	else if (xPercent < 0.8)
		return kTouchAreaCenter;
	else
		return kTouchAreaRight;
}

static void keyPress(const Common::KeyCode code, const KeyPressType type = PRESS) {
	Common::EventManager *em = g_system->getEventManager();
	Common::Event e;
	e.kbd.keycode = code;
	if (type == DOWN || type == PRESS) {
		e.type = Common::EVENT_KEYDOWN;
		em->pushEvent(e);
	}
	if (type == UP || type == PRESS) {
		e.type = Common::EVENT_KEYUP;
		em->pushEvent(e);
	}
}

static Common::Rect clipFor(const Common::KeyCode &cs) {
	switch (cs) {
	case Common::KEYCODE_UP:
	case Common::KEYCODE_PAGEUP:
		return Common::Rect(0, 128, 128, 256);
	case Common::KEYCODE_RIGHT:
		return Common::Rect(128, 128, 256, 256);
	case Common::KEYCODE_DOWN:
	case Common::KEYCODE_PAGEDOWN:
		return Common::Rect(256, 128, 384, 256);
	case Common::KEYCODE_LEFT:
		return Common::Rect(384, 128, 512, 256);
	case Common::KEYCODE_i:
		return Common::Rect(0, 0, 128, 128);
	case Common::KEYCODE_p:
		return Common::Rect(128, 0, 256, 128);
	case Common::KEYCODE_u:
		return Common::Rect(256, 0, 384, 128);
	case Common::KEYCODE_e:
	case Common::KEYCODE_l:
		return Common::Rect(384, 0, 512, 128);
	default: // unreachable
		return Common::Rect(0, 0, 1, 1);
	}
}

class JoystickMode::State {
	public:
	State();
	~State();

	struct Pointer {
		uint16 startX, startY;
		uint16 currentX, currentY;
		TouchArea function;
		bool active;
	};

	Pointer _pointers[kNumPointers];
	int _activePointers[4];
	Common::KeyCode _joystickPressing, _centerPressing, _rightPressing;
	int &pointerFor(TouchArea ta);
	Graphics::Texture *_arrowsTexture;
};

static Graphics::Texture *loadBuiltinTexture(const char *filename) {
	Common::ArchiveMemberPtr member = SearchMan.getMember(filename);
	Common::SeekableReadStream *str = member->createReadStream();
	Image::TGADecoder dec;
	dec.loadStream(*str);
	Graphics::Texture *tex = new Graphics::Texture(*dec.getSurface());

	dec.destroy();
	delete str;
	return tex;
}

JoystickMode::State::State() :
	_joystickPressing(Common::KEYCODE_INVALID),
	_centerPressing(Common::KEYCODE_INVALID),
	_rightPressing(Common::KEYCODE_INVALID) {

	_arrowsTexture = loadBuiltinTexture("arrows.tga");

	for (int p = 0; p < kNumPointers; ++p) {
		Pointer &pp = _pointers[p];
		pp.currentX = pp.currentY = pp.startX = pp.startY = 0;
		pp.active   = false;
		pp.function = kTouchAreaNone;
	}

	for (int i = 0; i < 4; ++i) {
		_activePointers[i] = -1;
	}
}

JoystickMode::State::~State() {
	delete _arrowsTexture;
}


int &JoystickMode::State::pointerFor(TouchArea ta) {
	return _activePointers[ta - kTouchAreaNone];
}

JoystickMode *JoystickMode::create() {
	TouchControlsBackend *tcb = dynamic_cast<TouchControlsBackend *>(g_system);
	uint32 width  = tcb->getTouchWidth();
	uint32 height = tcb->getTouchHeight();
	return new JoystickMode(width, height);
}

JoystickMode::JoystickMode(uint32 width, uint32 height)
	: TouchControlsImpl(width, height) {
		_gc = new JoystickMode::State();
}

JoystickMode::~JoystickMode() {
	delete _gc;
}

void JoystickMode::draw() {
	if (_gc->_joystickPressing != Common::KEYCODE_INVALID) {
		Common::Rect clip = clipFor(_gc->_joystickPressing);
		drawRect(_gc->_arrowsTexture, clip, Math::Vector2d(2 * _screenW / 10, _screenH / 2), Math::Vector2d(64, 64));
	}

	if (_gc->_centerPressing != Common::KEYCODE_INVALID) {
		Common::Rect clip = clipFor(_gc->_centerPressing);
		drawRect(_gc->_arrowsTexture, clip, Math::Vector2d(_screenW / 2, _screenH / 2), Math::Vector2d(64, 64));
	}

	if (_gc->_rightPressing != Common::KEYCODE_INVALID) {
		Common::Rect clip = clipFor(_gc->_rightPressing);
		drawRect(_gc->_arrowsTexture, clip, Math::Vector2d(8 * _screenW / 10, _screenH / 2), Math::Vector2d(64, 64));
	}
}

void JoystickMode::pointerDown(uint32 ptr, uint32 x, uint32 y) {
	if (ptr > kNumPointers)
		return;

	TouchArea touchArea = getTouchArea(float(x) / _screenW, float(y) / _screenH);

	if (touchArea > kTouchAreaNone && -1 == _gc->pointerFor(touchArea)) {
		_gc->pointerFor(touchArea) = ptr;
		State::Pointer *pointers = _gc->_pointers;
		pointers[ptr].active = true;
		pointers[ptr].function = touchArea;
		pointers[ptr].startX = pointers[ptr].currentX = x;
		pointers[ptr].startY = pointers[ptr].currentY = y;
		// fall through to move case to initialize _{joy,center,right}Pressing
		pointerMove(ptr, x, y);
	}
}

void JoystickMode::pointerMove(uint32 ptr, uint32 x, uint32 y) {
	if (ptr > kNumPointers)
		return;

	State::Pointer *pointers = _gc->_pointers;
	pointers[ptr].currentX = x;
	pointers[ptr].currentY = y;
	int dX = x - pointers[ptr].startX;
	int dY = y - pointers[ptr].startY;

	switch (pointers[ptr].function) {
	case kTouchAreaJoystick: {
		Common::KeyCode newPressing = determineKey(dX, dY);
		if (newPressing != _gc->_joystickPressing) {
			keyPress(_gc->_joystickPressing, UP);
			keyPress(newPressing, DOWN);
			_gc->_joystickPressing = newPressing;
		} else if(uint32(abs(dY)) > _screenH / 5) {
			 keyPress(Common::KEYCODE_LSHIFT, DOWN);
		} else if(uint32(abs(dY)) <= _screenH / 5){
			 keyPress(Common::KEYCODE_LSHIFT, UP);
		}
		return;
	}

	case kTouchAreaCenter:
		_gc->_centerPressing = determineKey(dX, dY, Common::KEYCODE_RETURN);
		return;

	case kTouchAreaRight:
		_gc->_rightPressing = determineKey(dX, dY, Common::KEYCODE_i);
		switch (_gc->_rightPressing) {
		case Common::KEYCODE_LEFT:
		case Common::KEYCODE_RIGHT:
			_gc->_rightPressing = _rightKeycodes[abs(dX / 100) % _numRightKeycodes];
			break;

		case Common::KEYCODE_UP:
			_gc->_rightPressing = Common::KEYCODE_PAGEUP;
			break;

		case Common::KEYCODE_DOWN:
			_gc->_rightPressing = Common::KEYCODE_PAGEDOWN;
			break;

		default:
			break;
		}

	default:
		return;
	}
}

void JoystickMode::pointerUp(uint32 ptr, uint32 x, uint32 y) {
	if (ptr > kNumPointers)
		return;

	State::Pointer *pointers = _gc->_pointers;
	switch (pointers[ptr].function) {
	case kTouchAreaJoystick:
		_gc->pointerFor(kTouchAreaJoystick) = -1;
		if (_gc->_joystickPressing != Common::KEYCODE_INVALID) {
			keyPress(_gc->_joystickPressing, UP);
			_gc->_joystickPressing = Common::KEYCODE_INVALID;
			keyPress(Common::KEYCODE_LSHIFT, UP);
		}
		break;

	case kTouchAreaCenter:
		_gc->pointerFor(kTouchAreaCenter) = -1;
		keyPress(_gc->_centerPressing);
		_gc->_centerPressing = Common::KEYCODE_INVALID;
		break;

	case kTouchAreaRight:
		_gc->pointerFor(kTouchAreaRight) = -1;
		keyPress(_gc->_rightPressing);
		_gc->_rightPressing = Common::KEYCODE_INVALID;
		break;

	case kTouchAreaNone:
	default:
		break;
	}


	pointers[ptr].active = false;
	pointers[ptr].function = kTouchAreaNone;
}

}
#endif
