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
// Defines two fake buttons for L2/R2 (defined as axis in controller)
#define SDL_CONTROLLER_BUTTON_L2 (SDL_CONTROLLER_BUTTON_MAX)
#define SDL_CONTROLLER_BUTTON_R2 (SDL_CONTROLLER_BUTTON_MAX+1)

bool ResVmSdlEventSource::handleControllerButton(const SDL_Event &ev, Common::Event &event, bool buttonUp) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleControllerButton(ev, event, buttonUp);
	} else {
#ifdef ENABLE_VKEYBD
	        // Trigger virtual keyboard on long press of more than 1 second of configured button
        	const uint32 vkeybdTime = 1000;
        	static uint32 vkeybdThen = 0;

		if (ev.cbutton.button == CONTROLLER_BUT_VKEYBOARD) {
			if (!buttonUp)
				vkeybdThen = g_system->getMillis();
			else if ((vkeybdThen > 0) && (g_system->getMillis() - vkeybdThen >= vkeybdTime)) {
				vkeybdThen = 0;
				event.type = Common::EVENT_VIRTUAL_KEYBOARD;
				return true;
			}
		}
#endif
		event.type = buttonUp ? Common::EVENT_JOYBUTTON_UP : Common::EVENT_JOYBUTTON_DOWN;
		event.joystick.button = ev.cbutton.button;
		return true;
	}
}

bool ResVmSdlEventSource::handleControllerAxisMotion(const SDL_Event &ev, Common::Event &event) {
	if (shouldGenerateMouseEvents()) {
		return SdlEventSource::handleControllerAxisMotion(ev, event);
	} else {
		// Indicates if L2/R2 are currently pushed or not
		static bool l2Pushed = false, r2Pushed = false;
		// Simulate buttons from left and right triggers (needed for EMI)
		if (ev.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
			bool pushed = (ev.caxis.value > 0);
			if (l2Pushed == pushed)
				return false;
			event.type = (pushed)?Common::EVENT_JOYBUTTON_DOWN:Common::EVENT_JOYBUTTON_UP;
			event.joystick.button = SDL_CONTROLLER_BUTTON_L2;
			l2Pushed = pushed;
			return true;
		} else if (ev.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
			bool pushed = (ev.caxis.value > 0);
			if (r2Pushed == pushed)
				return false;
			event.type = (pushed)?Common::EVENT_JOYBUTTON_DOWN:Common::EVENT_JOYBUTTON_UP;
			event.joystick.button = SDL_CONTROLLER_BUTTON_R2;
			r2Pushed = pushed;
			return true;
		} else {
			event.type = Common::EVENT_JOYAXIS_MOTION;
			event.joystick.axis = ev.caxis.axis;
			event.joystick.position = ev.caxis.value;
			return true;
		}
	}
}
#endif

bool ResVmSdlEventSource::shouldGenerateMouseEvents() {
	// Engine doesn't support joystick -> emulate mouse events
	if (g_engine && !g_engine->hasFeature(Engine::kSupportsJoystick)) {
		return true;
	}

	// Even if engine supports joystick, emulate mouse events if in GUI or in virtual keyboard
	if (g_gui.isActive() || g_engine->isPaused()) {
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

		if (graphicsManager) {
			if (graphicsManager->isMouseLocked()) {
				_km.x = oldKmX;
				_km.y = oldKmY;
			} else {
				// Without this "else", Myst3 relative movements are crazy and the camera points to the zenith.
				// Seems linked to mouse moved events generated by SDL2
				graphicsManager->getWindow()->warpMouseInWindow((Uint16)(_km.x / MULTIPLIER), (Uint16)(_km.y / MULTIPLIER));
			}
		}

		event.type = Common::EVENT_MOUSEMOVE;
		return processMouseEvent(event, _km.x / MULTIPLIER, _km.y / MULTIPLIER, relX / MULTIPLIER, relY / MULTIPLIER);
	}

	return false;
}

#endif
