
#ifndef AGL_TEXTURE_H
#define AGL_TEXTURE_H

#include "common/rect.h"

namespace Graphics {
class PixelFormat;
}

namespace AGL {

class Texture {
public:
	virtual ~Texture();

	virtual void bind() = 0;

	inline bool hasAlpha() const { return _hasAlpha; }

	inline int getWidth() const { return _width; }
	inline int getHeight() const { return _height; }

protected:
	Texture(const Graphics::PixelFormat &format, int width, int height);

private:
	int _width;
	int _height;
	bool _hasAlpha;

};

}

#endif
