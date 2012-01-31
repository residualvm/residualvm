
#include "common/system.h"
#include "common/rect.h"

#include "graphics/pixelbuffer.h"

#include "math/vector3d.h"

#include "graphics/agl/tinyglrenderer/tinyglrenderer.h"
#include "graphics/agl/bitmap2d.h"
#include "graphics/agl/target.h"

#include "graphics/tinygl/zgl.h"

namespace AGL {

class TGLBitmap2D : public Bitmap2D {
public:
	TGLBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height)
		: Bitmap2D(type, width, height) {
		_buf.create(buf.getFormat(), width * height, DisposeAfterUse::YES);
		_buf.copyBuffer(0, width * height, buf);
	}

	void draw(int x, int y) {
		_renderer->_zb->pbuf.copyBuffer(0, getWidth()*getHeight(), _buf);
	}

	TinyGLRenderer *_renderer;
	Graphics::PixelBuffer _buf;
};

class TGLTarget : public Target {
public:
	TGLTarget(int width, int height, int bpp, const Graphics::PixelFormat &format)
		: Target(width, height, bpp) {

		_screenSize = width * height;
		_storedDisplay.create(format, _screenSize, DisposeAfterUse::YES);
		_storedDisplay.clear(_screenSize);
	}
	void clear() {
		_renderer->_zb->pbuf.clear(_screenSize);
	}

	void dim(float amount) {
		for (int l = 0; l < _screenSize; l++) {
			uint8 r, g, b;
			_storedDisplay.getRGBAt(l, r, g, b);
			uint32 color = (r + g + b) * amount;
			_storedDisplay.setPixelAt(l, color, color, color);
		}
	}

	void storeContent() {
		_storedDisplay.copyBuffer(0, _screenSize, _renderer->_zb->pbuf);
	}
	void restoreContent() {
		_renderer->_zb->pbuf.copyBuffer(0, _screenSize, _storedDisplay);
	}

	TinyGLRenderer *_renderer;
	int _screenSize;
	Graphics::PixelBuffer _storedDisplay;
};

// below funcs lookAt, transformPoint and tgluProject are from Mesa glu sources
static void lookAt(TGLfloat eyex, TGLfloat eyey, TGLfloat eyez, TGLfloat centerx,
				   TGLfloat centery, TGLfloat centerz, TGLfloat upx, TGLfloat upy, TGLfloat upz) {
	TGLfloat m[16];
	TGLfloat x[3], y[3], z[3];
	TGLfloat mag;

	z[0] = eyex - centerx;
	z[1] = eyey - centery;
	z[2] = eyez - centerz;
	mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	if (mag) {
		z[0] /= mag;
		z[1] /= mag;
		z[2] /= mag;
	}

	y[0] = upx;
	y[1] = upy;
	y[2] = upz;

	x[0] = y[1] * z[2] - y[2] * z[1];
	x[1] = -y[0] * z[2] + y[2] * z[0];
	x[2] = y[0] * z[1] - y[1] * z[0];

	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = -z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (mag) {
		x[0] /= mag;
		x[1] /= mag;
		x[2] /= mag;
	}

	mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	if (mag) {
		y[0] /= mag;
		y[1] /= mag;
		y[2] /= mag;
	}

	#define M(row,col)  m[col * 4 + row]
	M(0, 0) = x[0];
	M(0, 1) = x[1];
	M(0, 2) = x[2];
	M(0, 3) = 0.0f;
	M(1, 0) = y[0];
	M(1, 1) = y[1];
	M(1, 2) = y[2];
	M(1, 3) = 0.0f;
	M(2, 0) = z[0];
	M(2, 1) = z[1];
	M(2, 2) = z[2];
	M(2, 3) = 0.0f;
	M(3, 0) = 0.0f;
	M(3, 1) = 0.0f;
	M(3, 2) = 0.0f;
	M(3, 3) = 1.0f;
	#undef M
	tglMultMatrixf(m);

	tglTranslatef(-eyex, -eyey, -eyez);
				   }


TinyGLRenderer::TinyGLRenderer() {
}

Target *TinyGLRenderer::setupScreen(int screenW, int screenH, bool fullscreen, int bpp) {
	Graphics::PixelBuffer buf = g_system->setupScreen(screenW, screenH, fullscreen, false);
// 	byte *buffer = buf.getRawBuffer();

// 	_pixelFormat = buf.getFormat();
	_zb = TinyGL::ZB_open(screenW, screenH, buf);
	TinyGL::glInit(_zb);

// 	_screenSize = 640 * 480 * _pixelFormat.bytesPerPixel;
// 	_storedDisplay.create(_pixelFormat, 640 * 480, DisposeAfterUse::YES);
// 	_storedDisplay.clear(640 * 480);

// 	_currentShadowArray = NULL;

	TGLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	tglLightModelfv(TGL_LIGHT_MODEL_AMBIENT, ambientSource);

	TGLTarget *t = new TGLTarget(screenW, screenH, bpp, buf.getFormat());
	t->_renderer = this;
	return t;
}

void TinyGLRenderer::setupCamera(float fov, float nclip, float fclip, float roll) {
	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();

	float right = nclip * tan(fov / 2 * (LOCAL_PI / 180));
	tglFrustum(-right, right, -right * 0.75, right * 0.75, nclip, fclip);

	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();

	tglRotatef(roll, 0, 0, -1);
}

void TinyGLRenderer::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest) {
	Math::Vector3d up_vec(0, 0, 1);

	if (pos.x() == interest.x() && pos.y() == interest.y())
		up_vec = Math::Vector3d(0, 1, 0);

	lookAt(pos.x(), pos.y(), pos.z(), interest.x(), interest.y(), interest.z(), up_vec.x(), up_vec.y(), up_vec.z());
}

void TinyGLRenderer::enableLighting() {
	tglEnable(TGL_LIGHTING);
}

void TinyGLRenderer::disableLighting() {
	tglDisable(TGL_LIGHTING);
}

Bitmap2D *TinyGLRenderer::createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height) {
	TGLBitmap2D *b = new TGLBitmap2D(type, buf, width, height);
	b->_renderer = this;
	return b;
}

Texture *TinyGLRenderer::createTexture(const Graphics::PixelBuffer &buf, int width, int height) {
	return NULL;
}

Mesh *TinyGLRenderer::createMesh() {

}

Light *TinyGLRenderer::createLight(Light::Type type) {

}

Primitive *TinyGLRenderer::createPrimitive() {

}

ShadowPlane *TinyGLRenderer::createShadowPlane() {

}

const char *TinyGLRenderer::prettyString() const {
	return "ResidualVM: Software 3D Renderer";
}




class TinyGLPlugin : public RendererPluginObject {
public:
	TinyGLRenderer *createInstance() {
		return new TinyGLRenderer();
	}

	const char *getName() const {
		return "TinyGL";
	}
};

}

REGISTER_PLUGIN_STATIC(TinyGL, PLUGIN_TYPE_AGL_RENDERER, AGL::TinyGLPlugin);
