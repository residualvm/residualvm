
#include "graphics/pixelformat.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/renderer.h"
#include "graphics/agl/texture.h"

namespace AGL {

Texture::Texture(const Graphics::PixelFormat &format, int width, int height)
	: _width(width),
	  _height(height) {

	_hasAlpha = format.aBits() > 0;
}

Texture::~Texture() {

}

}
