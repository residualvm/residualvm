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

static float textured_quad[] = {
//	X   , Y   , S   , T
	0.0f, 0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
};

static float textured_quad_centered[] = {
//	 X   ,  Y   , Z   , S   , T
	-0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
	-0.5f, +0.5f, 0.0f, 0.0f, 0.0f,
	+0.5f, +0.5f, 0.0f, 1.0f, 0.0f,
	+0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
};

GfxBase *CreateGfxOpenGL() {
	return new GfxOpenGLS();
}

GfxOpenGLS::GfxOpenGLS() {
	_smushNumTex = 0;
}

GfxOpenGLS::~GfxOpenGLS() {

}

const GLchar* readFile(const Common::String& filename) {
	Common::File file;
	file.open(Common::String("shaders/") + filename);
	if (!file.isOpen())
		error("Could not open shader %s!", filename.c_str());

	const int32 size = file.size();
	GLchar *shaderSource = new GLchar[size + 1];
	file.read(shaderSource, size);
	file.close();
	shaderSource[size] = '\0';
	return shaderSource;
}

static GLuint loadShader(const char *base, const char *extension, GLenum shaderType) {
	const Common::String filename = Common::String(base) + "." + extension;
	const GLchar *shaderSource = readFile(filename);
	const GLchar *compatSource = readFile(shaderType == GL_VERTEX_SHADER ? "compat.vertex" : "compat.fragment");
	const GLchar *shaderSources[] = {
			"#version 150\n",
			compatSource,
			shaderSource
	};

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 3, shaderSources, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(shader, 512, NULL, buffer);
		error("Could not compile shader %s.%s: %s", base, extension, buffer);
	}
	delete[] shaderSource;
	delete[] compatSource;

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

void GfxOpenGLS::setupBigEBO() {
	// FIXME: Probably way too big...
	unsigned short quad_indices[6 * 1000];

	unsigned short start = 0;
	for (unsigned short *p = quad_indices; p < &quad_indices[6 * 1000]; p += 6) {
		p[0] = p[3] = start++;
		p[1] = start++;
		p[2] = p[4] = start++;
		p[5] = start++;
	}

	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	_bigQuadEBO = ebo;
}

void GfxOpenGLS::setupQuadEBO() {
	unsigned short quad_indices[] = { 0, 2, 2, 0, 2, 3};
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	_quadEBO = ebo;
}

