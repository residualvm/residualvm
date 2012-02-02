
#ifndef AGL_GLBITMAP2D_H
#define AGL_GLBITMAP2D_H

#include "graphics/agl/bitmap2d.h"

namespace AGL {

class GLBitmap2D : public Bitmap2D {
public:
	GLBitmap2D(OpenGLRenderer *rend, Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	~GLBitmap2D();

	void draw(int x, int y);

	OpenGLRenderer *_renderer;
	GLuint *_texIds;
	bool _hasTransparency;
	int _numTex;
	byte *_data;
};

}

#endif
