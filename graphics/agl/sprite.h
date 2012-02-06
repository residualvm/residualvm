
#ifndef AGL_SPRITE_H
#define AGL_SPRITE_H

#include "math/vector3d.h"

namespace AGL {

class Texture;

class Sprite {
public:
	Sprite(float width, float height);
	virtual ~Sprite();

	virtual void draw(Texture *tex, float x, float y, float z) const = 0;
	inline void draw(Texture *tex, const Math::Vector3d &pos) const { draw(tex, pos.x(), pos.y(), pos.z()); }

protected:
	inline float getWidth() const { return _width; }
	inline float getHeight() const { return _height; }

private:
	float _width;
	float _height;

};

}

#endif
