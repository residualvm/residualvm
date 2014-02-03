#ifndef BACKENDS_TOUCHCONTROLS_H
#define BACKENDS_TOUCHCONTROLS_H

#include "backends/touch/touch_impl.h"

class TouchControlsBackend {
	public:
	void requestTouchControlMode(TouchControlsImpl *impl);

	TouchControlsBackend();
	~TouchControlsBackend();

	void initTouchSurface(uint32 width, uint32 height);
	void draw() { _touchImpl->draw(); }

	void pointerDown(uint32 pointerId, uint32 x, uint32 y) {
		_touchImpl->pointerDown(pointerId, x, y);
	}
	void pointerMove(uint32 pointerId, uint32 x, uint32 y) {
		_touchImpl->pointerMove(pointerId, x, y);
	}
	void pointerUp(uint32 pointerId, uint32 x, uint32 y) {
		_touchImpl->pointerUp(pointerId, x, y);
	}

	uint32 getTouchWidth()  const { return _touchWidth; }
	uint32 getTouchHeight() const { return _touchHeight; }

	private:
	TouchControlsImpl *_touchImpl;

	uint32 _touchWidth, _touchHeight;
};

#endif
