
#ifndef AGL_RENDERER_H
#define AGL_RENDERER_H

#include "base/plugins.h"

#include "math/mathfwd.h"

#include "graphics/agl/bitmap2d.h"
#include "graphics/agl/light.h"
#include "graphics/agl/primitive.h"

namespace Common {
class String;
}

namespace Graphics {
class PixelBuffer;
}

namespace AGL {

class Target;
class Texture;
class Mesh;
class ShadowPlane;
class Label;
class Font;

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	virtual Target *setupScreen(int screenW, int screenH, bool fullscreen, int bpp) = 0;
	virtual void setupCamera(float fov, float nclip, float fclip, float roll) = 0;
	virtual void positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest) = 0;

	virtual void enableLighting() = 0;
	virtual void disableLighting() = 0;

	virtual Bitmap2D *createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height) = 0;
	virtual Texture *createTexture(const Graphics::PixelBuffer &buf, int width, int height) = 0;
	virtual Mesh *createMesh() = 0;
	virtual Light *createLight(Light::Type type) = 0;
	virtual Primitive *createPrimitive() = 0;
	virtual ShadowPlane *createShadowPlane() = 0;
	virtual Label *createLabel() = 0;

	virtual void pushMatrix() { }
	virtual void translate(float x, float y, float z) { }
	virtual void rotate(float deg, float x, float y, float z) { }
	virtual void popMatrix() { }

	virtual const char *prettyString() const = 0;

};

class RendererPluginObject : public PluginObject {
public:
	virtual Renderer *createInstance() = 0;
};

}

#endif
