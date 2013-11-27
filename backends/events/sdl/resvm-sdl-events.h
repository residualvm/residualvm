#ifndef BACKEND_EVENTS_RESVM_SDL
#define BACKEND_EVENTS_RESVM_SDL

#include "sdl-events.h"

/**
 * Custom event source for ResidualVM with true joystick support.
 */
class ResVmSdlEventSource : public SdlEventSource {
protected:
	bool handleJoyButtonDown(SDL_Event &ev, Common::Event &event);
	bool handleJoyButtonUp(SDL_Event &ev, Common::Event &event);
	bool handleJoyAxisMotion(SDL_Event &ev, Common::Event &event);
};

#endif
