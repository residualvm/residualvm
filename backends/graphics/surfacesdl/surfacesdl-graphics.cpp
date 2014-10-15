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

#include "common/scummsys.h"

#if defined(SDL_BACKEND)

#include "backends/graphics/surfacesdl/surfacesdl-graphics.h"
#include "backends/events/sdl/sdl-events.h"
#include "backends/platform/sdl/sdl.h"
#include "common/config-manager.h"
#include "common/mutex.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/util.h"
#ifdef USE_RGB_COLOR
#include "common/list.h"
#endif
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/scaler.h"
#include "graphics/surface.h"
#include "graphics/pixelbuffer.h"
#include "gui/EventRecorder.h"

static const OSystem::GraphicsMode s_supportedGraphicsModes[] = {
	{0, 0, 0}
};

SurfaceSdlGraphicsManager::SurfaceSdlGraphicsManager(SdlEventSource *sdlEventSource)
	:
	SdlGraphicsManager(sdlEventSource),
	_screen(0),
	_overlayVisible(false),
	_overlayscreen(0),
	_overlayWidth(0), _overlayHeight(0),
	_overlayDirty(true),
	_screenChangeCount(0)
#ifdef USE_OPENGL
	, _opengl(false), _overlayNumTex(0), _overlayTexIds(0),
	_gameScreen(0), _gameScreenWidth(0), _gameScreenHeight(0),
	_gameScreenDirty(true)
#endif
#ifdef USE_OPENGL_SHADERS
	, _boxShader(nullptr), _boxVerticesVBO(0)
#endif
	{
}

SurfaceSdlGraphicsManager::~SurfaceSdlGraphicsManager() {
	closeOverlay();
}

void SurfaceSdlGraphicsManager::activateManager() {
	SdlGraphicsManager::activateManager();

	// Register the graphics manager as a event observer
	g_system->getEventManager()->getEventDispatcher()->registerObserver(this, 10, false);
}

void SurfaceSdlGraphicsManager::deactivateManager() {
	// Unregister the event observer
	if (g_system->getEventManager()->getEventDispatcher()) {
		g_system->getEventManager()->getEventDispatcher()->unregisterObserver(this);
	}

	SdlGraphicsManager::deactivateManager();
}

void SurfaceSdlGraphicsManager::resetGraphicsScale() {
	setGraphicsMode(0);
}

bool SurfaceSdlGraphicsManager::hasFeature(OSystem::Feature f) {
	return
		(f == OSystem::kFeatureFullscreenMode) ||
#ifdef USE_OPENGL
		(f == OSystem::kFeatureOpenGL);
#else
	false;
#endif
}

void SurfaceSdlGraphicsManager::setFeatureState(OSystem::Feature f, bool enable) {
	switch (f) {
	case OSystem::kFeatureFullscreenMode:
		_fullscreen = enable;
		break;
	default:
		break;
	}
}

bool SurfaceSdlGraphicsManager::getFeatureState(OSystem::Feature f) {
	switch (f) {
		case OSystem::kFeatureFullscreenMode:
			return _fullscreen;
		default:
			return false;
	}
}

const OSystem::GraphicsMode *SurfaceSdlGraphicsManager::getSupportedGraphicsModes() const {
	return s_supportedGraphicsModes;
}

int SurfaceSdlGraphicsManager::getDefaultGraphicsMode() const {
	return 0;// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::beginGFXTransaction() {
	// ResidualVM: not use it
}

OSystem::TransactionError SurfaceSdlGraphicsManager::endGFXTransaction() {
	// ResidualVM: not use it
	return OSystem::kTransactionSuccess;
}

#ifdef USE_RGB_COLOR
Common::List<Graphics::PixelFormat> SurfaceSdlGraphicsManager::getSupportedFormats() const {
	// ResidualVM: not use it
	return _supportedFormats;
}
#endif

bool SurfaceSdlGraphicsManager::setGraphicsMode(int mode) {
	// ResidualVM: not use it
	return true;
}

