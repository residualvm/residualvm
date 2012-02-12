/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/str.h"
#include "common/system.h"

#include "graphics/surface.h"
#include "graphics/pixelbuffer.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/renderer.h"
#include "graphics/agl/rendererfactory.h"
#include "graphics/agl/label.h"

namespace Common {
DECLARE_SINGLETON(AGL::Manager);
}

namespace AGL {

extern	RendererFactory *_instance;

Manager::Manager() {
	_renderer = NULL;
}

void Manager::init(const Common::String &renderer) {
	delete _renderer;
	_renderer = NULL;

	RendererPlugin *p = RendererFactory::instance().getPlugin(renderer);
	if (p) {
		_renderer = (*p)->createInstance();
	}
	assert(_renderer);
}

Target *Manager::setupScreen(int screenW, int screenH, bool fullscreen, int bpp) {
	_target = _renderer->setupScreen(screenW, screenH, fullscreen, bpp);

	_fullscreen = g_system->getFeatureState(OSystem::kFeatureFullscreenMode);

	g_system->showMouse(!fullscreen);
	g_system->setWindowCaption(_renderer->prettyName().c_str());

	return _target;
}

bool Manager::isFullscreen() const {
	return _fullscreen;
}

bool Manager::isHardwareAccelerated() const {
	return _renderer->isHardwareAccelerated();
}

Common::String Manager::getRendererName() const {
	return _renderer->getName();
}

void Manager::setupCamera(float fov, float nclip, float fclip, float roll) {
	_renderer->setupCamera(fov, nclip, fclip, roll);
}

void Manager::positionCamera(const Math::Matrix3x3 &worldRot,  const Math::Vector3d &pos, const Math::Vector3d &interest) {
	_renderer->positionCamera(worldRot, pos, interest);
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

Bitmap2D *Manager::createBitmap2D(Graphics::Surface *surface) {
	return _renderer->createBitmap2D(Bitmap2D::Image, Graphics::PixelBuffer(surface->format, (byte *)surface->pixels),
									 surface->w, surface->h);
}

Texture *Manager::createTexture(const Graphics::PixelBuffer &buf, int width, int height) {
	return _renderer->createTexture(buf, width, height);
}

Texture *Manager::createTexture(Graphics::Surface *surface) {
	return _renderer->createTexture(Graphics::PixelBuffer(surface->format, (byte *)surface->pixels), surface->w, surface->h);
}

Mesh *Manager::createMesh() {
	return _renderer->createMesh();
}

Light *Manager::createLight(Light::Type type) {
	return _renderer->createLight(type);
}

Primitive *Manager::createPrimitive() {
	return _renderer->createPrimitive();
}

ShadowPlane *Manager::createShadowPlane() {
	return _renderer->createShadowPlane();
}

Font *Manager::createFont(FontMetric *metric, const Graphics::PixelBuffer &buf, int width, int height) {
	return _renderer->createFont(metric, buf, width, height);
}

Label *Manager::createLabel(Font *font) {
	Label *l = _renderer->createLabel();
	l->setFont(font);

	return l;
}

Label *Manager::createLabel(Font *font, const Common::String &string) {
	Label *l = _renderer->createLabel();
	l->setFont(font);
	l->setText(string);

	return l;
}

Sprite *Manager::createSprite(float width, float height) {
	return _renderer->createSprite(width, height);
}

}
