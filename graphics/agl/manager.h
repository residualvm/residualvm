
#ifndef AGL_MANAGER_H
#define AGL_MANAGER_H

#include "common/singleton.h"

#include "math/mathfwd.h"

#include "graphics/agl/bitmap2d.h"
#include "graphics/agl/light.h"
#include "graphics/agl/shadowplane.h"

namespace Common {
class String;
}

namespace Graphics {
class PixelBuffer;
class Surface;
}

namespace AGL {

class Renderer;
class Target;
class Mesh;
class Texture;
class Primitive;
class ShadowPlane;
class Label;
class Font;

class Manager : public Common::Singleton<Manager> {
public:
	Manager();
	void init(const Common::String &renderer);

	Target *setupScreen(int screenW, int screenH, bool fullscreen, int bpp);
	Target *getTarget() const { return _target; }

	void setupCamera(float fov, float nclip, float fclip, float roll);
	void positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest);
	void flipBuffer();

	void enableLighting();
	void disableLighting();

	Bitmap2D *createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	Bitmap2D *createBitmap2D(Graphics::Surface *surface);
	Texture *createTexture(const Graphics::PixelBuffer &buf, int width, int height);
	Texture *createTexture(Graphics::Surface *surface);
	Mesh *createMesh();
	Light *createLight(Light::Type type);
	Primitive *createPrimitive();
	ShadowPlane *createShadowPlane();
	Label *createLabel(Font *font, const Common::String &string);
	Label *createLabel(Font *font);

private:
	Renderer *_renderer;
	Target *_target;

	friend class ModelView;
};

#define AGLMan AGL::Manager::instance()

}

#endif