int SurfaceSdlGraphicsManager::getGraphicsMode() const {
	// ResidualVM: not use it
	return 0;
}

void SurfaceSdlGraphicsManager::initSize(uint w, uint h, const Graphics::PixelFormat *format) {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::launcherInitSize(uint w, uint h) {
	closeOverlay();
	setupScreen(w, h, false, false);
}

Graphics::PixelBuffer SurfaceSdlGraphicsManager::setupScreen(uint screenW, uint screenH, bool fullscreen, bool accel3d) {
	uint32 sdlflags;
	int bpp;

	closeOverlay();

#ifdef USE_OPENGL
	_tinygl = !accel3d;
	// This should be decided based on some way to test if OpenGL is at all available
	_opengl = true;
	_antialiasing = 0;
#endif
	_fullscreen = fullscreen;

#ifdef USE_OPENGL
	if (_opengl) {
		if (ConfMan.hasKey("antialiasing"))
			_antialiasing = ConfMan.getInt("antialiasing");

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		setAntialiasing(true);

		sdlflags = SDL_OPENGL;
		bpp = 24;
	} else
#endif
	{
		bpp = 16;
		sdlflags = SDL_SWSURFACE;
	}

	if (_fullscreen)
		sdlflags |= SDL_FULLSCREEN;

	_screen = SDL_SetVideoMode(screenW, screenH, bpp, sdlflags);
#ifdef USE_OPENGL
	// If 32-bit with antialiasing failed, try 32-bit without antialiasing
	if (!_screen && _opengl && _antialiasing) {
		warning("Couldn't create 32-bit visual with AA, trying 32-bit without AA");
		setAntialiasing(false);
		_screen = SDL_SetVideoMode(screenW, screenH, bpp, sdlflags);
	}

	// If 32-bit failed, try 16-bit
	if (!_screen && _opengl) {
		warning("Couldn't create 32-bit visual, trying 16-bit");
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
		setAntialiasing(true);
		_screen = SDL_SetVideoMode(screenW, screenH, 0, sdlflags);
	}

	// If 16-bit with antialiasing failed, try 16-bit without antialiasing
	if (!_screen && _opengl && _antialiasing) {
		warning("Couldn't create 16-bit visual with AA, trying 16-bit without AA");
		setAntialiasing(false);
		_screen = SDL_SetVideoMode(screenW, screenH, 0, sdlflags);
	}

	// If 16-bit with alpha failed, try 16-bit without alpha
	if (!_screen && _opengl) {
		warning("Couldn't create 16-bit visual with alpha, trying without alpha");
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
		setAntialiasing(true);
		_screen = SDL_SetVideoMode(screenW, screenH, 0, sdlflags);
	}

	// If 16-bit without alpha and with antialiasing didn't work, try without antialiasing
	if (!_screen && _opengl && _antialiasing) {
		warning("Couldn't create 16-bit visual with AA, trying 16-bit without AA");
		setAntialiasing(false);
		_screen = SDL_SetVideoMode(screenW, screenH, 0, sdlflags);
	}
#endif

	if (!_screen) {
		warning("Error: %s", SDL_GetError());
		g_system->quit();
	}

#ifdef USE_OPENGL
	if (_opengl) {
		int glflag;
		const GLubyte *str;

		// apply atribute again for sure based on SDL docs
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		str = glGetString(GL_VENDOR);
		debug("INFO: OpenGL Vendor: %s", str);
		str = glGetString(GL_RENDERER);
		debug("INFO: OpenGL Renderer: %s", str);
		str = glGetString(GL_VERSION);
		debug("INFO: OpenGL Version: %s", str);
		SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &glflag);
		debug("INFO: OpenGL Red bits: %d", glflag);
		SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &glflag);
		debug("INFO: OpenGL Green bits: %d", glflag);
		SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &glflag);
		debug("INFO: OpenGL Blue bits: %d", glflag);
		SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &glflag);
		debug("INFO: OpenGL Alpha bits: %d", glflag);
		SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &glflag);
		debug("INFO: OpenGL Z buffer depth bits: %d", glflag);
		SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &glflag);
		debug("INFO: OpenGL Double Buffer: %d", glflag);
		SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &glflag);
		debug("INFO: OpenGL Stencil buffer bits: %d", glflag);

