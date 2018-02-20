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
#include "graphics/cursorman.h"
#include "engines/engine.h"

// The maximum index of the joystick button to convert in mouse event
#define JOY_TO_MOUSE_BUTTONS 1

bool ResVmSdlEventSource::handleJoyButtonDown(SDL_Event &ev, Common::Event &event) {
	// Engine doesn't support joystick -> emulate mouse events
	if (g_engine)
		if (!g_engine->hasFeature(Engine::kSupportsJoystick))
			return SdlEventSource::handleJoyButtonDown(ev, event);
	
	// If mouse cursor is visible, emulates standard buttons of joystick as mouse
	if (CursorMan.isVisible() && (ev.jbutton.button < JOY_TO_MOUSE_BUTTONS)) {
		return SdlEventSource::handleJoyButtonDown(ev, event);
	}
	else {
		event.type = Common::EVENT_JOYBUTTON_DOWN;
		event.joystick.button = ev.jbutton.button;
		return true;
	}
}

bool ResVmSdlEventSource::handleJoyButtonUp(SDL_Event &ev, Common::Event &event) {
	// Engine doesn't support joystick -> emulate mouse events
	if (g_engine)
		if (!g_engine->hasFeature(Engine::kSupportsJoystick))
			return SdlEventSource::handleJoyButtonUp(ev, event);
	
	// If mouse cursor is visible, emulates standard buttons of joystick as mouse
	if (CursorMan.isVisible() && (ev.jbutton.button < JOY_TO_MOUSE_BUTTONS)) {
		return SdlEventSource::handleJoyButtonUp(ev, event);
	}
	else {
		event.type = Common::EVENT_JOYBUTTON_UP;
		event.joystick.button = ev.jbutton.button;
		return true;
	}
}

bool ResVmSdlEventSource::handleJoyAxisMotion(SDL_Event &ev, Common::Event &event) {
	// Engine doesn't support joystick -> emulate mouse events
	if (g_engine)
			if (!g_engine->hasFeature(Engine::kSupportsJoystick))
				return SdlEventSource::handleJoyAxisMotion(ev, event);
	
	// If mouse cursor is visible, emulates joystick movements as mouse
	if (CursorMan.isVisible()) {
		return SdlEventSource::handleJoyAxisMotion(ev, event);
	}
	else {
		event.type = Common::EVENT_JOYAXIS_MOTION;
		event.joystick.axis = ev.jaxis.axis;
		event.joystick.position = ev.jaxis.value;
		return true;
	}
}

#endif
