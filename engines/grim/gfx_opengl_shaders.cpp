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
#include "common/file.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "graphics/surface.h"
#include "graphics/pixelbuffer.h"

#include "engines/grim/actor.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/colormap.h"
#include "engines/grim/emi/modelemi.h"
#include "engines/grim/font.h"
#include "engines/grim/gfx_opengl_shaders.h"
#include "engines/grim/grim.h"
#include "engines/grim/material.h"
#include "engines/grim/model.h"
#include "engines/grim/primitives.h"
#include "engines/grim/set.h"


namespace Grim {

GfxBase *CreateGfxOpenGL() {
	return new GfxOpenGLS();
}

GfxOpenGLS::GfxOpenGLS() {

}

GfxOpenGLS::~GfxOpenGLS() {

}

static GLuint loadShader(const char *base, const char *extension, GLenum shaderType) {
	const Common::String filename = Common::String("shaders/") + base + "." + extension;
	Common::File file;
	file.open(filename);
	if (!file.isOpen())
		error("Could not open shader %s!", filename.c_str());

	const int32 size = file.size();
	GLchar *shaderSource = new GLchar[size + 1];
	file.read(shaderSource, size);
	file.close();
	shaderSource[size] = '\0';

	GLuint shader = glCreateShader( shaderType );
	glShaderSource( shader, 1, (const GLchar **)&shaderSource, NULL );
	glCompileShader( shader );

	GLint status;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog( shader, 512, NULL, buffer );
		error("Could not compile shader %s.%s: %s", base, extension, buffer);
	}
	delete[] shaderSource;

	return shader;
}

GLuint GfxOpenGLS::compileShader(const char *vertex, const char *fragment) {
	GLuint shaderProgram = glCreateProgram();

	GLuint vertexShader = loadShader(vertex, "vertex", GL_VERTEX_SHADER);
	GLuint fragmentShader = loadShader(fragment, "fragment", GL_FRAGMENT_SHADER);

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	return shaderProgram;
}

