
#include "common/str.h"
#include "common/system.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/renderer.h"
#include "graphics/agl/rendererfactory.h"

namespace Common {
DECLARE_SINGLETON(AGL::Manager);
}

namespace AGL {

extern	RendererFactory *_instance;

Manager::Manager() {

}

void Manager::init(const Common::String &renderer) {
	_renderer = NULL;

	RendererPlugin *p = RendererFactory::instance().getPlugin(renderer);
	if (p) {
		_renderer = (*p)->createInstance();
	}
	assert(_renderer);
}

Target *Manager::setupScreen(int screenW, int screenH, bool fullscreen, int bpp) {
	_target = _renderer->setupScreen(screenW, screenH, fullscreen, bpp);

	fullscreen = g_system->getFeatureState(OSystem::kFeatureFullscreenMode);

	g_system->showMouse(!fullscreen);
	g_system->setWindowCaption(_renderer->prettyString());

	return _target;
}

void Manager::setupCamera(float fov, float nclip, float fclip, float roll) {
	_renderer->setupCamera(fov, nclip, fclip, roll);
}

void Manager::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest) {
	_renderer->positionCamera(pos, interest);
}

void Manager::flipBuffer() {
	g_system->updateScreen();
}

void Manager::enableLighting() {
	_renderer->enableLighting();
}

void Manager::disableLighting() {
	_renderer->disableLighting();
}

Bitmap2D *Manager::createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height) {
	return _renderer->createBitmap2D(type, buf, width, height);
}

Texture *Manager::createTexture(const Graphics::PixelBuffer &buf, int width, int height) {
	return _renderer->createTexture(buf, width, height);
}

Mesh *Manager::createMesh() {
	return _renderer->createMesh();
}

Light *Manager::createLight(Light::Type type) {
	return _renderer->createLight(type);
}

}
