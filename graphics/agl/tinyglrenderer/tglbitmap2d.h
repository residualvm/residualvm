
#ifndef AGL_TGLBITMAP2D
#define AGL_TGLBITMAP2D

#include "graphics/pixelbuffer.h"

#include "graphics/agl/bitmap2d.h"

namespace AGL {

class TinyGLRenderer;
class BlitImage;

class TGLBitmap2D : public Bitmap2D {
public:
	TGLBitmap2D(TinyGLRenderer *renderer, Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	~TGLBitmap2D();

	void draw(int x, int y);

	TinyGLRenderer *_renderer;
	BlitImage *_img;
	uint16 *_zimg;
	int _size;
};

}

#endif