void GfxOpenGLS::setupShaders() {
	_backgroundProgram = compileShader("background");
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

	setupShaders();

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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GfxOpenGLS::flipBuffer() {
	g_system->updateScreen();
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

#define BITMAP_TEXTURE_SIZE 256

void GfxOpenGLS::createBitmap(BitmapData *bitmap) {
	GLuint *textures;

	if (bitmap->_format != 1) {
		for (int pic = 0; pic < bitmap->_numImages; pic++) {
			uint16 *zbufPtr = reinterpret_cast<uint16 *>(bitmap->getImageData(pic).getRawBuffer());
			for (int i = 0; i < (bitmap->_width * bitmap->_height); i++) {
				uint16 val = READ_LE_UINT16(zbufPtr + i);
				// fix the value if it is incorrectly set to the bitmap transparency color
				if (val == 0xf81f) {
					val = 0;
				}
				zbufPtr[i] = 0xffff - ((uint32)val) * 0x10000 / 100 / (0x10000 - val);
			}

			// Flip the zbuffer image to match what GL expects
			for (int y = 0; y < bitmap->_height / 2; y++) {
				uint16 *ptr1 = zbufPtr + y * bitmap->_width;
				uint16 *ptr2 = zbufPtr + (bitmap->_height - 1 - y) * bitmap->_width;
				for (int x = 0; x < bitmap->_width; x++, ptr1++, ptr2++) {
					uint16 tmp = *ptr1;
					*ptr1 = *ptr2;
					*ptr2 = tmp;
				}
			}
		}
	}
	if (bitmap->_format == 1) {
		bitmap->_hasTransparency = false;
		bitmap->_numTex = ((bitmap->_width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE) *
			((bitmap->_height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
		bitmap->_texIds = new GLuint[bitmap->_numTex * bitmap->_numImages];
		textures = (GLuint *)bitmap->_texIds;
		glGenTextures(bitmap->_numTex * bitmap->_numImages, textures);

		byte *texData = 0;
		byte *texOut = 0;

		GLint format = GL_RGBA;
		GLint type = GL_UNSIGNED_BYTE;
		int bytes = 4;
		if (bitmap->_format != 1) {
			format = GL_DEPTH_COMPONENT;
			type = GL_UNSIGNED_SHORT;
			bytes = 2;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, bytes);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap->_width);

		for (int pic = 0; pic < bitmap->_numImages; pic++) {
			if (bitmap->_format == 1 && bitmap->_bpp == 16 && bitmap->_colorFormat != BM_RGB1555) {
				if (texData == 0)
					texData = new byte[4 * bitmap->_width * bitmap->_height];
				// Convert data to 32-bit RGBA format
				byte *texDataPtr = texData;
				uint16 *bitmapData = reinterpret_cast<uint16 *>(bitmap->getImageData(pic).getRawBuffer());
				for (int i = 0; i < bitmap->_width * bitmap->_height; i++, texDataPtr += 4, bitmapData++) {
					uint16 pixel = *bitmapData;
					int r = pixel >> 11;
					texDataPtr[0] = (r << 3) | (r >> 2);
					int g = (pixel >> 5) & 0x3f;
					texDataPtr[1] = (g << 2) | (g >> 4);
					int b = pixel & 0x1f;
					texDataPtr[2] = (b << 3) | (b >> 2);
					if (pixel == 0xf81f) { // transparent
						texDataPtr[3] = 0;
						bitmap->_hasTransparency = true;
					} else {
						texDataPtr[3] = 255;
					}
				}
				texOut = texData;
			} else if (bitmap->_format == 1 && bitmap->_colorFormat == BM_RGB1555) {
				bitmap->convertToColorFormat(pic, Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
				texOut = (byte *)bitmap->getImageData(pic).getRawBuffer();
			} else {
				texOut = (byte *)bitmap->getImageData(pic).getRawBuffer();
			}

			for (int i = 0; i < bitmap->_numTex; i++) {
				glBindTexture(GL_TEXTURE_2D, textures[bitmap->_numTex * pic + i]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glTexImage2D(GL_TEXTURE_2D, 0, format, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, format, type, NULL);
			}

			int cur_tex_idx = bitmap->_numTex * pic;

			for (int y = 0; y < bitmap->_height; y += BITMAP_TEXTURE_SIZE) {
				for (int x = 0; x < bitmap->_width; x += BITMAP_TEXTURE_SIZE) {
					int width  = (x + BITMAP_TEXTURE_SIZE >= bitmap->_width) ? (bitmap->_width - x) : BITMAP_TEXTURE_SIZE;
					int height = (y + BITMAP_TEXTURE_SIZE >= bitmap->_height) ? (bitmap->_height - y) : BITMAP_TEXTURE_SIZE;
					glBindTexture(GL_TEXTURE_2D, textures[cur_tex_idx]);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, type,
						texOut + (y * bytes * bitmap->_width) + (bytes * x));
					cur_tex_idx++;
				}
			}
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		delete[] texData;
		bitmap->freeData();

		GLuint vao;
		glGenVertexArrays( 1, &vao );
		glBindVertexArray( vao );
		bitmap->_bufferVAO = vao;

		GLuint vbo;
		glGenBuffers( 1, &vbo );
		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		glBufferData( GL_ARRAY_BUFFER, bitmap->_numCoords * 4 * sizeof(float), bitmap->_texc, GL_STATIC_DRAW );
		bitmap->_bufferVBO = vbo;

		glUseProgram(_backgroundProgram);

		GLint posAttrib = glGetAttribLocation( _backgroundProgram, "position" );
		glEnableVertexAttribArray( posAttrib );
		glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0 );

		GLint coordAttrib = glGetAttribLocation( _backgroundProgram, "texcoord" );
		glEnableVertexAttribArray( coordAttrib );
		glVertexAttribPointer( coordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2*sizeof(float)) );

		glBindVertexArray(0);
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	}
}


void GfxOpenGLS::drawBitmap(const Bitmap *bitmap, int x, int y, bool initialDraw) {
	BitmapData *data = bitmap->_data;
	GLuint *textures = (GLuint *)bitmap->getTexIds();

	int curLayer, frontLayer;
	if (initialDraw) {
		curLayer = frontLayer = data->_numLayers - 1;
		glDisable(GL_BLEND);
	} else {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		curLayer = data->_numLayers - 2;
		frontLayer = 0;
	}

	glDisable(GL_DEPTH_TEST);

	glUseProgram(_backgroundProgram);
	glBindVertexArray( data->_bufferVAO );
	while (frontLayer <= curLayer) {
		uint32 offset = data->_layers[curLayer]._offset;
		for (uint32 i = offset; i < offset + data->_layers[curLayer]._numImages; ++i) {
			glBindTexture(GL_TEXTURE_2D, textures[data->_verts[i]._texid]);

			glDrawArrays(GL_QUADS, data->_verts[i]._pos, data->_verts[i]._verts);
		}
		curLayer--;
	}
	glBindVertexArray(0);
}


void GfxOpenGLS::destroyBitmap(BitmapData *bitmap) {
	GLuint *textures = (GLuint *)bitmap->_texIds;
	if (textures) {
		glDeleteTextures(bitmap->_numTex * bitmap->_numImages, textures);
		delete[] textures;
		bitmap->_texIds = 0;
	}

	glDeleteBuffers(1, &bitmap->_bufferVAO);
	glDeleteBuffers(1, &bitmap->_bufferVBO);
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
