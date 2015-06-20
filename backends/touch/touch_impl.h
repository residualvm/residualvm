#ifndef BACKENDS_TOUCH_IMPL_H
#define BACKENDS_TOUCH_IMPL_H

#include "common/events.h"
#include "common/ptr.h"
#include "common/rect.h"

#include "math/vector2d.h"

namespace Graphics {
	class Texture;
}

class TouchControlsImpl {
	public:
	TouchControlsImpl(uint32 width, uint32 height)
		: _screenW(width), _screenH(height) {}
	virtual ~TouchControlsImpl() {}
	virtual void process(const Common::Event &ev) = 0;
	virtual void draw() {};
	virtual void pointerDown(uint32 pointerId, uint32 x, uint32 y) = 0;
	virtual void pointerMove(uint32 pointerId, uint32 x, uint32 y) = 0;
	virtual void pointerUp(uint32 pointerId, uint32 x, uint32 y) = 0;

	protected:
	uint32 _screenW, _screenH;
	void drawRect(Graphics::Texture *tex, const Common::Rect& src,
	              const Math::Vector2d &dstPos, const Math::Vector2d &dstSize);
};

class NullTouchMode : public TouchControlsImpl {
	public:
	NullTouchMode() : TouchControlsImpl(0,0) {}
	virtual void process(const Common::Event &ev) {};
	virtual void pointerDown(uint32 pointerId, uint32 x, uint32 y) {};
	virtual void pointerMove(uint32 pointerId, uint32 x, uint32 y) {};
	virtual void pointerUp(uint32 pointerId, uint32 x, uint32 y) {};
};

#endif
