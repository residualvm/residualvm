/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
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

#if defined(USE_OPENGL) || defined(USE_GLES2) || defined(USE_OPENGL_SHADERS)

#include "engines/myst3/gfx_opengl_texture.h"

#include "graphics/opengl/context.h"

namespace Myst3 {

// From Bit Twiddling Hacks
static uint32 upperPowerOfTwo(uint32 v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

OpenGLTexture::OpenGLTexture(const Graphics::Surface *surface) {
	width = surface->w;
	height = surface->h;
	format = surface->format;

	// Pad the textures if non power of two support is unavailable
	if (OpenGLContext.NPOTSupported) {
		internalHeight = height;
		internalWidth = width;
	} else {
		internalHeight = upperPowerOfTwo(height);
		internalWidth = upperPowerOfTwo(width);
	}

	if (format.bytesPerPixel == 4) {
		internalFormat = GL_RGBA;

		sourceFormat = GL_UNSIGNED_BYTE;
	} else if (format.bytesPerPixel == 2) {
		internalFormat = GL_RGB;
		sourceFormat = GL_UNSIGNED_SHORT_5_6_5;
	} else
		error("Unknown pixel format");

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, internalWidth, internalHeight, 0, internalFormat, sourceFormat, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// TODO: If non power of two textures are unavailable this clamping
	// has no effect on the padded sides (resulting in white lines on the edges)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	update(surface);

}

OpenGLTexture::~OpenGLTexture() {
	glDeleteTextures(1, &id);
}

void OpenGLTexture::update(const Graphics::Surface *surface) {
	updatePartial(surface, Common::Rect(surface->w, surface->h));
}

void OpenGLTexture::updateTexture(const Graphics::Surface* surface, const Common::Rect& rect) {
	glBindTexture(GL_TEXTURE_2D, id);

	if (OpenGLContext.unpackSubImageSupported) {
		const Graphics::Surface subArea = surface->getSubArea(rect);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format.bytesPerPixel);
		glTexSubImage2D(GL_TEXTURE_2D, 0, rect.left, rect.top, subArea.w, subArea.h, internalFormat, sourceFormat, subArea.getPixels());
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	} else {
		// GL_UNPACK_ROW_LENGTH is not supported, don't bother and do a full texture update
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h, internalFormat, sourceFormat, surface->getPixels());
	}
}

void OpenGLTexture::updatePartial(const Graphics::Surface *surface, const Common::Rect &rect) {
	updateTexture(surface, rect);
}

} // End of namespace Myst3

#endif
