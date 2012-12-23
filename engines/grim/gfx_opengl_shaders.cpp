/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#if defined(WIN32) && !defined(__SYMBIAN32__)
#include <windows.h>
// winnt.h defines ARRAYSIZE, but we want our own one...
#undef ARRAYSIZE
#endif

#include <GL/glew.h>

#include "common/endian.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "graphics/surface.h"
#include "graphics/pixelbuffer.h"

#include "engines/grim/gfx_opengl_shaders.h"

namespace Grim {

GfxBase *CreateGfxOpenGL() {
	return new GfxOpenGLS();
}

GfxOpenGLS::GfxOpenGLS() {

}

GfxOpenGLS::~GfxOpenGLS() {

}

byte *GfxOpenGLS::setupScreen(int screenW, int screenH, bool fullscreen) {
	_pixelFormat = g_system->setupScreen(screenW, screenH, fullscreen, true).getFormat();
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		error("Error: %s\n", glewGetErrorString(err));
	}
	assert(GLEW_OK == err);

	_screenWidth = screenW;
	_screenHeight = screenH;
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	_isFullscreen = g_system->getFeatureState(OSystem::kFeatureFullscreenMode);

	g_system->showMouse(!fullscreen);

	char GLDriver[1024];
	sprintf(GLDriver, "ResidualVM: %s/%s with shaders", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	g_system->setWindowCaption(GLDriver);

	// Load emergency built-in font
//  loadEmergFont();

	_screenSize = _screenWidth * _screenHeight * 4;

	return NULL;
}


void GfxOpenGLS::setupCamera(float fov, float nclip, float fclip, float roll) {

}

void GfxOpenGLS::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest, float roll) {

}


void GfxOpenGLS::clearScreen() {

}


/**
 *	Swap the buffers, making the drawn screen visible
 */
void GfxOpenGLS::flipBuffer() {

}


void GfxOpenGLS::getBoundingBoxPos(const Mesh *mesh, int *x1, int *y1, int *x2, int *y2) {

}

void GfxOpenGLS::startActorDraw(const Math::Vector3d &pos, float scale, const Math::Quaternion &quat,
														const bool inOverworld, const float alpha) {

}


void GfxOpenGLS::finishActorDraw() {

}

void GfxOpenGLS::setShadow(Shadow *shadow) {

}

void GfxOpenGLS::drawShadowPlanes() {

}

void GfxOpenGLS::setShadowMode() {

}

void GfxOpenGLS::clearShadowMode() {

}

bool GfxOpenGLS::isShadowModeActive() {

}

void GfxOpenGLS::setShadowColor(byte r, byte g, byte b) {

}

void GfxOpenGLS::getShadowColor(byte *r, byte *g, byte *b) {

}


void GfxOpenGLS::set3DMode() {

}


void GfxOpenGLS::translateViewpointStart() {

}

void GfxOpenGLS::translateViewpoint(const Math::Vector3d &vec) {

}

void GfxOpenGLS::rotateViewpoint(const Math::Angle &angle, const Math::Vector3d &axis) {

}

void GfxOpenGLS::translateViewpointFinish() {

}


void GfxOpenGLS::drawEMIModelFace(const EMIModel* model, const EMIMeshFace* face) {

}

void GfxOpenGLS::drawModelFace(const MeshFace *face, float *vertices, float *vertNormals, float *textureVerts) {

}

void GfxOpenGLS::drawSprite(const Sprite *sprite) {

}


void GfxOpenGLS::enableLights() {

}

void GfxOpenGLS::disableLights() {

}

void GfxOpenGLS::setupLight(Light *light, int lightId) {

}

void GfxOpenGLS::turnOffLight(int lightId) {

}


void GfxOpenGLS::createMaterial(Texture *material, const char *data, const CMap *cmap) {

}

void GfxOpenGLS::selectMaterial(const Texture *material) {

}

void GfxOpenGLS::destroyMaterial(Texture *material) {

}


void GfxOpenGLS::createBitmap(BitmapData *bitmap) {

}


void GfxOpenGLS::drawBitmap(const Bitmap *bitmap, int x, int y, bool initialDraw) {

}


void GfxOpenGLS::destroyBitmap(BitmapData *bitmap) {

}


void GfxOpenGLS::createFont(Font *font) {

}

void GfxOpenGLS::destroyFont(Font *font) {

}


void GfxOpenGLS::createTextObject(TextObject *text) {

}

void GfxOpenGLS::drawTextObject(const TextObject *text) {

}

void GfxOpenGLS::destroyTextObject(TextObject *text) {

}


Bitmap *GfxOpenGLS::getScreenshot(int w, int h) {

}

void GfxOpenGLS::storeDisplay() {

}

void GfxOpenGLS::copyStoredToDisplay() {

}


void GfxOpenGLS::dimScreen() {

}

void GfxOpenGLS::dimRegion(int x, int y, int w, int h, float level) {

}


void GfxOpenGLS::irisAroundRegion(int x1, int y1, int x2, int y2) {

}


void GfxOpenGLS::drawEmergString(int x, int y, const char *text, const Color &fgColor) {

}

void GfxOpenGLS::loadEmergFont() {

}


void GfxOpenGLS::drawRectangle(const PrimitiveObject *primitive) {

}

void GfxOpenGLS::drawLine(const PrimitiveObject *primitive) {

}

void GfxOpenGLS::drawPolygon(const PrimitiveObject *primitive) {

}


void GfxOpenGLS::prepareMovieFrame(Graphics::Surface* frame) {

}

void GfxOpenGLS::drawMovieFrame(int offsetX, int offsetY) {

}


void GfxOpenGLS::releaseMovieFrame() {

}


const char *GfxOpenGLS::getVideoDeviceName() {
	return "OpenGLS Renderer";
}

void GfxOpenGLS::renderBitmaps(bool render) {

}

void GfxOpenGLS::renderZBitmaps(bool render) {

}


void GfxOpenGLS::createSpecialtyTextures() {

}

}
