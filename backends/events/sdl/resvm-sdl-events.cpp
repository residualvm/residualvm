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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/scummsys.h"

#if defined(SDL_BACKEND)

#include "resvm-sdl-events.h"
#include "backends/graphics/sdl/resvm-sdl-graphics.h"
#include "engines/engine.h"
#include "gui/gui-manager.h"
#include "graphics/cursorman.h"

#if defined(ENABLE_VKEYBD) && SDL_VERSION_ATLEAST(2, 0, 0)
#define CONTROLLER_BUT_VKEYBOARD SDL_CONTROLLER_BUTTON_BACK
#endif

bool ResVmSdlEventSource::handleJoyButtonDown(SDL_Event &ev, Common::Event &event) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleJoyButtonDown(ev, event);
	} else {
		event.type = Common::EVENT_JOYBUTTON_DOWN;
		event.joystick.button = ev.jbutton.button;
		return true;
	}
}

bool ResVmSdlEventSource::handleJoyButtonUp(SDL_Event &ev, Common::Event &event) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleJoyButtonUp(ev, event);
	} else {
		event.type = Common::EVENT_JOYBUTTON_UP;
		event.joystick.button = ev.jbutton.button;
		return true;
	}
}

bool ResVmSdlEventSource::handleJoyAxisMotion(SDL_Event &ev, Common::Event &event) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleJoyAxisMotion(ev, event);
	} else {
		event.type = Common::EVENT_JOYAXIS_MOTION;
		event.joystick.axis = ev.jaxis.axis;
		event.joystick.position = ev.jaxis.value;
		return true;
	}
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
bool ResVmSdlEventSource::handleControllerButton(const SDL_Event &ev, Common::Event &event, bool buttonUp) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleControllerButton(ev, event, buttonUp);
	} else {
#ifdef ENABLE_VKEYBD
		if (ev.cbutton.button == CONTROLLER_BUT_VKEYBOARD) {
			if (!buttonUp) {
				event.type = Common::EVENT_VIRTUAL_KEYBOARD;
				return true;
			}
			return false;
		}
#endif
		event.type = buttonUp ? Common::EVENT_CONTROLLERBUTTON_UP : Common::EVENT_CONTROLLERBUTTON_DOWN;
		event.joystick.button = ev.cbutton.button;
		return true;
	}
}

bool ResVmSdlEventSource::handleControllerAxisMotion(const SDL_Event &ev, Common::Event &event) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleControllerAxisMotion(ev, event);
	} else {
		event.type = Common::EVENT_CONTROLLERAXIS_MOTION;
		event.joystick.axis = ev.caxis.axis;
		event.joystick.position = ev.caxis.value;
		return true;
	}
}
#endif

bool ResVmSdlEventSource::shouldGenerateMouseEvents() {
	// Engine doesn't support joystick -> emulate mouse events
	if (g_engine && !g_engine->hasFeature(Engine::kSupportsJoystick)) {
		return true;
	}

	// FIXME: call to CursorMan.isVisible is not clean ! But don't know how to do better
	if (g_gui.isActive() || CursorMan.isVisible()) {
		return true;
	}

	return false;
}

bool ResVmSdlEventSource::handleKbdMouse(Common::Event &event) {
	// The ResidualVM version of this method handles relative mouse
	// movement, as required by Myst III.

	int16 oldKmX = _km.x;
	int16 oldKmY = _km.y;

	updateKbdMouse();

	if (_km.x != oldKmX || _km.y != oldKmY) {
		ResVmSdlGraphicsManager *graphicsManager = dynamic_cast<ResVmSdlGraphicsManager *>(_graphicsManager);

		int16 relX = _km.x - oldKmX;
		int16 relY = _km.y - oldKmY;

		if (graphicsManager && graphicsManager->isMouseLocked()) {
			_km.x = oldKmX;
			_km.y = oldKmY;
		}
		else
		if (graphicsManager) {
			graphicsManager->getWindow()->warpMouseInWindow((Uint16)(_km.x / MULTIPLIER), (Uint16)(_km.y / MULTIPLIER));
		}

		event.type = Common::EVENT_MOUSEMOVE;
		return processMouseEvent(event, _km.x / MULTIPLIER, _km.y / MULTIPLIER, relX / MULTIPLIER, relY / MULTIPLIER);
	}

	return false;
}

#endif