void GfxOpenGLS::setupTexturedQuad() {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	_smushVAO = vao;
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textured_quad), textured_quad, GL_STATIC_DRAW);
	_smushVBO = vbo;

	glUseProgram(_smushProgram);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	GLint posAttrib = glGetAttribLocation(_smushProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	GLint coordAttrib = glGetAttribLocation(_smushProgram, "texcoord");
	glEnableVertexAttribArray(coordAttrib);
	glVertexAttribPointer(coordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GfxOpenGLS::setupTexturedCenteredQuad() {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	_spriteVAO = vao;
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textured_quad_centered), textured_quad_centered, GL_STATIC_DRAW);
	_spriteVBO = vbo;

	GLint posAttrib = glGetAttribLocation(_actorProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			0);
	GLint coordAttrib = glGetAttribLocation(_actorProgram, "texcoord");
	glEnableVertexAttribArray(coordAttrib);
	glVertexAttribPointer(coordAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			(void *) (3 * sizeof(float)));
	GLint colorAttrib = glGetAttribLocation(_actorProgram, "color");
	glDisableVertexAttribArray(colorAttrib);
	glVertexAttrib4f(colorAttrib, 1.0f, 1.0f, 1.0f, 1.0f);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GfxOpenGLS::setupShaders() {
	bool isEMI = g_grim->getGameType() == GType_MONKEY4;
	_backgroundProgram = compileShader(isEMI ? "emi_background" : "grim_background");
	_smushProgram = compileShader("smush");
	_textProgram = compileShader("text");
	_actorProgram = compileShader(isEMI ? "emi_actor" : "grim_actor");
	setupBigEBO();
	setupQuadEBO();
	setupTexturedQuad();
	setupTexturedCenteredQuad();
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

// matrix calculation based on the glm library.
void GfxOpenGLS::setupCamera(float fov, float nclip, float fclip, float roll) {
	if (_fov == fov && _nclip == nclip && _fclip == fclip)
		return;

	_fov = fov; _nclip = nclip; _fclip = fclip;

	float right = nclip * tan(fov / 2 * (LOCAL_PI / 180));
	float left = -right;
	float top = right * 0.75;
	float bottom = -right * 0.75;

	Math::Matrix4 proj;
	proj(0,0) = (2.0f * nclip) / (right - left);
	proj(1,1) = (2.0f * nclip) / (top - bottom);
	proj(2,0) = (right + left) / (right - left);
	proj(2,1) = (top + bottom) / (top - bottom);
	proj(2,2) = -(fclip + nclip) / (fclip - nclip);
	proj(2,3) = -1.0f;
	proj(3,2) = -(2.0f * fclip * nclip) / (fclip - nclip);
	proj(3,3) = 0.0f;
	proj.transpose();

	_projMatrix = proj;
}

void GfxOpenGLS::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest, float roll) {
	if (g_grim->getGameType() == GType_MONKEY4) {
		_currentPos = pos;
		_currentQuat = Math::Quaternion(interest.x(), interest.y(), interest.z(), roll);
	} else {
//		Math::Vector3d up_vec(0, 0, 1);
//
//		glRotatef(roll, 0, 0, -1);
//
//		if (pos.x() == interest.x() && pos.y() == interest.y())
//			up_vec = Math::Vector3d(0, 1, 0);
//
//		gluLookAt(pos.x(), pos.y(), pos.z(), interest.x(), interest.y(), interest.z(), up_vec.x(), up_vec.y(), up_vec.z());
	}
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
	glUseProgram(_actorProgram);
	GLint modelMatrixPos = glGetUniformLocation(_actorProgram, "modelMatrix");
	GLint projMatrixPos = glGetUniformLocation(_actorProgram, "projMatrix");
	GLint viewMatrixPos = glGetUniformLocation(_actorProgram, "viewMatrix");
	GLint extraMatrixPos = glGetUniformLocation(_actorProgram, "extraMatrix");
	GLint cameraPos = glGetUniformLocation(_actorProgram, "cameraPos");
	GLint actorPos = glGetUniformLocation(_actorProgram, "actorPos");
	GLint billboardPos = glGetUniformLocation(_actorProgram, "isBillboard");

	glEnable(GL_DEPTH_TEST);

	Math::Matrix4 viewMatrix = _currentQuat.toMatrix();
	viewMatrix.transpose();

	Math::Matrix4 modelMatrix = quat.toMatrix();
	modelMatrix.transpose();

	Math::Matrix4 extraMatrix;

	_mvpMatrix = _projMatrix * viewMatrix * modelMatrix;

	glUniformMatrix4fv(modelMatrixPos, GL_TRUE, 1, modelMatrix.getData());
	glUniformMatrix4fv(viewMatrixPos, GL_TRUE, 1, viewMatrix.getData());
	glUniformMatrix4fv(projMatrixPos, GL_TRUE, 1, _projMatrix.getData());
	glUniformMatrix4fv(extraMatrixPos, GL_TRUE, 1, extraMatrix.getData());
	glUniform3fv(cameraPos, 1, _currentPos.getData());
	glUniform3fv(actorPos, 1, pos.getData());
	glUniform1i(billboardPos, GL_FALSE);
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
	if (model->_dirtySkeleton) {
		model->_dirtySkeleton = false;
		glBindBuffer(GL_ARRAY_BUFFER, model->_verticesVBO);
		void * bufData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(bufData, model->_drawVertices, 3 * sizeof(float) * model->_numVertices);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBindVertexArray(model->_modelVAO);
	GLint texturedPos = glGetUniformLocation(_actorProgram, "textured");
	glUniform1i(texturedPos, face->_hasTexture ? GL_TRUE : GL_FALSE);

	GLint extraMatrixPos = glGetUniformLocation(_actorProgram, "extraMatrix");
	Math::Matrix4 extraMatrix;
	glUniformMatrix4fv(extraMatrixPos, GL_TRUE, 1, extraMatrix.getData());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face->_indicesEBO);

	GLint colAttrib = glGetAttribLocation(_actorProgram, "color");
	glEnableVertexAttribArray(colAttrib);

	glDrawElements(GL_TRIANGLES, 3 * face->_faceLength, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GfxOpenGLS::drawModelFace(const Mesh *mesh, const MeshFace *face) {

}

void GfxOpenGLS::drawSprite(const Sprite *sprite) {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);
	GLint texturedPos = glGetUniformLocation(_actorProgram, "textured");
	GLint billboardPos = glGetUniformLocation(_actorProgram, "isBillboard");
	GLint extraMatrixPos = glGetUniformLocation(_actorProgram, "extraMatrix");

	glBindVertexArray(_spriteVAO);
	Math::Matrix4 extraMatrix;
	extraMatrix.setPosition(sprite->_pos);
	extraMatrix(0,0) *= sprite->_width;
	extraMatrix(1,1) *= sprite->_height;
	glUniformMatrix4fv(extraMatrixPos, GL_TRUE, 1, extraMatrix.getData());

	glUniform1i(texturedPos, GL_TRUE);
	glUniform1i(billboardPos, GL_TRUE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDisable(GL_ALPHA_TEST);
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
	material->_texture = new GLuint[1];
	glGenTextures(1, (GLuint *)material->_texture);
	char *texdata = new char[material->_width * material->_height * 4];
	char *texdatapos = texdata;

	if (cmap != NULL) { // EMI doesn't have colour-maps
		for (int y = 0; y < material->_height; y++) {
			for (int x = 0; x < material->_width; x++) {
				uint8 col = *(const uint8 *)(data);
				if (col == 0) {
					memset(texdatapos, 0, 4); // transparent
					if (!material->_hasAlpha) {
						texdatapos[3] = '\xff'; // fully opaque
					}
				} else {
					memcpy(texdatapos, cmap->_colors + 3 * (col), 3);
					texdatapos[3] = '\xff'; // fully opaque
				}
				texdatapos += 4;
				data++;
			}
		}
	} else {
		memcpy(texdata, data, material->_width * material->_height * material->_bpp);
	}

	GLuint format = 0;
	GLuint internalFormat = 0;
	if (material->_colorFormat == BM_RGBA) {
		format = GL_RGBA;
		internalFormat = GL_RGBA;
	} else if (material->_colorFormat == BM_BGRA) {
		format = GL_BGRA;
		internalFormat = GL_RGBA;
	} else {	// The only other colorFormat we load right now is BGR
		format = GL_BGR;
		internalFormat = GL_RGB;
	}

	GLuint *textures = (GLuint *)material->_texture;
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, material->_width, material->_height, 0, format, GL_UNSIGNED_BYTE, texdata);
	delete[] texdata;
}

void GfxOpenGLS::selectMaterial(const Texture *material) {
	GLuint *textures = (GLuint *)material->_texture;
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	if (material->_hasAlpha && g_grim->getGameType() == GType_MONKEY4) {
		glEnable(GL_BLEND);
	}

//	// Grim has inverted tex-coords, EMI doesn't
//	if (g_grim->getGameType() != GType_MONKEY4) {
//		glMatrixMode(GL_TEXTURE);
//		glLoadIdentity();
//		glScalef(1.0f / material->_width, 1.0f / material->_height, 1);
//	}
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
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

	if (g_grim->getGameType() == GType_MONKEY4) {
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		bitmap->_bufferVAO = vao;

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, bitmap->_numCoords * 4 * sizeof(float), bitmap->_texc, GL_STATIC_DRAW);
		bitmap->_bufferVBO = vbo;

		glUseProgram(_backgroundProgram);

		GLint posAttrib = glGetAttribLocation(_backgroundProgram, "position");
		glEnableVertexAttribArray(posAttrib);
		glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

		GLint coordAttrib = glGetAttribLocation(_backgroundProgram, "texcoord");
		glEnableVertexAttribArray(coordAttrib);
		glVertexAttribPointer(coordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2*sizeof(float)));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	} else {
		bitmap->_bufferVBO = _smushVBO;
		bitmap->_bufferVAO = _smushVAO;
	}
}


