
#include "graphics/agl/manager.h"
#include "graphics/agl/renderer.h"
#include "graphics/agl/bitmap2d.h"

namespace AGL {

Bitmap2D::Bitmap2D(Type type, int width, int height)
	: _type(type),
	  _width(width),
	  _height(height) {

}

Bitmap2D::~Bitmap2D() {

}

}
