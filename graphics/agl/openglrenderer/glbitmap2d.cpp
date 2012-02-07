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

#include "common/endian.h"

#include "graphics/pixelbuffer.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/target.h"

#include "graphics/agl/openglrenderer/openglrenderer.h"
#include "graphics/agl/openglrenderer/glbitmap2d.h"

#if defined (SDL_BACKEND) && !defined(__amigaos4__)
#include <SDL_opengl.h>
#undef ARRAYSIZE
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

namespace AGL {

#define BITMAP_TEXTURE_SIZE 256

static void drawDepthBitmap(int x, int y, int w, int h, char *data) {
	const int screenHeight = AGLMan.getTarget()->getHeight();

	if (y + h == screenHeight) {
		glRasterPos2i(x, screenHeight - 1);
		glBitmap(0, 0, 0, 0, 0, -1, NULL);
	} else
		glRasterPos2i(x, y + h);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

	glDrawPixels(w, h, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, data);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthFunc(GL_LESS);
}

GLBitmap2D::GLBitmap2D(OpenGLRenderer *rend, Bitmap2D::Type texType, const Graphics::PixelBuffer &buf, int width, int height)
	: Bitmap2D(texType, width, height),
	  _renderer(rend) {

	byte *texOut = buf.getRawBuffer();

	if (texType == Bitmap2D::Depth) {
		Graphics::PixelFormat f(2, 5, 6, 5, 0, 11, 5, 0, 0);
		if (buf.getFormat() != f) {
			Graphics::PixelBuffer dst(f, width * height, DisposeAfterUse::NO);
			dst.copyBuffer(0, width * height, buf);
			texOut = dst.getRawBuffer();
		}

		// Flip the zbuffer image to match what GL expects
		if (!_renderer->_useDepthShader) {
			uint16 *zbufPtr = reinterpret_cast<uint16 *>(texOut);
			for (int y = 0; y < height / 2; y++) {
				uint16 *ptr1 = zbufPtr + y * width;
				uint16 *ptr2 = zbufPtr + (height - 1 - y) * width;
				for (int x = 0; x < width; x++, ptr1++, ptr2++) {
					uint16 tmp = *ptr1;
					*ptr1 = *ptr2;
					*ptr2 = tmp;
				}
			}
			_data = texOut;
		}
	}
	if (texType == Bitmap2D::Image || _renderer->_useDepthShader) {
		_numTex = ((width + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE) *
						  ((height + (BITMAP_TEXTURE_SIZE - 1)) / BITMAP_TEXTURE_SIZE);
		_texIds = new GLuint[_numTex];
		glGenTextures(_numTex, _texIds);

		byte *texData = 0;

		GLint format = GL_RGBA;
		GLint type = GL_UNSIGNED_BYTE;
		int bytes = buf.getFormat().bytesPerPixel;
		if (texType == Bitmap2D::Depth) {
			format = GL_DEPTH_COMPONENT;
			type = GL_UNSIGNED_SHORT;
			bytes = 2;
		} else if (buf.getFormat() == Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0)) {
			format = GL_RGB;
			type = GL_UNSIGNED_SHORT_5_6_5;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, bytes);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

		for (int i = 0; i < _numTex; i++) {
			glBindTexture(GL_TEXTURE_2D, _texIds[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexImage2D(GL_TEXTURE_2D, 0, format, BITMAP_TEXTURE_SIZE, BITMAP_TEXTURE_SIZE, 0, format, type, NULL);
		}

		int cur_tex_idx = 0;
		for (int y = 0; y < height; y += BITMAP_TEXTURE_SIZE) {
			for (int x = 0; x < width; x += BITMAP_TEXTURE_SIZE) {
				int w  = (x + BITMAP_TEXTURE_SIZE >= width)  ? (width  - x) : BITMAP_TEXTURE_SIZE;
				int h = (y + BITMAP_TEXTURE_SIZE >= height) ? (height - y) : BITMAP_TEXTURE_SIZE;
				glBindTexture(GL_TEXTURE_2D, _texIds[cur_tex_idx]);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type,
								texOut + (y * bytes * width) + (bytes * x));
				cur_tex_idx++;
			}
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		delete[] texData;
	}
}

GLBitmap2D::~GLBitmap2D() {
	glDeleteTextures(_numTex, _texIds);
	delete[] _texIds;
}

void GLBitmap2D::draw(int texX, int texY) {
	int texWidth = getWidth();
	int texHeight = getHeight();

	const int screenWidth = AGLMan.getTarget()->getWidth();
	const int screenHeight = AGLMan.getTarget()->getHeight();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, screenWidth, screenHeight, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	if (getType() == Bitmap2D::Image) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else
		glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	// If drawing a Z-buffer image, but no shaders are available, fall back to the glDrawPixels method.
	if (getType() == Bitmap2D::Depth && !_renderer->_useDepthShader) {
		// Only draw the manual zbuffer when enabled
		drawDepthBitmap(texX, texY, getWidth(), getHeight(), (char *)_data);
		glEnable(GL_LIGHTING);
		return;
	}

	if (getType() == Bitmap2D::Image) { // Normal image
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	} else { // ZBuffer image
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_TRUE);
#ifdef GL_ARB_fragment_program
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
#endif
	}

	glEnable(GL_SCISSOR_TEST);
	glScissor(texX, screenHeight - (texY + texHeight), texWidth, texHeight);
	int cur_tex_idx = 0;
	for (int y = texY; y < (texY + texHeight); y += BITMAP_TEXTURE_SIZE) {
		for (int x = texX; x < (texX + texWidth); x += BITMAP_TEXTURE_SIZE) {
			glBindTexture(GL_TEXTURE_2D, _texIds[cur_tex_idx]);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2i(x, y);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(x + BITMAP_TEXTURE_SIZE, y);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(x + BITMAP_TEXTURE_SIZE, y + BITMAP_TEXTURE_SIZE);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(x, y + BITMAP_TEXTURE_SIZE);
			glEnd();
			cur_tex_idx++;
		}
	}

	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	if (getType() == Bitmap2D::Image) {
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	} else {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthFunc(GL_LESS);
#ifdef GL_ARB_fragment_program
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif
	}
	glEnable(GL_LIGHTING);
}

}