#ifdef USE_OPENGL_SHADERS
		debug("INFO: GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

		debug("INFO: GLEW Version: %s", glewGetString(GLEW_VERSION));

		// GLEW needs to be initialized to use shaders
		GLenum err = glewInit();
		if (err != GLEW_OK) {
			warning("Error: %s", glewGetErrorString(err));
			g_system->quit();
		}

		const GLfloat vertices[] = {
			0.0, 0.0,
			1.0, 0.0,
			0.0, 1.0,
			1.0, 1.0,
		};

		// Setup the box shader used to render the overlay
		const char* attributes[] = { "position", "texcoord", NULL };
		_boxShader = Graphics::Shader::fromStrings("box", Graphics::BuiltinShaders::boxVertex, Graphics::BuiltinShaders::boxFragment, attributes);
		_boxVerticesVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, sizeof(vertices), vertices);
		_boxShader->enableVertexAttribute("position", _boxVerticesVBO, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), 0);
		_boxShader->enableVertexAttribute("texcoord", _boxVerticesVBO, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), 0);
#endif

	}
#endif

	_overlayWidth = _gameScreenWidth = screenW;
	_overlayHeight = _gameScreenHeight = screenH;

#ifdef USE_OPENGL
	if (_opengl) {
		uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0x00001f00;
		gmask = 0x000007e0;
		bmask = 0x000000f8;
		amask = 0x00000000;
#else
		rmask = 0x0000f800;
		gmask = 0x000007e0;
		bmask = 0x0000001f;
		amask = 0x00000000;
#endif
		_overlayscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, _overlayWidth, _overlayHeight, 16,
						rmask, gmask, bmask, amask);
		if (_tinygl) {
			_gameScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, _gameScreenWidth, _gameScreenHeight, 16,
						rmask, gmask, bmask, amask);
			_gameScreenGLFormat = GL_UNSIGNED_SHORT_5_6_5;
		}
		_overlayScreenGLFormat = GL_UNSIGNED_SHORT_5_6_5;
	} else
#endif
	{
		_overlayscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, _overlayWidth, _overlayHeight, 16,
					_screen->format->Rmask, _screen->format->Gmask, _screen->format->Bmask, _screen->format->Amask);
	}

	if (!_overlayscreen) {
		warning("Error: %s", SDL_GetError());
		g_system->quit();
	}

	/*_overlayFormat.bytesPerPixel = _overlayscreen->format->BytesPerPixel;

// 	For some reason the values below aren't right, at least on my system
	_overlayFormat.rLoss = _overlayscreen->format->Rloss;
	_overlayFormat.gLoss = _overlayscreen->format->Gloss;
	_overlayFormat.bLoss = _overlayscreen->format->Bloss;
	_overlayFormat.aLoss = _overlayscreen->format->Aloss;

	_overlayFormat.rShift = _overlayscreen->format->Rshift;
	_overlayFormat.gShift = _overlayscreen->format->Gshift;
	_overlayFormat.bShift = _overlayscreen->format->Bshift;
	_overlayFormat.aShift = _overlayscreen->format->Ashift;*/

	_overlayFormat = _gameScreenFormat = Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);

	_screenChangeCount++;

	SDL_PixelFormat *f = _screen->format;
	_screenFormat = Graphics::PixelFormat(f->BytesPerPixel, 8 - f->Rloss, 8 - f->Gloss, 8 - f->Bloss, 0,
										f->Rshift, f->Gshift, f->Bshift, f->Ashift);

	if (_tinygl) {
		_gameScreenFormat = _gameScreenFormat = Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
		return Graphics::PixelBuffer(_gameScreenFormat, (byte *)_gameScreen->pixels);
	}

	return Graphics::PixelBuffer(_screenFormat, (byte *)_screen->pixels);
}

#ifdef USE_OPENGL

#define BITMAP_TEXTURE_SIZE 256

