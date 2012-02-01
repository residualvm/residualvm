
#ifndef AGL_TINYGLRENDERER_H
#define AGL_TINYGLRENDERER_H

#include "graphics/agl/renderer.h"

namespace TinyGL {
struct ZBuffer;
}

namespace AGL {

class TGLTexture;

class TinyGLRenderer : public Renderer {
public:
	TinyGLRenderer();

	Target *setupScreen(int screenW, int screenH, bool fullscreen, int bpp);
	void setupCamera(float fov, float nclip, float fclip, float roll);
	void positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest);

	void enableLighting();
	void disableLighting();

	Bitmap2D *createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	Texture *createTexture(const Graphics::PixelBuffer &buf, int width, int height);
	Mesh *createMesh();
	Light *createLight(Light::Type type);
	Primitive *createPrimitive();
	ShadowPlane *createShadowPlane();
	Label *createLabel();

	const char *prettyString() const;

// private:
	TinyGL::ZBuffer *_zb;

	friend class TGLBitmap2D;
};

}

#endif
