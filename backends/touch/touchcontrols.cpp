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

#include "common/rect.h"
#include "common/system.h"

#include "backends/touch/touchcontrols.h"
#include "backends/touch/touch_impl.h"

class TouchpadMode : public TouchControlsImpl {
	public:
	TouchpadMode(uint32 width, uint32 height)
		: TouchControlsImpl(width, height) {}

	virtual void process(const Common::Event &ev) {

	}

	virtual void draw() {

	}

	virtual void pointerDown(uint32 pointerId, uint32 x, uint32 y) {
		_mouseStartPos = g_system->getEventManager()->getMousePos();
		_screenStartPos = Common::Point(x,y);
	}

	virtual void pointerMove(uint32 pointerId, uint32 x, uint32 y) {
		Common::Event e;

		const Common::Point screenNowPos(x,y);
		const Common::Point mouseNowPos = _mouseStartPos + (screenNowPos - _screenStartPos);

		e.type = Common::EVENT_MOUSEMOVE;
		e.mouse.x = CLIP(int16(mouseNowPos.x), int16(0), int16(_screenW - 1));
		e.mouse.y = CLIP(int16(mouseNowPos.y), int16(0), int16(_screenH - 1));

		g_system->getEventManager()->pushEvent(e);
	}

	virtual void pointerUp(uint32 pointerId, uint32 x, uint32 y) {
		Common::Event e;
		e.mouse = g_system->getEventManager()->getMousePos();

		if (_mouseStartPos.sqrDist(e.mouse) > 2500)
			return;

		e.type = Common::EVENT_LBUTTONDOWN;
		g_system->getEventManager()->pushEvent(e);

		e.type = Common::EVENT_LBUTTONUP;
		g_system->getEventManager()->pushEvent(e);
	}

	private:
	Common::Point _mouseStartPos;
	Common::Point _screenStartPos;
};

void TouchControlsBackend::requestTouchControlMode(TouchControlsImpl *impl) {
	if (impl == _touchImpl)
		return;

	delete _touchImpl;
	_touchImpl = impl;
}

void TouchControlsBackend::initTouchSurface(uint32 width, uint32 height) {
	_touchWidth = width;
	_touchHeight = height;
	TouchControlsImpl *mode = new TouchpadMode(width, height);
	requestTouchControlMode(mode);
}

TouchControlsBackend::TouchControlsBackend()
	: _touchImpl(new NullTouchMode()),
	  _touchWidth(0),
	  _touchHeight(0) {}

TouchControlsBackend::~TouchControlsBackend() {
	delete _touchImpl;
}