void SurfaceSdlGraphicsManager::updateOverlayTextures() {
	if (!_overlayscreen)
		return;

	// remove if already exist
	if (_overlayNumTex > 0) {
		glDeleteTextures(_overlayNumTex, _overlayTexIds);
		delete[] _overlayTexIds;
		_overlayNumTex = 0;
	}

	_overlayNumTex = ((_overlayWidth + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE) *
					((_overlayHeight + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_overlayTexIds = new GLuint[_overlayNumTex];
	glGenTextures(_overlayNumTex, _overlayTexIds);
	for (int i = 0; i < _overlayNumTex; i++) {
		glBindTexture(GL_TEXTURE_2D, _overlayTexIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, GL_RGB, _overlayScreenGLFormat, NULL);
	}

	int bpp = _overlayscreen->format->BytesPerPixel;

	glPixelStorei(GL_UNPACK_ALIGNMENT, bpp);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, _overlayWidth);

	int curTexIdx = 0;
	for (int y = 0; y < _overlayHeight; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < _overlayWidth; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= _overlayWidth) ? (_overlayWidth - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= _overlayHeight) ? (_overlayHeight - y) : BITMAP_TEXTURE_SIZE;
			glBindTexture(GL_TEXTURE_2D, _overlayTexIds[curTexIdx]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t_width, t_height, GL_RGB, _overlayScreenGLFormat,
				(byte *)_overlayscreen->pixels + (y * _overlayscreen->pitch) + (bpp * x));
			curTexIdx++;
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void SurfaceSdlGraphicsManager::drawOverlayOpenGL() {
	if (!_overlayscreen)
		return;

	// Save current state
	glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_SCISSOR_BIT);

	// prepare view
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _overlayWidth, _overlayHeight, 0, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_SCISSOR_TEST);

	glScissor(0, 0, _overlayWidth, _overlayHeight);

	int curTexIdx = 0;
	for (int y = 0; y < _overlayHeight; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < _overlayWidth; x += BITMAP_TEXTURE_SIZE) {
			glBindTexture(GL_TEXTURE_2D, _overlayTexIds[curTexIdx]);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2i(x, y);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(x + BITMAP_TEXTURE_SIZE, y);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(x + BITMAP_TEXTURE_SIZE, y + BITMAP_TEXTURE_SIZE);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(x, y + BITMAP_TEXTURE_SIZE);
			glEnd();
			curTexIdx++;
		}
	}

	// Restore previous state
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();

	glPopAttrib();
}

void SurfaceSdlGraphicsManager::updateGameScreenTextures() {
	if (!_gameScreen)
		return;

	// remove if already exist
	if (_gameScreenNumTex > 0) {
		glDeleteTextures(_gameScreenNumTex, _gameScreenTexIds);
		delete[] _gameScreenTexIds;
		_gameScreenNumTex = 0;
	}

	_gameScreenNumTex = ((_gameScreenWidth + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE) *
					((_gameScreenHeight + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_gameScreenTexIds = new GLuint[_gameScreenNumTex];
	glGenTextures(_gameScreenNumTex, _gameScreenTexIds);
	for (int i = 0; i < _gameScreenNumTex; i++) {
		glBindTexture(GL_TEXTURE_2D, _gameScreenTexIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, GL_RGB, _gameScreenGLFormat, NULL);
	}

	int bpp = _gameScreen->format->BytesPerPixel;

	glPixelStorei(GL_UNPACK_ALIGNMENT, bpp);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, _gameScreenWidth);

	int curTexIdx = 0;
	for (int y = 0; y < _gameScreenHeight; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < _gameScreenWidth; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= _gameScreenWidth) ? (_gameScreenWidth - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= _gameScreenHeight) ? (_gameScreenHeight - y) : BITMAP_TEXTURE_SIZE;
			glBindTexture(GL_TEXTURE_2D, _gameScreenTexIds[curTexIdx]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t_width, t_height, GL_RGB, _gameScreenGLFormat,
				(byte *)_gameScreen->pixels + (y * _gameScreen->pitch) + (bpp * x));
			curTexIdx++;
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void SurfaceSdlGraphicsManager::drawGameScreenOpenGL() {
	if (!_overlayscreen)
		return;

	// Save current state
	glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_SCISSOR_BIT);

	// prepare view
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _gameScreenWidth, _gameScreenHeight, 0, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_SCISSOR_TEST);

	glScissor(0, 0, _gameScreenWidth, _gameScreenHeight);

	int curTexIdx = 0;
	for (int y = 0; y < _gameScreenHeight; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < _gameScreenWidth; x += BITMAP_TEXTURE_SIZE) {
			glBindTexture(GL_TEXTURE_2D, _gameScreenTexIds[curTexIdx]);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2i(x, y);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(x + BITMAP_TEXTURE_SIZE, y);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(x + BITMAP_TEXTURE_SIZE, y + BITMAP_TEXTURE_SIZE);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(x, y + BITMAP_TEXTURE_SIZE);
			glEnd();
			curTexIdx++;
		}
	}

	// Restore previous state
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();

	glPopAttrib();
}


