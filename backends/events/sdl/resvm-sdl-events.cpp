#include "common/scummsys.h"

#if defined(SDL_BACKEND)

#include "resvm-sdl-events.h"

bool ResVmSdlEventSource::handleJoyButtonDown(SDL_Event &ev, Common::Event &event) {
	event.type = Common::EVENT_JOYBUTTON_DOWN;
	event.joystick.button = ev.jbutton.button;
	return true;
}

bool ResVmSdlEventSource::handleJoyButtonUp(SDL_Event &ev, Common::Event &event) {
	event.type = Common::EVENT_JOYBUTTON_UP;
	event.joystick.button = ev.jbutton.button;
	return true;
}

bool ResVmSdlEventSource::handleJoyAxisMotion(SDL_Event &ev, Common::Event &event) {
	event.type = Common::EVENT_JOYAXIS_MOTION;
	event.joystick.axis = ev.jaxis.axis;
	event.joystick.position = ev.jaxis.value;
	return true;
}

#endif