void GfxOpenGLS::drawBitmap(const Bitmap *bitmap, int dx, int dy, bool initialDraw) {
	if (g_grim->getGameType() == GType_MONKEY4 && bitmap->_data->_numImages > 1) {
		BitmapData *data = bitmap->_data;
		GLuint *textures = (GLuint *)bitmap->getTexIds();

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);

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

		glUseProgram(_backgroundProgram);
		glBindVertexArray(data->_bufferVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bigQuadEBO);
		while (frontLayer <= curLayer) {
			uint32 offset = data->_layers[curLayer]._offset;
			for (uint32 i = offset; i < offset + data->_layers[curLayer]._numImages; ++i) {
				glBindTexture(GL_TEXTURE_2D, textures[data->_verts[i]._texid]);

				unsigned short startVertex = data->_verts[i]._pos / 4 * 6;
				unsigned short numVertices = data->_verts[i]._verts / 4 * 6;
				glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_SHORT, (void *)(startVertex * sizeof(unsigned short)));
			}
			curLayer--;
		}
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		return;
	}

	int format = bitmap->getFormat();
	if ((format == 1 && !_renderBitmaps) || (format == 5 && !_renderZBitmaps)) {
		return;
	}

	GLuint *textures = (GLuint *)bitmap->getTexIds();
	if (bitmap->getFormat() == 1 && bitmap->getHasTransparency()) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glDisable(GL_BLEND);
	}

	glUseProgram(_backgroundProgram);
	glBindVertexArray(_smushVAO);
	GLuint drawToZPos = glGetUniformLocation(_backgroundProgram, "drawToZ");
	GLuint offsetPos = glGetUniformLocation(_backgroundProgram, "offset");
	GLuint sizePos = glGetUniformLocation(_backgroundProgram, "sizeWH");
	if (bitmap->getFormat() == 1) { // Normal image
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glUniform1i(drawToZPos, GL_FALSE);
	} else { // ZBuffer image
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_TRUE);
		glUniform1i(drawToZPos, GL_TRUE);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bigQuadEBO);
	int cur_tex_idx = bitmap->getNumTex() * (bitmap->getActiveImage() - 1);
	for (int y = dy; y < (dy + bitmap->getHeight()); y += BITMAP_TEXTURE_SIZE) {
		for (int x = dx; x < (dx + bitmap->getWidth()); x += BITMAP_TEXTURE_SIZE) {
			glBindTexture(GL_TEXTURE_2D, textures[cur_tex_idx]);
			glUniform2f(offsetPos, x * _scaleW / _screenWidth, y * _scaleH / _screenHeight);
			glUniform2f(sizePos, BITMAP_TEXTURE_SIZE * _scaleW / _screenWidth, BITMAP_TEXTURE_SIZE * _scaleH / _screenHeight);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
			cur_tex_idx++;
		}
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisable(GL_BLEND);
	if (bitmap->getFormat() == 1) {
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	} else {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthFunc(GL_LESS);
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

struct FontUserData {
	int size;
	GLuint texture;
};

void GfxOpenGLS::createFont(Font *font) {
	const byte *bitmapData = font->getFontData();
	uint dataSize = font->getDataSize();

	uint8 bpp = 4;
	uint8 charsWide = 16;
	uint8 charsHigh = 16;

	byte *texDataPtr = new byte[dataSize * bpp];
	byte *data = texDataPtr;

	for (uint i = 0; i < dataSize; i++, texDataPtr += bpp, bitmapData++) {
		byte pixel = *bitmapData;
		if (pixel == 0x00) {
			texDataPtr[0] = texDataPtr[1] = texDataPtr[2] = texDataPtr[3] = 0;
		} else if (pixel == 0x80) {
			texDataPtr[0] = texDataPtr[1] = texDataPtr[2] = 0;
			texDataPtr[3] = 255;
		} else if (pixel == 0xFF) {
			texDataPtr[0] = texDataPtr[1] = texDataPtr[2] = texDataPtr[3] = 255;
		}
	}
	int size = 0;
	for (int i = 0; i < 256; ++i) {
		int width = font->getCharDataWidth(i), height = font->getCharDataHeight(i);
		int m = MAX(width, height);
		if (m > size)
			size = m;
	}
	assert(size < 64);
	if (size < 8)
		size = 8;
	if (size < 16)
		size = 16;
	else if (size < 32)
		size = 32;
	else if (size < 64)
		size = 64;

	uint arraySize = size * size * bpp * charsWide * charsHigh;
	byte *temp = new byte[arraySize];
	if (!temp)
		error("Could not allocate %d bytes", arraySize);

	memset(temp, 0, arraySize);

	FontUserData *userData = new FontUserData;
	font->setUserData(userData);
	userData->texture = 0;
	userData->size = size;

	GLuint *texture = &(userData->texture);
	glGenTextures(1, texture);

	for (int i = 0, row = 0; i < 256; ++i) {
		int width = font->getCharDataWidth(i), height = font->getCharDataHeight(i);
		int32 d = font->getCharOffset(i);
		for (int x = 0; x < height; ++x) {
			// a is the offset to get to the correct row.
			// b is the offset to get to the correct line in the character.
			// c is the offset of the character from the start of the row.
			uint a = row * size * size * bpp * charsHigh;
			uint b = x * size * charsWide * bpp;
			uint c = 0;
			if (i != 0)
				c = ((i - 1) % 16) * size * bpp;

			uint pos = a + b + c;
			uint pos2 = d * bpp + x * width * bpp;
			assert(pos + width * bpp <= arraySize);
			assert(pos2 + width * bpp <= dataSize * bpp);
			memcpy(temp + pos, data + pos2, width * bpp);
		}
		if (i != 0 && i % charsWide == 0)
			++row;

	}
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size * charsWide, size * charsHigh, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp);

	delete[] data;
	delete[] temp;
}

void GfxOpenGLS::destroyFont(Font *font) {
	const FontUserData *data = (const FontUserData *)font->getUserData();
	if (data) {
		glDeleteTextures(1, &(data->texture));
		delete data;
	}
}

struct TextUserData {
	GLuint vao, vbo;
	uint32 characters;
	Color  color;
	GLuint texture;
};

void GfxOpenGLS::createTextObject(TextObject *text) {
	const Color &color = text->getFGColor();
	const Font *font = text->getFont();

	const FontUserData *userData = (const FontUserData *)font->getUserData();
	if (!userData)
		error("Could not get font userdata");
	float sizeW = userData->size * _scaleW / _screenWidth;
	float sizeH = userData->size * _scaleH / _screenHeight;
	const Common::String *lines = text->getLines();
	int numLines = text->getNumLines();

	int numCharacters = 0;
	for (int j = 0; j < numLines; ++j) {
		numCharacters += lines[j].size();
	}

	float * bufData = new float[numCharacters * 16];
	float * cur = bufData;

	for (int j = 0; j < numLines; ++j) {
		const Common::String &line = lines[j];
		int x = text->getLineX(j);
		int y = text->getLineY(j);
		for (uint i = 0; i < line.size(); ++i) {
			uint8 character = line[i];
			float w = y + font->getCharStartingLine(character);
			if (g_grim->getGameType() == GType_GRIM)
				w += font->getBaseOffsetY();
			float z = x + font->getCharStartingCol(character);
			z *= _scaleW; z /= _screenWidth;
			w *= _scaleH; w /= _screenHeight;
			float width = 1 / 16.f;
			float cx = ((character - 1) % 16) / 16.0f;
			float cy = ((character - 1) / 16) / 16.0f;

			float charData[] = {
					z, w, cx, cy,
					z + sizeW, w, cx + width, cy,
					z + sizeW, w + sizeH, cx + width, cy + width,
					z, w + sizeH, cx, cy + width
			};
			memcpy(cur, charData, 16 * sizeof(float));
			cur += 16;

			x += font->getCharWidth(character);
		}
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numCharacters * 16 * sizeof(float), bufData, GL_STATIC_DRAW);

	glUseProgram(_textProgram);
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLint posAttrib = glGetAttribLocation(_textProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	GLint coordAttrib = glGetAttribLocation(_textProgram, "texcoord");
	glEnableVertexAttribArray(coordAttrib);
	glVertexAttribPointer(coordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
			(void *) (2 * sizeof(float)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	TextUserData * td = new TextUserData;
	td->characters = numCharacters;
	td->vao = vao;
	td->vbo = vbo;
	td->color = color;
	td->texture = userData->texture;
	text->setUserData(td);
	delete[] bufData;
}

void GfxOpenGLS::drawTextObject(const TextObject *text) {
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	const TextUserData * td = (const TextUserData *) text->getUserData();
	assert(td);
	glUseProgram(_textProgram);
	glBindVertexArray(td->vao);

	GLint colorAttrib = glGetUniformLocation(_textProgram, "color");
	glUniform3f(colorAttrib, float(td->color.getRed()) / 255.0f, float(td->color.getGreen()) / 255.0f, float(td->color.getBlue()) / 255.0f);
	glBindTexture(GL_TEXTURE_2D, td->texture);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _bigQuadEBO);
	glDrawElements(GL_TRIANGLES, td->characters * 6, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GfxOpenGLS::destroyTextObject(TextObject *text) {
	const TextUserData * td = (const TextUserData *) text->getUserData();
	glDeleteBuffers(1, &td->vbo);
	glDeleteVertexArrays(1, &td->vao);
	text->setUserData(NULL);
	delete td;
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
	int height = frame->h;
	int width = frame->w;
	byte *bitmap = (byte *)frame->pixels;


	// create texture
	_smushNumTex = ((width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE) *
		((height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
	_smushTexIds = new GLuint[_smushNumTex];
	glGenTextures(_smushNumTex, _smushTexIds);
	for (int i = 0; i < _smushNumTex; i++) {
		glBindTexture(GL_TEXTURE_2D, _smushTexIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

	int curTexIdx = 0;
	for (int y = 0; y < height; y += BITMAP_TEXTURE_SIZE) {
		for (int x = 0; x < width; x += BITMAP_TEXTURE_SIZE) {
			int t_width = (x + BITMAP_TEXTURE_SIZE >= width) ? (width - x) : BITMAP_TEXTURE_SIZE;
			int t_height = (y + BITMAP_TEXTURE_SIZE >= height) ? (height - y) : BITMAP_TEXTURE_SIZE;
			glBindTexture(GL_TEXTURE_2D, _smushTexIds[curTexIdx]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t_width, t_height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, bitmap + (y * 2 * width) + (2 * x));
			curTexIdx++;
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	_smushWidth = (int)(width * _scaleW);
	_smushHeight = (int)(height * _scaleH);
}

void GfxOpenGLS::drawMovieFrame(int offsetX, int offsetY) {
	glUseProgram(_smushProgram);
	glBindVertexArray(_smushVAO);
	glDisable(GL_DEPTH_TEST);

	GLint offsetPos = glGetUniformLocation(_smushProgram, "offsetXY");
	GLint sizePos = glGetUniformLocation(_smushProgram, "sizeWH");

	glBindBuffer(GL_ARRAY_BUFFER, _smushVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	int curTexIdx = 0;
	for (int y = 0; y < _smushHeight; y += (int)(BITMAP_TEXTURE_SIZE * _scaleH)) {
		for (int x = 0; x < _smushWidth; x += (int)(BITMAP_TEXTURE_SIZE * _scaleW)) {
			glBindTexture(GL_TEXTURE_2D, _smushTexIds[curTexIdx]);

			glUniform2f(offsetPos, float(x + offsetX) / _screenWidth, float(y + offsetY) / _screenHeight);
			glUniform2f(sizePos, BITMAP_TEXTURE_SIZE * _scaleW / _screenWidth, BITMAP_TEXTURE_SIZE * _scaleH / _screenHeight);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

			curTexIdx++;
		}
	}
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void GfxOpenGLS::releaseMovieFrame() {
	if (_smushNumTex > 0) {
		glDeleteTextures(_smushNumTex, _smushTexIds);
		delete[] _smushTexIds;
		_smushNumTex = 0;
	}
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

void GfxOpenGLS::createEMIModel(EMIModel *model) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	model->_modelVAO = vao;

	GLuint verticesVBO;
	glGenBuffers(1, &verticesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
	glBufferData(GL_ARRAY_BUFFER, model->_numVertices * 3 * sizeof(float), model->_vertices, GL_STREAM_DRAW);
	model->_verticesVBO = verticesVBO;

//	GLuint normalsVBO;
//	glGenBuffers(1, &normalsVBO);
//	glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
//	glBufferData(GL_ARRAY_BUFFER, model->_numVertices * 3 * sizeof(float), model->_normals, GL_STATIC_DRAW);
//	model->_normalsVBO = normalsVBO;

	GLuint texCoordsVBO;
	glGenBuffers(1, &texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, model->_numVertices * 2 * sizeof(float), model->_texVerts, GL_STATIC_DRAW);
	model->_texCoordsVBO = texCoordsVBO;

	GLuint colorMapVBO;
	glGenBuffers(1, &colorMapVBO);
	glBindBuffer(GL_ARRAY_BUFFER, colorMapVBO);
	glBufferData(GL_ARRAY_BUFFER, model->_numVertices * 4 * sizeof(byte), model->_colorMap, GL_STATIC_DRAW);
	model->_colorMapVBO = colorMapVBO;

	glUseProgram(_actorProgram);
	GLint posAttrib = glGetAttribLocation(_actorProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

	GLint texAttrib = glGetAttribLocation(_actorProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordsVBO);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	GLint colAttrib = glGetAttribLocation(_actorProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glBindBuffer(GL_ARRAY_BUFFER, colorMapVBO);
	glVertexAttribPointer(colAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(byte), 0);

	for (uint32 i = 0; i < model->_numFaces; ++i) {
		EMIMeshFace * face = &model->_faces[i];
		GLuint indicesEBO;
		glGenBuffers(1, &indicesEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, face->_faceLength * 3 * sizeof(uint32), face->_indexes, GL_STATIC_DRAW);
		face->_indicesEBO = indicesEBO;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GfxOpenGLS::createModel(Mesh *mesh) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	mesh->_modelVAO = vao;

	GLuint verticesVBO;
	glGenBuffers(1, &verticesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh->_numVertices * 3 * sizeof(float), mesh->_vertices, GL_STREAM_DRAW);
	mesh->_verticesVBO = verticesVBO;

	//	GLuint normalsVBO;
	//	glGenBuffers(1, &normalsVBO);
	//	glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
	//	glBufferData(GL_ARRAY_BUFFER, model->_numVertices * 3 * sizeof(float), model->_normals, GL_STATIC_DRAW);
	//	mesh->_normalsVBO = normalsVBO;

	GLuint texCoordsVBO;
	glGenBuffers(1, &texCoordsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordsVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh->_numVertices * 2 * sizeof(float), mesh->_textureVerts, GL_STATIC_DRAW);
	mesh->_texCoordsVBO = texCoordsVBO;

	glUseProgram(_actorProgram);
	GLint posAttrib = glGetAttribLocation(_actorProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

	GLint texAttrib = glGetAttribLocation(_actorProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordsVBO);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	for (int i = 0; i < mesh->_numFaces; ++i) {
		MeshFace * face = &mesh->_faces[i];
		GLuint indicesEBO;
		glGenBuffers(1, &indicesEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, face->_numVertices * sizeof(uint32), face->_vertices, GL_STATIC_DRAW);
		face->_indicesEBO = indicesEBO;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}