#ifdef USE_OPENGL_SHADERS
void SurfaceSdlGraphicsManager::drawOverlayOpenGLShaders() {
	if (!_overlayscreen)
		return;

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_SCISSOR_TEST);

	glScissor(0, 0, _overlayWidth, _overlayHeight);

	_boxShader->use();
	_boxShader->setUniform("sizeWH", Math::Vector2d(BITMAP_TEXTURE_SIZE / (float)_overlayWidth, BITMAP_TEXTURE_SIZE / (float)_overlayHeight));
	_boxShader->setUniform("flipY", true);
	_boxShader->setUniform("texcrop", Math::Vector2d(1.0, 1.0));

	int curTexIdx = 0;
	for (int y = 0; y < _overlayHeight; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < _overlayWidth; x += BITMAP_TEXTURE_SIZE) {
			_boxShader->setUniform("offsetXY", Math::Vector2d(x / (float)_overlayWidth, y / (float)_overlayHeight));

			glBindTexture(GL_TEXTURE_2D, _overlayTexIds[curTexIdx]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			curTexIdx++;
		}
	}
}
#endif
#endif

void SurfaceSdlGraphicsManager::drawOverlay() {
	if (!_overlayscreen)
		return;

	SDL_LockSurface(_screen);
	SDL_LockSurface(_overlayscreen);
	Graphics::PixelBuffer srcBuf(_overlayFormat, (byte *)_overlayscreen->pixels);
	Graphics::PixelBuffer dstBuf(_screenFormat, (byte *)_screen->pixels);
	int h = _overlayHeight;

	do {
		dstBuf.copyBuffer(0, _overlayWidth, srcBuf);

		srcBuf.shiftBy(_overlayWidth);
		dstBuf.shiftBy(_overlayWidth);
	} while (--h);
	SDL_UnlockSurface(_screen);
	SDL_UnlockSurface(_overlayscreen);
}

void SurfaceSdlGraphicsManager::updateScreen() {
#ifdef USE_OPENGL
	if (_opengl) {
		if (_tinygl) {
			updateGameScreenTextures();
			drawGameScreenOpenGL();
		}
		if (_overlayVisible) {
			if (_overlayDirty) {
				updateOverlayTextures();
			}

#ifndef USE_OPENGL_SHADERS
			drawOverlayOpenGL();
#else
			drawOverlayOpenGLShaders();
#endif
		}
		SDL_GL_SwapBuffers();
	} else
#endif
	{
		if (_overlayVisible) {
			drawOverlay();
		}
		SDL_Flip(_screen);
	}
}

void SurfaceSdlGraphicsManager::copyRectToScreen(const void *src, int pitch, int x, int y, int w, int h) {
	// ResidualVM: not use it
}

Graphics::Surface *SurfaceSdlGraphicsManager::lockScreen() {
	return NULL; // ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::unlockScreen() {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::fillScreen(uint32 col) {
	// ResidualVM: not use it
}

