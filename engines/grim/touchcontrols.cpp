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

#include "backends/platform/android/texture.h"

#include "backends/touch/touchcontrols.h"
#include "engines/grim/touchcontrols.h"

namespace Grim {

enum TouchArea {
	kTouchAreaJoystick = 0xffff,
	kTouchAreaCenter   = 0xfffe,
	kTouchAreaRight    = 0xfffd,
	kTouchAreaNone     = 0xfffc,
};

static TouchArea getTouchArea(float xPercent, float yPercent) {
	if (xPercent < 0.3)
		return kTouchAreaJoystick;
	else if (xPercent < 0.8)
		return kTouchAreaCenter;
	else
		return kTouchAreaRight;
}

class GrimControls {
public:
	GrimControls(int width, int height);
	~GrimControls();

	enum { Touch_DOWN, Touch_MOVE, Touch_UP };

	void draw();
	void update(int ptr, int action, int x, int y);

private:
	int _screenWidth, _screenHeight;

	struct Pointer {
		uint16 startX, startY;
		uint16 currentX, currentY;
		TouchArea function;
		bool active;
	};

	enum KeyPressType { DOWN, UP, PRESS };
	void keyPress(const Common::KeyCode code, const KeyPressType type = PRESS);

	enum { kNumPointers = 5 };
	Pointer _pointers[kNumPointers];
	int _activePointers[4];
	Common::KeyCode _joystickPressing, _centerPressing, _rightPressing;
	int &pointerFor(TouchArea ta);
	GLESTexture *_arrowsTexture;

};

static GLES8888Texture *loadBuiltinTexture(const char *filename) {
	Common::ArchiveMemberPtr member = SearchMan.getMember(filename);
	Common::SeekableReadStream *str = member->createReadStream();
	Image::TGADecoder dec;
	dec.loadStream(*str);
	const void *pixels = dec.getSurface()->getPixels();

	GLES8888Texture *ret = new GLES8888Texture();
	uint16 w = dec.getSurface()->w;
	uint16 h = dec.getSurface()->h;
	uint16 pitch = dec.getSurface()->pitch;
	ret->allocBuffer(w, h);
	ret->updateBuffer(0, 0, w, h, pixels, pitch);

	delete str;
	return ret;
}


static Common::Rect clipFor(const Common::KeyCode &cs) {
	switch (cs) {
	case Common::KEYCODE_UP:
	case Common::KEYCODE_PAGEUP:
		return Common::Rect(0, 0, 128, 128);
	case Common::KEYCODE_RIGHT:
		return Common::Rect(128, 0, 256, 128);
	case Common::KEYCODE_DOWN:
	case Common::KEYCODE_PAGEDOWN:
		return Common::Rect(256, 0, 384, 128);
	case Common::KEYCODE_LEFT:
		return Common::Rect(384, 0, 512, 128);
	case Common::KEYCODE_i:
		return Common::Rect(0, 128, 128, 256);
	case Common::KEYCODE_p:
		return Common::Rect(128, 128, 256, 256);
	case Common::KEYCODE_u:
		return Common::Rect(256, 128, 384, 256);
	case Common::KEYCODE_e:
	case Common::KEYCODE_l:
		return Common::Rect(384, 128, 512, 256);
	default: // unreachable
		return Common::Rect(0, 0, 1, 1);
	}
}

GrimControls::GrimControls(int width, int height) :
	_joystickPressing(Common::KEYCODE_INVALID),
	_centerPressing(Common::KEYCODE_INVALID),
	_rightPressing(Common::KEYCODE_INVALID),
	_screenWidth(width),
	_screenHeight(height) {

	_arrowsTexture = loadBuiltinTexture("arrows.tga");

	for (int p = 0; p < kNumPointers; ++p) {
		Pointer &pp = _pointers[p];
		pp.currentX = pp.currentY = pp.startX = pp.startY = 0;
		pp.active   = false;
		pp.function = kTouchAreaNone;
	}

	for (int i = 0; i < 4; ++i)
		_activePointers[i] = -1;
}

GrimControls::~GrimControls() {
	if (_arrowsTexture) {
		delete _arrowsTexture;
		_arrowsTexture = 0;
	}
}

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

const uint _numRightKeycodes = 4;
const Common::KeyCode _rightKeycodes[] = {
	Common::KEYCODE_i,
	Common::KEYCODE_p,
	Common::KEYCODE_u,
	Common::KEYCODE_e
};

void GrimControls::keyPress(const Common::KeyCode code, const KeyPressType type) {
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

void GrimControls::draw() {
	if (_joystickPressing != Common::KEYCODE_INVALID) {
		Common::Rect clip = clipFor(_joystickPressing);
		_arrowsTexture->drawTexture(2 * _screenWidth / 10, _screenHeight / 2, 64, 64, clip);
	}

	if (_centerPressing != Common::KEYCODE_INVALID) {
		Common::Rect clip = clipFor(_centerPressing);
		_arrowsTexture->drawTexture(_screenWidth / 2, _screenHeight / 2, 64, 64, clip);
	}

	if (_rightPressing != Common::KEYCODE_INVALID) {
		Common::Rect clip = clipFor(_rightPressing);
		_arrowsTexture->drawTexture( 8 * _screenWidth / 10, _screenHeight / 2, 64, 64, clip);
	}
}

void GrimControls::update(int ptr, int action, int x, int y) {
	if (ptr > kNumPointers)
		return;

	TouchArea touchArea = getTouchArea(float(x) / _screenWidth, float(y) / _screenHeight);

	switch (action) {
	case Touch_DOWN: {
		if (touchArea > kTouchAreaNone && -1 == pointerFor(touchArea)) {
			pointerFor(touchArea) = ptr;
			_pointers[ptr].active = true;
			_pointers[ptr].function = touchArea;
			_pointers[ptr].startX = _pointers[ptr].currentX = x;
			_pointers[ptr].startY = _pointers[ptr].currentY = y;
			// fall through to move case to initialize _{joy,center,right}Pressing
		} else {
			return;
		}
	}

	case Touch_MOVE: {
		_pointers[ptr].currentX = x;
		_pointers[ptr].currentY = y;
		int dX = x - _pointers[ptr].startX;
		int dY = y - _pointers[ptr].startY;

		switch (_pointers[ptr].function) {
		case kTouchAreaJoystick: {
			Common::KeyCode newPressing = determineKey(dX, dY);
			if (newPressing != _joystickPressing) {
				keyPress(_joystickPressing, UP);
				keyPress(newPressing, DOWN);
				_joystickPressing = newPressing;
			} else if(abs(dY) > _screenHeight / 5) {
			   keyPress(Common::KEYCODE_LSHIFT, DOWN);
			} else if(abs(dY) <= _screenHeight / 5){
			   keyPress(Common::KEYCODE_LSHIFT, UP);
			}
			return;
		}

		case kTouchAreaCenter:
			_centerPressing = determineKey(dX, dY, Common::KEYCODE_RETURN);
			return;

		case kTouchAreaRight:
			_rightPressing = determineKey(dX, dY, Common::KEYCODE_i);
			switch (_rightPressing) {
			case Common::KEYCODE_LEFT:
			case Common::KEYCODE_RIGHT:
				_rightPressing = _rightKeycodes[abs(dX / 100) % _numRightKeycodes];
				break;

			case Common::KEYCODE_UP:
				_rightPressing = Common::KEYCODE_PAGEUP;
				break;

			case Common::KEYCODE_DOWN:
				_rightPressing = Common::KEYCODE_PAGEDOWN;
				break;

			default:
				break;
			}

		default:
			return;
		}
		return;
	}

	case Touch_UP: {
		switch (_pointers[ptr].function) {
		case kTouchAreaJoystick:
			pointerFor(kTouchAreaJoystick) = -1;
			if (_joystickPressing != Common::KEYCODE_INVALID) {
				keyPress(_joystickPressing, UP);
				_joystickPressing = Common::KEYCODE_INVALID;
				keyPress(Common::KEYCODE_LSHIFT, UP);
			}
			break;

		case kTouchAreaCenter:
			pointerFor(kTouchAreaCenter) = -1;
			keyPress(_centerPressing);
			_centerPressing = Common::KEYCODE_INVALID;
			break;

		case kTouchAreaRight:
			pointerFor(kTouchAreaRight) = -1;
			keyPress(_rightPressing);
			_rightPressing = Common::KEYCODE_INVALID;
			break;

		case kTouchAreaNone:
		default:
			break;
		}

		_pointers[ptr].active = false;
		_pointers[ptr].function = kTouchAreaNone;
		return;
	}
	}
}

int &GrimControls::pointerFor(TouchArea ta) {
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
		_gc = new GrimControls(width, height);
}

JoystickMode::~JoystickMode() {
	delete _gc;
}

void JoystickMode::draw() {
	_gc->draw();
}

void JoystickMode::pointerDown(uint32 pointerId, uint32 x, uint32 y) {
	_gc->update(pointerId, GrimControls::Touch_DOWN, x, y);
}

void JoystickMode::pointerMove(uint32 pointerId, uint32 x, uint32 y) {
	_gc->update(pointerId, GrimControls::Touch_MOVE, x, y);
}

void JoystickMode::pointerUp(uint32 pointerId, uint32 x, uint32 y) {
	_gc->update(pointerId, GrimControls::Touch_UP, x, y);
}

}
#endif
