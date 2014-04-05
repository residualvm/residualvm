#define FORBIDDEN_SYMBOL_EXCEPTION_time_h
#define FORBIDDEN_SYMBOL_EXCEPTION_printf

#include "common/rect.h"
#include "backends/platform/android/android.h"
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

#include "backends/touch/grimcontrols.h"

void TouchControlsBackend::requestTouchControlMode(const TouchControlsMode mode) {
	if (mode == _touchMode)
		return;

	_touchMode = mode;
	delete _touchImpl;
	
	switch (_touchMode) {
		case kNullMode:
			_touchImpl = new NullTouchMode();
			break;
		case kTouchpadMode:
			_touchImpl = new TouchpadMode(_touchWidth, _touchHeight);
			break;
		case kJoystickMode:
			_touchImpl = new JoystickMode(_touchWidth, _touchHeight);
			break;
	}
}

void TouchControlsBackend::initTouchSurface(uint32 width, uint32 height) {
	_touchWidth = width; _touchHeight = height;
	LOGD("RequestTouchControlMode! (%d, %x) ", width, height);
	requestTouchControlMode(kTouchpadMode);
}

TouchControlsBackend::TouchControlsBackend()
	: _touchMode(kNullMode),
    _touchImpl(new NullTouchMode()),
    _touchWidth(0),
    _touchHeight(0) {}

TouchControlsBackend::~TouchControlsBackend() {
	delete _touchImpl;
}