int16 SurfaceSdlGraphicsManager::getHeight() {
	// ResidualVM specific
	return _screen->h;
}

int16 SurfaceSdlGraphicsManager::getWidth() {
	// ResidualVM specific
	return _screen->w;
}

void SurfaceSdlGraphicsManager::setPalette(const byte *colors, uint start, uint num) {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::grabPalette(byte *colors, uint start, uint num) {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::setCursorPalette(const byte *colors, uint start, uint num) {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::setShakePos(int shake_pos) {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::setFocusRectangle(const Common::Rect &rect) {
	// ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::clearFocusRectangle() {
	// ResidualVM: not use it
}

#pragma mark -
#pragma mark --- Overlays ---
#pragma mark -

void SurfaceSdlGraphicsManager::showOverlay() {
	if (_overlayVisible)
		return;

	_overlayVisible = true;

	clearOverlay();
}

void SurfaceSdlGraphicsManager::hideOverlay() {
	if (!_overlayVisible)
		return;

	_overlayVisible = false;

	clearOverlay();
}

void SurfaceSdlGraphicsManager::clearOverlay() {
	if (!_overlayscreen)
		return;

	if (!_overlayVisible)
		return;

#ifdef USE_OPENGL
	if (_opengl) {
		SDL_Surface *tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, _overlayWidth, _overlayHeight,
				_overlayscreen->format->BytesPerPixel * 8,
				_overlayscreen->format->Rmask, _overlayscreen->format->Gmask,
				_overlayscreen->format->Bmask, _overlayscreen->format->Amask);

		SDL_LockSurface(tmp);
		SDL_LockSurface(_overlayscreen);

		glReadPixels(0, 0, _overlayWidth, _overlayHeight, GL_RGB, _overlayScreenGLFormat, tmp->pixels);

		// Flip pixels vertically
		byte *src = (byte *)tmp->pixels;
		byte *buf = (byte *)_overlayscreen->pixels + (_overlayHeight - 1) * _overlayscreen->pitch;
		int h = _overlayHeight;
		do {
			memcpy(buf, src, _overlayWidth * _overlayscreen->format->BytesPerPixel);
			src += tmp->pitch;
			buf -= _overlayscreen->pitch;
		} while (--h);

		SDL_UnlockSurface(_overlayscreen);
		SDL_UnlockSurface(tmp);

		SDL_FreeSurface(tmp);
	} else
#endif
	{
		SDL_LockSurface(_screen);
		SDL_LockSurface(_overlayscreen);
		Graphics::PixelBuffer srcBuf(_screenFormat, (byte *)_screen->pixels);
		Graphics::PixelBuffer dstBuf(_overlayFormat, (byte *)_overlayscreen->pixels);
		int h = _overlayHeight;

		do {
			dstBuf.copyBuffer(0, _overlayWidth, srcBuf);

			srcBuf.shiftBy(_overlayWidth);
			dstBuf.shiftBy(_overlayWidth);
		} while (--h);
		SDL_UnlockSurface(_screen);
		SDL_UnlockSurface(_overlayscreen);
	}
	_overlayDirty = true;
}

void SurfaceSdlGraphicsManager::grabOverlay(void *buf, int pitch) {
	if (_overlayscreen == NULL)
		return;

	if (SDL_LockSurface(_overlayscreen) == -1)
		error("SDL_LockSurface failed: %s", SDL_GetError());

	byte *src = (byte *)_overlayscreen->pixels;
	byte *dst = (byte *)buf;
	int h = _overlayHeight;
	do {
		memcpy(dst, src, _overlayWidth * _overlayscreen->format->BytesPerPixel);
		src += _overlayscreen->pitch;
		dst += pitch;
	} while (--h);

	SDL_UnlockSurface(_overlayscreen);
}

void SurfaceSdlGraphicsManager::copyRectToOverlay(const void *buf, int pitch, int x, int y, int w, int h) {
	if (_overlayscreen == NULL)
		return;

	const byte *src = (const byte *)buf;

	// Clip the coordinates
	if (x < 0) {
		w += x;
		src -= x * _overlayscreen->format->BytesPerPixel;
		x = 0;
	}

	if (y < 0) {
		h += y;
		src -= y * pitch;
		y = 0;
	}

	if (w > _overlayWidth - x) {
		w = _overlayWidth - x;
	}

	if (h > _overlayHeight - y) {
		h = _overlayHeight - y;
	}

	if (w <= 0 || h <= 0)
		return;

	if (SDL_LockSurface(_overlayscreen) == -1)
		error("SDL_LockSurface failed: %s", SDL_GetError());

	byte *dst = (byte *)_overlayscreen->pixels + y * _overlayscreen->pitch + x * _overlayscreen->format->BytesPerPixel;
	do {
		memcpy(dst, src, w * _overlayscreen->format->BytesPerPixel);
		dst += _overlayscreen->pitch;
		src += pitch;
	} while (--h);

	SDL_UnlockSurface(_overlayscreen);
}

void SurfaceSdlGraphicsManager::closeOverlay() {
	if (_overlayscreen) {
		SDL_FreeSurface(_overlayscreen);
		_overlayscreen = NULL;
#ifdef USE_OPENGL
		if (_opengl) {
			if (_overlayNumTex > 0) {
				glDeleteTextures(_overlayNumTex, _overlayTexIds);
				delete[] _overlayTexIds;
				_overlayNumTex = 0;
			}

#ifdef USE_OPENGL_SHADERS
			glDeleteBuffers(1, &_boxVerticesVBO);
			_boxVerticesVBO = 0;

			delete _boxShader;
			_boxShader = nullptr;
#endif
		}
#endif
	}
}

#pragma mark -
#pragma mark --- Mouse ---
#pragma mark -

bool SurfaceSdlGraphicsManager::showMouse(bool visible) {
	SDL_ShowCursor(visible);
	return true;
}

// ResidualVM specific method
bool SurfaceSdlGraphicsManager::lockMouse(bool lock) {
	if (lock)
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	return true;
}

void SurfaceSdlGraphicsManager::warpMouse(int x, int y) {
	//ResidualVM specific
	SDL_WarpMouse(x, y);
}

void SurfaceSdlGraphicsManager::setMouseCursor(const void *buf, uint w, uint h, int hotspot_x, int hotspot_y, uint32 keycolor, bool dontScale, const Graphics::PixelFormat *format) {
	// ResidualVM: not use it
}

#pragma mark -
#pragma mark --- On Screen Display ---
#pragma mark -

#ifdef USE_OSD
void SurfaceSdlGraphicsManager::displayMessageOnOSD(const char *msg) {
	// ResidualVM: not use it
}
#endif


#ifdef USE_OPENGL
void SurfaceSdlGraphicsManager::setAntialiasing(bool enable) {
	// Antialiasing works without setting MULTISAMPLEBUFFERS, but as SDL's official
	// tests set both values, this seems to be the standard way to do it. It could
	// just be that in current OpenGL implementations setting SDL_GL_MULTISAMPLESAMPLES
	// implicitly sets SDL_GL_MULTISAMPLEBUFFERS as well.
	if (_antialiasing && enable) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, _antialiasing);
	} else {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
}
#endif

bool SurfaceSdlGraphicsManager::notifyEvent(const Common::Event &event) {
	//ResidualVM specific:
	switch ((int)event.type) {
	case Common::EVENT_KEYDOWN:
		break;
	case Common::EVENT_KEYUP:
		break;
	default:
		break;
	}

	return false;
}

void SurfaceSdlGraphicsManager::notifyVideoExpose() {
	_forceFull = true;
	//ResidualVM specific:
	updateScreen();
}

void SurfaceSdlGraphicsManager::transformMouseCoordinates(Common::Point &point) {
	return; // ResidualVM: not use it
}

void SurfaceSdlGraphicsManager::notifyMousePos(Common::Point mouse) {
	transformMouseCoordinates(mouse);
	// ResidualVM: not use that:
	//setMousePos(mouse.x, mouse.y);
}

#endif
